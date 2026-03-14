import os
from pathlib import Path

import dotenv

""" import this file for access to the .env constants """

dotenv.load_dotenv()

# app paths
BASE_DIR = Path(__file__).resolve().parent.parent
SPELLER = os.getenv("SPELLER_DIR", "speller")
SPELLER_WS = os.getenv("SPELLER_WORKSPACE_DIR", "speller_workspace")
SPELLER_BASENAME=os.environ["SPELLER_BASENAME"]
BENCHMARK_BASENAME=os.environ["BENCHMARK_BASENAME"]

# Flask app settings
SECRET_KEY = os.environ["SECRET_KEY"]
DATABASE_URL = os.environ["DATABASE_URL"]
MAX_CONTENT_LENGTH = int(os.getenv("MAX_CONTENT_LENGTH", "32768"))

# Benchmark settings
ITERATIONS = os.getenv("ITERATIONS", 1)

# Container configs
CONTAINER_IMAGE_NAME = os.environ["CONTAINER_IMAGE_NAME"]
CONTAINER_SCRIPT = os.getenv("CONTAINER_SCRIPT", "docker_entry.sh")
CONTAINER_MEM = os.getenv("CONTAINER_MEM", "256m")
CONTAINER_CPUS = os.getenv("CONTAINER_CPUS", "0.5")
CONTAINER_PID_LIMIT = os.getenv("CONTAINER_PID_LIMIT", "50")
CONTAINER_FSIZE_LIMIT = os.getenv("CONTAINER_FSIZE_LIMIT", "fsize=26214400")
CONTAINER_TIMEOUT = int( os.getenv("CONTAINER_TIMEOUT", "20") )

# Submission settings
SUBMISSIONS_MAX_PER_USER = int(os.getenv("SUBMISSIONS_MAX_PER_USER", "50"))
SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE = int(os.getenv("SUBMISSIONS_LEADERBOARD_ITEMS_PER_PAGE", "50"))
SUBMISSION_POLL_INTERVAL_MS = int(os.getenv("SUBMISSION_POLL_INTERVAL_MS", "750"))