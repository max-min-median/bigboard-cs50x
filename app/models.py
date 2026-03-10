from dataclasses import dataclass

from flask_login import UserMixin
from flask_sqlalchemy import SQLAlchemy

db = SQLAlchemy()


# not for database, just needed across different modules
@dataclass
class QueueItem:
    submission_id: str  # unique id used by client to check status of their submission
    timestamp: int  # Unix timestamp
    code: str  # contents of submission's dictionary.c
    header: str  # contents of submission's dictionary.h (blank if using distribution)
    status: str = "pending"  # pending | running | done | error
    output: str = ""  # output for webpage's "terminal"


class User(UserMixin, db.Model):
    id = db.Column(db.Integer, primary_key=True)
    username = db.Column(db.String(80), unique=True, nullable=False)
    password_hash = db.Column(db.String(200), nullable=False)
