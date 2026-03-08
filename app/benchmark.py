from dataclasses import dataclass
import logging
import subprocess

from .config import  BASE_DIR, SPELLER, SPELLER_WS, ITERATIONS
from .container import spin_container
from .models import QueueItem

log = logging.getLogger(__name__)


@dataclass
class BenchmarkResult:
    status: str = ""
    output: str = ""


def benchmark_submission(item: QueueItem) -> BenchmarkResult:

    _prepare_submission_files(item)

    compile_result = _compile_submission(item)
    if compile_result.status == "error":
        return compile_result

    execute_result = _execute_benchmark(item)
    return BenchmarkResult(execute_result.status, compile_result.output + "\n" + execute_result.output)


def _prepare_submission_files(item: QueueItem) -> None:
    # write/symlink submission code
    with open(BASE_DIR / SPELLER_WS / "_dictionary.c", "w") as f:
        f.write(item.code)

    header_file = BASE_DIR / SPELLER_WS / "_dictionary.h"
    header_file.unlink(missing_ok=True)
    if item.header:
        with open(header_file, "w") as f:
            f.write(item.header)
    else:
        # if no dictionary.h submitted, use the distribution code version
        # symlink relative to container's filesystem
        header_file.symlink_to(f"/{SPELLER_WS}/distribution_dictionary.h")


def _compile_submission(item: QueueItem) -> BenchmarkResult:
    # Spin up a docker container to compile and run the student's submission.
    try:
        result = spin_container(speller_perms="rw", mount_workspace=True, flags=["--compile-submission"])
        output = result.stdout + result.stderr
        status = "running" if result.returncode == 0 else "error"

        log.debug("Submission %s finished compiling, exit code: %s", item.submission_id, result.returncode)
        return BenchmarkResult(status=status, output=output)
    except subprocess.TimeoutExpired:
        log.warning("Submission %s compilation timed out", item.submission_id)
        return BenchmarkResult(status="error", output="Error: compilation timed out")


def _execute_benchmark(item: QueueItem) -> BenchmarkResult:
    # ITERATIONS imported up top from config module now

    try:
        # TODO this command is a placeholder, we'll clean it up later
        signature = item.submission_id
        sh_cmds = [f"cd /{SPELLER} && "]

        # switched this to using BASE_DIR - we should use this to build paths
        textpaths = list(map(lambda filepath: filepath.name, BASE_DIR / SPELLER / "texts").iterdir())
        log.debug(textpaths)

        result = spin_container(parameters=["-c",
            f"cd /{SPELLER} && "
            f"./speller -i 5 texts/holmes.txt && echo 'Benchmark:' && ./benchmark -i 5 texts/holmes.txt"])
        output = result.stdout + result.stderr


        # TODO return different statuses
        status = "done"

        log.debug("Submission %s finished benchmark, exit code: %s", item.submission_id, result.returncode)
        return BenchmarkResult("done", output)
    except subprocess.TimeoutExpired:
        log.warning("Submission %s execution timed out", item.submission_id)
        return BenchmarkResult(status="error", output="Error: execution timed out")

        # TODO may need to implement a docker stop command here in case of timeouts? Can process keep running?
