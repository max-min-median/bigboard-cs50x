import logging
from pathlib import Path
import shutil
import subprocess

from flask import Blueprint, Response, jsonify, render_template, request

# project root dir, for building paths reliably
BASE_DIR = Path(__file__).resolve().parent.parent

log = logging.getLogger(__name__)

main = Blueprint("main", __name__)

@main.route("/")
def index() -> str:
    return render_template("index.html")


@main.route("/submit", methods=["POST"])
def submit() -> Response:
    data = request.get_json()
    if not data or "code" not in data:
        return jsonify({"output": "Error: no code received."}), 400

    code = data["code"]
    header = data.get("header", "")

    with open(BASE_DIR / "submission" / "dictionary.c", "w") as f:
        f.write(code)

    if header:
        with open(BASE_DIR / "submission" / "dictionary.h", "w") as f:
            f.write(header)
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

    # TODO better to externalize resource limits to .env file later
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
            timeout=15,
        )
        output = result.stdout + result.stderr
        log.info("Submission processed, exit code: %s", result.returncode)
    except subprocess.TimeoutExpired:
        output = "Error: execution timed out."

        # TODO may need to implement a docker stop command here? Can process keep running?

    return jsonify({"output": output})
