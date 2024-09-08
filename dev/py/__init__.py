import os, subprocess
from typing import *
from pathlib import Path

TT_METAL_HOME : Path = Path(os.environ["TT_METAL_HOME"])
ARCH_NAME : str = os.environ["ARCH_NAME"]

DEV_GEN_DIR : Path = Path(TT_METAL_HOME) / "generated" / "dev" 
DEV_TEST_DIR : Path = Path(TT_METAL_HOME) / "dev" / "py" / "tests"

def TTPath(*path: Union[str, Path]) -> Path:
    return Path(TT_METAL_HOME).joinpath(*path)