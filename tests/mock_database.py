"""
Wipes the database and populates it with mock users and submissions for testing.
Run from the project root: python -m tests.mock_database
"""
import random
import sys
import uuid
from pathlib import Path

from flask import Flask
from werkzeug.security import generate_password_hash

# Ensure project root is on the path when run directly
sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from app.config import DATABASE_URL
from app.models import QueueItem, Submission, User, db
from app.queue_worker import _save_submission

SUBMISSIONS_PER_USER = 45

SPECIAL_USERS = ["soosh", "max", "safi"]
GENERIC_USER_COUNT = 210


def make_app() -> Flask:
    app = Flask(__name__)
    app.config["SQLALCHEMY_DATABASE_URI"] = DATABASE_URL
    db.init_app(app)
    return app


def random_results() -> dict[str, float]:
    return {
        "load_average":   random.uniform(0.1, 3.0),
        "load_minimum":   random.uniform(0.05, 1.5),
        "check_average":  random.uniform(0.1, 3.0),
        "check_minimum":  random.uniform(0.05, 1.5),
        "size_average":   random.uniform(0.1, 3.0),
        "size_minimum":   random.uniform(0.05, 1.5),
        "unload_average": random.uniform(0.1, 3.0),
        "unload_minimum": random.uniform(0.05, 1.5),
        "total_average":  random.uniform(0.5, 10.0),
        "total_minimum":  random.uniform(0.2, 5.0),
    }


def fake_queue_item(user_id: int) -> QueueItem:
    return QueueItem(
        user_id=user_id,
        submission_id=str(uuid.uuid4()),
        timestamp=0,
        code="",
        header="",
    )


def create_user(username: str) -> User:
    user = User(
        username=username,
        password_hash=generate_password_hash(username),
    )
    db.session.add(user)
    db.session.flush()  # populate user.id before returning
    return user


def populate():
    print("Dropping and recreating all tables...")
    db.drop_all()
    db.create_all()

    usernames = SPECIAL_USERS + [f"user{i}" for i in range(1, GENERIC_USER_COUNT + 1)]
    print(f"Creating {len(usernames)} users, {SUBMISSIONS_PER_USER} submissions each...")

    for username in usernames:
        user = create_user(username)
        db.session.commit()

        for _ in range(SUBMISSIONS_PER_USER):
            item = fake_queue_item(user.id)
            _save_submission(item, random_results())

    print(f"Done. {len(usernames)} users, {len(usernames) * SUBMISSIONS_PER_USER} submissions created.")


if __name__ == "__main__":
    app = make_app()
    with app.app_context():
        populate()
