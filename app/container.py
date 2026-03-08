import logging
import os
from pathlib import Path
import subprocess
from time import perf_counter
from typing import Literal

from .config import *

log = logging.getLogger(__name__)


def spin_container(
        speller_perms: Literal["ro", "rw"] = "ro",
        mount_workspace: bool = False,
        entrypoint: str = "sh",
        parameters: list[str] = [str(Path("/") / SPELLER_WS / CONTAINER_SCRIPT)],
        flags: list[str] = []
    ) -> subprocess.CompletedProcess:
    start_t = perf_counter()
    log.debug("spinup: %s %s %s", entrypoint, parameters, flags)

    # --rm                      remove the container automatically after it exits
    # --network none            no network access from inside the container
    # --memory 256m             hard memory cap
    # --cpus 0.5                limit CPU cores
    # --cap-drop ALL            drop all Linux capabilities (least-privilege)
    # --pids-limit 50           max number of processes/threads (prevents fork bombs)
    # --security-opt ...        prevent processes from gaining new privileges via setuid
    # --ulimit fsize=...        cap any single file write to a max size
    # -v speller:/speller:rw    mount distribution code read-write (compiled output lands here)
    # -v submission:...  :ro    mount student submission files as read-only

    mounts = [ "-v", f"{BASE_DIR / SPELLER}:/{SPELLER}:{speller_perms}" ]
    if mount_workspace:
        mounts.extend([ "-v", f"{BASE_DIR / SPELLER_WS}:/{SPELLER_WS}:rw" ])

    command = [
        "docker", "run", "--rm",
        "-e", f"SPELLER={SPELLER}",
        "-e", f"SPELLER_WS={SPELLER_WS}",
        "--network", "none",
        "--memory", CONTAINER_MEM,
        "--cpus", CONTAINER_CPUS,
        "--cap-drop", "ALL",
        "--pids-limit", CONTAINER_PID_LIMIT,
        "--security-opt", "no-new-privileges",
        "--ulimit", CONTAINER_FSIZE_LIMIT,
    ] + mounts + [
        "--entrypoint", entrypoint,
        CONTAINER_IMAGE_NAME,
    ] + parameters + flags

    result = subprocess.run(command, capture_output=True, text=True, timeout=CONTAINER_TIMEOUT)
    log.debug("spindown: %.3fs. Return Code: %d", perf_counter() - start_t, result.returncode)

    return result