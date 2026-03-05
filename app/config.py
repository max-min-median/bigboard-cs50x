import os
from pathlib import Path

import dotenv

dotenv.load_dotenv()

# import this file for access to the common constants

BASE_DIR = Path(__file__).resolve().parent.parent
SPELLER = os.getenv("SPELLER_DIR", "speller")
SPELLER_WS = os.getenv("SPELLER_WORKSPACE_DIR", "speller_workspace")