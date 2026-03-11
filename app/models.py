from dataclasses import dataclass
import time

from flask_login import UserMixin
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


# not for database, just needed across different modules
@dataclass
class QueueItem:
    user_id: int                # primary key id of User who submitted the request, for use when we save to db
    submission_id: str          # unique id used by client to check status of their submission
    timestamp: int              # Unix timestamp
    code: str                   # contents of submission's dictionary.c
    header: str                 # contents of submission's dictionary.h (blank if using distribution)
    status: str = "pending"     # pending | running | done | error
    output: str = ""            # output for webpage's "terminal"


class User(UserMixin, db.Model):
    id = db.Column(db.Integer, primary_key=True)

    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(200), nullable=False)

    total_submissions = db.Column(db.Integer, nullable=False, default=0)
    registration_date = db.Column(db.BigInteger, nullable=False, default=lambda: int(time.time()))


class Submission(db.Model):
    id = db.Column(db.Integer, primary_key=True)
    # matches QueueItem.submission_id
    submission_id = db.Column(db.String(36), unique=True, nullable=False)

    user_id = db.Column(db.Integer, db.ForeignKey("user.id"), nullable=False)
    user = db.relationship("User", backref=db.backref("submissions", lazy=True))

    # Unix timestamp, user-defined label to differentiate their submissions
    timestamp = db.Column(db.BigInteger, nullable=False, default=lambda: int(time.time()))
    label = db.Column(db.String(100), nullable=True, default="")

    # benchmark results
    load_average    = db.Column(db.Float, nullable=False)
    load_minimum    = db.Column(db.Float, nullable=False)
    check_average   = db.Column(db.Float, nullable=False)
    check_minimum   = db.Column(db.Float, nullable=False)
    size_average    = db.Column(db.Float, nullable=False)
    size_minimum    = db.Column(db.Float, nullable=False)
    unload_average  = db.Column(db.Float, nullable=False)
    unload_minimum  = db.Column(db.Float, nullable=False)
    total_average   = db.Column(db.Float, nullable=False)
    total_minimum   = db.Column(db.Float, nullable=False)