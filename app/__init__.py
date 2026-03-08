import logging
import sys
from logging.handlers import RotatingFileHandler

import colorlog
from flask import Flask
from flask_login import LoginManager

from flask_session import Session

from .config import SECRET_KEY, DATABASE_URL, MAX_CONTENT_LENGTH
from .container import spin_container
from .models import User, db
from .queue_worker import start_queue_worker
from .routes import main


def create_app() -> Flask:
    setup_logging()
    log = logging.getLogger(__name__)

    app = Flask(__name__)

    app.config["SECRET_KEY"] = SECRET_KEY
    app.config["SQLALCHEMY_DATABASE_URI"] = DATABASE_URL
    app.config["MAX_CONTENT_LENGTH"] = MAX_CONTENT_LENGTH

    login_manager = LoginManager()
    login_manager.login_view = "main.login"
    login_manager.init_app(app)

    @login_manager.user_loader
    def load_user(user_id):
        return User.query.get(int(user_id))

    # Configure session
    app.config["SESSION_PERMANENT"] = False
    app.config["SESSION_TYPE"] = "filesystem"
    Session(app)

    db.init_app(app)
    with app.app_context():
        db.create_all()

    app.register_blueprint(main)

    # compiles the benchmark executable and creates some necessary symlinks
    result = spin_container(
        speller_perms="rw", mount_workspace=True, flags=["--initialize"]
    )
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
    console_handler.setFormatter(
        colorlog.ColoredFormatter(
            "%(log_color)s%(levelname)s %(asctime)s %(module)s %(message)s",
            datefmt="%H:%M:%S",
            log_colors={
                "DEBUG": "cyan",
                "WARN": "yellow",
                "INFO": "green",
                "ERROR": "red",
                "CRIT": "red,bg_white",
            },
        )
    )

    # Rotating file handler
    file_handler = RotatingFileHandler(
        "logs/app.log", maxBytes=1024 * 1024 * 5, backupCount=3
    )
    file_handler.setLevel(logging.DEBUG)
    file_handler.setFormatter(
        logging.Formatter("%(levelname)s %(asctime)s %(module)s %(message)s")
    )

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
