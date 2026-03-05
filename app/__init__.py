import os
import logging
from logging.handlers import RotatingFileHandler
import sys

import colorlog
from flask import Flask
from flask_sqlalchemy import SQLAlchemy

from .config import *
from .container import spin_container
from .queue_worker import start_queue_worker
from .routes import main

db = SQLAlchemy()

def create_app() -> Flask:
    setup_logging()
    log = logging.getLogger(__name__)

    app = Flask(__name__)

    app.config["SECRET_KEY"] = os.environ["SECRET_KEY"]
    app.config["SQLALCHEMY_DATABASE_URI"] = os.environ["DATABASE_URL"]
    app.config["MAX_CONTENT_LENGTH"] = int(os.getenv("MAX_CONTENT_LENGTH", "32768"))
    db.init_app(app)

    app.register_blueprint(main)

    # compiles the benchmark executable and creates some necessary symlinks
    result = spin_container(speller_perms="rw", mount_workspace=True, flags=["--initialize"])
    log.info(result.stdout + result.stderr)
    if result.returncode != 0:
        log.error("Error while spinning up container during app initialization")
        sys.exit(1)

    start_queue_worker()

    return app


def setup_logging() -> None:
    # Colored console handler
    console_handler = logging.StreamHandler()
    console_handler.setLevel(logging.DEBUG)
    console_handler.setFormatter(colorlog.ColoredFormatter(
        "%(log_color)s%(levelname)s %(asctime)s %(module)s %(message)s",
        datefmt="%H:%M:%S",
        log_colors={
            "DEBUG": "cyan",
            "WARN": "yellow",
            "INFO": "green",
            "ERROR": "red",
            "CRIT": "red,bg_white",
        }
    ))

    # Rotating file handler
    file_handler = RotatingFileHandler(
        "logs/app.log",
        maxBytes=1024 * 1024 * 5,
        backupCount=3
    )
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(logging.Formatter(
        "%(levelname)s %(asctime)s %(module)s %(message)s"
    ))

    logging.addLevelName(logging.DEBUG, "DEBUG")
    logging.addLevelName(logging.WARNING, "WARN")
    logging.addLevelName(logging.INFO, "INFO")
    logging.addLevelName(logging.ERROR, "ERROR")
    logging.addLevelName(logging.CRITICAL, "CRIT")

    root_logger = logging.getLogger()
    root_logger.setLevel(logging.DEBUG)

    root_logger.handlers.clear()  # prevent duplicates

    root_logger.addHandler(console_handler)
    root_logger.addHandler(file_handler)