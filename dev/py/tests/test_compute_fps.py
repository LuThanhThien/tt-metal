from pathlib import Path
from dev.py.report.compute_fps import compute_fps
from dev.py.utils import TTPath
from dev.py.common import *

test_data_path = TTPath(DEV_TEST_DIR,  "data/ops_perf_results_resnet_2024_08_27_08_05_31.csv")

def test_compute_fps():
    out_path = TTPath("generated/dev")
    out_path.mkdir(parents=True, exist_ok=True)
    compute_fps(test_data_path, out_path)


if __name__ == "__main__":
    test_compute_fps()
