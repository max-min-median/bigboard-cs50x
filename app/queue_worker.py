import logging
import shutil
import subprocess
import threading
import uuid
from dataclasses import dataclass
from pathlib import Path
from time import time_ns

BASE_DIR = Path(__file__).resolve().parent.parent

log = logging.getLogger(__name__)

@dataclass
class QueueItem:
    submission_id: str              # unique id used by client to check status of their submission
    timestamp: int                  # Unix timestamp
    code: str                       # contents of submission's dictionary.c
    header: str                     # contents of submission's dictionary.h (blank if using distribution)
    status: str = "pending"         # pending | running | done | error
    output: str = ""                # output for webpage's "terminal"

_pending: list[QueueItem] = []
_all: dict[str, QueueItem] = {}
_lock = threading.Lock()
_wake_worker_event = threading.Event()


def enqueue(code: str, header: str) -> QueueItem:
    """
    Add a new submission to queue, generating a uuid for it, storing its timestamp, 
    and contents of dictionary.c and dictionary.h. If student checked box to use distribution 
    dictionary.h then header will be an empty string. Set the _wake_worker_event trigger, which 
    will wake up the thread executing the looping _worker function. Return the generated uuid 
    so it can be sent to the user in the response of /submit route to check their status later.
    """
    item = QueueItem(submission_id=str(uuid.uuid4()), timestamp=time_ns(), code=code, header=header)
    with _lock:
        _pending.append(item)
        _all[item.submission_id] = item
    _wake_worker_event.set()
    log.info("Enqueued submission %s, queue length now %d", item.submission_id, len(_pending))
    return item


def get_status(submission_id: str) -> dict | None:
    """
    After submission, users keep asking server for status of their submission while its being processed.
    We give them some useful information like queue position or whether their submission is currently
    running (being benchmarked)
    """
    with _lock:
        item = _all.get(submission_id)
        if not item:
            return None
        if item.status == "pending":
            position = _pending.index(item)
            return {"status": "pending", "position": position}
        elif item.status == "running":
            return {"status": "running"}
        else:
            return {"status": item.status, "output": item.output}


def queue_length() -> int:
    with _lock:
        return len(_pending)


def _run_benchmark_container(item: QueueItem) -> None:
    """
    Write submission contents to files where they will be accessible to a docker container and
    spin up the container. Remove item from q
    """
    log.info("Submission %s starting docker container", item.submission_id)

    with open(BASE_DIR / "submission" / "dictionary.c", "w") as f:
        f.write(item.code)

    if item.header:
        with open(BASE_DIR / "submission" / "dictionary.h", "w") as f:
            f.write(item.header)
    else:
        # if no dictionary.h submitted, use the distribution code version
        shutil.copy(
            BASE_DIR / "submission" / "distribution_dictionary.h",
            BASE_DIR / "submission" / "dictionary.h"
        )

    # Spin up a docker container to compile and run the student's submission.

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

    # TODO better to externalize resource limits and docker settings to .env file

    try:
        result = subprocess.run(
            [
                "docker", "run", "--rm",
                "--network", "none",
                "--memory", "256m",
                "--cpus", "0.5",
                "--cap-drop", "ALL",
                "--pids-limit", "50",
                "--security-opt", "no-new-privileges",
                "--ulimit", "fsize=26214400",
                "-v", f"{BASE_DIR / 'speller'}:/speller:rw",
                "-v", f"{BASE_DIR / 'submission'}:/submission:ro",
                "bigboard-sandbox",
            ],
            capture_output=True,
            text=True,
            timeout=20,
        )
        output = result.stdout + result.stderr
        status = "done"
        log.info("Submission %s finished, exit code: %s", item.submission_id, result.returncode)
    except subprocess.TimeoutExpired:
        output = "Error: execution timed out."
        status = "error"
        log.warning("Submission %s timed out", item.submission_id)

        # TODO may need to implement a docker stop command here? Can process keep running?

    with _lock:
        item.output = output
        item.status = status
        if item in _pending:
            _pending.remove(item)


def _queue_worker() -> None:
    """
    This will loop over and over, sleeping efficiently until _wake_worker_event is triggered,
    at which point it will process all the items in the _pending queue.
    """
    while True:
        _wake_worker_event.wait()
        while True:
            with _lock:
                if not _pending:
                    _wake_worker_event.clear()
                    break
                item = _pending[0]
                item.status = "running"

            _run_benchmark_container(item)


def start_worker() -> None:
    t = threading.Thread(target=_queue_worker, daemon=True, name="queue-worker")
    log.info("Starting queue worker thread")
    t.start()