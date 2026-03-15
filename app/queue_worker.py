import logging
import threading
import uuid
from time import time_ns

from .benchmark import benchmark_submission
from .config import SUBMISSIONS_MAX_PER_USER
from .models import QueueItem, Submission, User, db

log = logging.getLogger(__name__)

_pending: list[QueueItem] = []
_all: dict[str, QueueItem] = {}
_lock = threading.Lock()
_wake_worker_event = threading.Event()
_app = None


def enqueue(code: str, header: str, user_id: int) -> QueueItem:
    """
    Add a new submission to queue, generating a uuid for it, storing its timestamp, 
    and contents of dictionary.c and dictionary.h. If student checked box to use distribution 
    dictionary.h then header will be an empty string. Set the _wake_worker_event trigger, which 
    will wake up the thread executing the looping _worker function. Return the generated uuid 
    so it can be sent to the user in the response of /submit route to check their status later.
    """
    item = QueueItem(
        user_id=user_id,
        submission_id=str(uuid.uuid4()),
        timestamp=time_ns(),
        code=code,
        header=header
    )
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
        elif item.status == "compiling":
            return {"status": "compiling"}
        else:
            return {"status": item.status, "output": item.output}


def queue_length() -> int:
    with _lock:
        return len(_pending)


def _save_submission(item: QueueItem, results: dict[str, float]) -> None:
    """Save a completed benchmark to the database, increment the user's submission counter,
    and enforce the per-user submission cap by deleting their worst result if needed."""
    user = db.session.get(User, item.user_id)
    user.total_submissions += 1

    # Enforce cap: delete worst submission if at the limit
    existing = db.session.scalars(
        db.select(Submission)
        .filter_by(user_id=item.user_id)
        .order_by(Submission.total_average.desc())
    ).all()
    if len(existing) >= SUBMISSIONS_MAX_PER_USER:
        db.session.delete(existing[0])

    submission = Submission(submission_id=item.submission_id, user_id=item.user_id, **results)
    try:
        db.session.add(submission)
        db.session.commit()
        log.info("Saved submission %s for user %d (lifetime total: %d)",
            item.submission_id, item.user_id, user.total_submissions)
    except Exception as e:
        db.session.rollback()
        log.exception(e)


def _process_item(item: QueueItem) -> None:
    """
    Write submission contents to files where they will be accessible to a docker container and
    spin up the container. Remove item from q
    """
    log.info("processing submission %s", item.submission_id)

    with _app.app_context():
        result = benchmark_submission(item)

        with _lock:
            item.output = result.output
            item.status = result.status
            if item in _pending:
                _pending.remove(item)

        if result.status == "done" and result.results:
            _save_submission(item, result.results)


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

            _process_item(item)


def start_queue_worker(app) -> None:
    global _app
    _app = app
    t = threading.Thread(target=_queue_worker, daemon=True, name="queue-worker")
    log.info("Starting queue worker thread")
    t.start()