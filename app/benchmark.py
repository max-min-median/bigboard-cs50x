from dataclasses import dataclass
import logging
import subprocess
import re
from .config import BASE_DIR, SPELLER, SPELLER_WS, SPELLER_BASENAME, BENCHMARK_BASENAME, ITERATIONS
from .container import spin_container
from .models import QueueItem

log = logging.getLogger(__name__)


@dataclass
class BenchmarkResult:
    status: str = ""
    output: str = ""
    results: dict[str, float] | None = None


def benchmark_submission(item: QueueItem) -> BenchmarkResult:

    _prepare_submission_files(item)

    compile_result = _compile_submission(item)
    if compile_result.status == "error":
        return compile_result

    execute_result = _execute_benchmark(item)
    return BenchmarkResult(execute_result.status, compile_result.output + "\n" + execute_result.output)


def _prepare_submission_files(item: QueueItem) -> None:
    # write submission code
    with open(BASE_DIR / SPELLER / "dictionary.c", "w") as f:
        f.write(item.code)

    header_file = BASE_DIR / SPELLER / "dictionary.h"
    if item.header:
        with open(header_file, "w") as f:
            f.write(item.header)
    else:
        # if no dictionary.h submitted, use the distribution code version
        try:
            header_file.symlink_to(f"/{SPELLER_WS}/distribution_dictionary.h", target_is_directory=False)
        except FileExistsError:
            pass


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

        result = spin_container(parameters=["-c",
            f"cd /speller && ./speller4 -i {ITERATIONS} -s {signature} texts",
        ])

        if result.stderr:
            log.debug("Error benchmarking %s, exit code: %s", item.submission_id, result.returncode)
            log.debug(result.stderr)
            return BenchmarkResult(status="error", output=result.stderr)
        else:
            # Grab output line:
            if (m := re.search(rf"^\[{signature}\].+?$", result.stdout, re.MULTILINE)):
                m.group()
                log.debug(f"Found output: \"{m.group()}\"")
                return BenchmarkResult(status="done", output=m.group())
            else:
                log.debug("Error benchmarking %s, output string not found!", item.submission_id, result.returncode)
                log.debug(result.stdout)
                return BenchmarkResult(status="error", output=result.stdout)
                
    except subprocess.TimeoutExpired:
        log.warning("Submission %s execution timed out", item.submission_id)
        return BenchmarkResult(status="error", output="Error: execution timed out")

        # TODO may need to implement a docker stop command here in case of timeouts? Can process keep running?
