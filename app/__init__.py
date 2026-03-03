import os
import logging
from logging.handlers import RotatingFileHandler

import colorlog
from dotenv import load_dotenv
from flask import Flask
from flask_sqlalchemy import SQLAlchemy


db = SQLAlchemy()
load_dotenv()

def create_app():
    app = Flask(__name__)

    app.config["SECRET_KEY"] = os.getenv("SECRET_KEY")
    app.config["SQLALCHEMY_DATABASE_URI"] = os.getenv("DATABASE_URL")
    db.init_app(app)
    setup_logging(app)

    from .routes import main
    app.register_blueprint(main)
    
    return app

def setup_logging(app: Flask):
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