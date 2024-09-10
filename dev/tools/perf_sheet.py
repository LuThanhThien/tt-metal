import pandas as pd
import os
import glob
from loguru import logger

TT_METAL_HOME = os.getenv("TT_METAL_HOME")


def get_latest_report(base_path):
    try:
        latest_dir = max(
            [os.path.join(base_path, d) for d in os.listdir(base_path) if os.path.isdir(os.path.join(base_path, d))],
            key=os.path.getmtime,
        )
        return max(glob.glob(os.path.join(latest_dir, "*")), key=os.path.getmtime)
    except ValueError:
        return None


report_path = f"{TT_METAL_HOME}/generated/profiler/reports/t5/"
latest_profile_report = get_latest_report(report_path)

if latest_profile_report != None:
    logger.info(f"Found {latest_profile_report}")
    df = pd.read_csv(latest_profile_report)
    print(df)
else:
    logger.error(f"There is no profile report in {report_path}")
