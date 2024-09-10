from typing import Union, Tuple
from pathlib import Path
import pandas as pd
import numpy as np
import traceback
from datetime import datetime
from loguru import logger
from dev import *
from dev.common import *

# Device core count
DEVICE_CORE_COUNT_MAP = {"grayskull": 108, "wormhole_b0": 64}
core_count = DEVICE_CORE_COUNT_MAP[ARCH_NAME]

# Sheet names
FILTER_PERF_SHEET_NAME = "filtered_ops_perf"
OTHER_DEVICE_PERF_SHEET_NAME = "other_device_ops_perf"
FULL_PERF_SHEET_NAME = "full_ops_perf"
FPS_SHEET_NAME = "fps_data"

# Col name
FPS_FILETERED_COL_NAME = "FPS (MatMul/Conv Ops only)"
FPS_OTHER_COL_NAME = "FPS (Other Device Ops)"
FPS_ALL_COL_NAME = "FPS (All Ops)"

COMPUTE_FPS_CACHE_FILE = DEV_GEN_DIR / str("tools/fps_generator/compute_fps_cache.json")


def assert_file(path: Union[str, Path]) -> Path:
    path = Path(path)
    if not path.exists():
        raise FileNotFoundError(f"File not found at {path}")
    if not path.is_file():
        raise FileNotFoundError(f"Path is not a file {path}")
    return path


def compute_fps_from_perf_result(file: Union[str, Path]) -> Tuple[pd.DataFrame]:
    # Process an file
    name = file.name
    df_csv = None
    if ".xlsx" in name:
        df_csv = pd.read_excel(file)
    # Process a CSV file
    elif ".csv" in name:
        df_csv = pd.read_csv(file)
    else:
        logger.error(f"{file.absolute()} File format not supported. Please provide an Excel or CSV file.")
        return None

    # Insert three empty columns before "CORE COUNT"
    core_count_idx = df_csv.columns.get_loc("CORE COUNT")
    for i in range(3):
        df_csv.insert(core_count_idx, f"Empty {i+1}", "")

    # Calculate FPS for all operations and MatMul + Conv operations only
    # filtered_df = df_csv[df_csv['OP CODE'].str.contains('matmul|conv', case=False, na=False)].copy()
    filtered_df = df_csv[df_csv["COMPUTE KERNEL PATH"].str.contains("matmul|conv", case=False, na=False)].copy()

    total_device_kernel_duration_ns_filtered = filtered_df["DEVICE KERNEL DURATION [ns]"].sum()

    total_device_kernel_duration_s_filtered = total_device_kernel_duration_ns_filtered * 1e-9 + 1e-9

    total_device_kernel_duration_ns = df_csv["DEVICE KERNEL DURATION [ns]"].sum()
    total_device_kernel_duration_s = total_device_kernel_duration_ns * 1e-9 + 1e-9

    fps_filtered = 1 / total_device_kernel_duration_s_filtered
    fps = 1 / total_device_kernel_duration_s

    # Create a sheet for other device operations
    other_device_ops_df = df_csv[
        (df_csv["OP TYPE"] == "tt_dnn_device")
        &
        # (~df_csv['OP CODE'].str.contains('matmult|conv', case=False, na=False))
        (~df_csv["COMPUTE KERNEL PATH"].str.contains("matmult|conv", case=False, na=False))
    ].copy()

    total_device_kernel_duration_ns_other = other_device_ops_df["DEVICE KERNEL DURATION [ns]"].sum()
    total_device_kernel_duration_s_other = total_device_kernel_duration_ns_other * 1e-9 + 1e-9
    fps_other = 1 / total_device_kernel_duration_s_other

    # Display FPS on interface
    logger.info(f"\033[93m{FPS_FILETERED_COL_NAME} = {round(fps_filtered, 3)}\033[0m")
    logger.info(f"\033[93m{FPS_OTHER_COL_NAME} = {round(fps_other, 3)}\033[0m")
    logger.info(f"\033[93m{FPS_ALL_COL_NAME} = {round(fps, 3)}\033[0m")

    adjUtil = "Adjusted Utilization = (PM ideal/device kernel duration)*(108/core count)"

    # Calculate Adjusted Utilization for the filtered data
    filtered_df[adjUtil] = (
        (filtered_df["PM IDEAL [ns]"] / filtered_df["DEVICE KERNEL DURATION [ns]"])
        * (core_count / filtered_df["CORE COUNT"])
        * 100
    )
    filtered_df[adjUtil] = filtered_df[adjUtil].replace([np.inf, -np.inf], np.nan).fillna(0)

    filtered_df[adjUtil] = filtered_df[adjUtil].astype(float)
    filtered_df[adjUtil] = filtered_df[adjUtil].astype(int).astype(str) + "%"
    filtered_df[FPS_FILETERED_COL_NAME] = round(fps_filtered, 3)

    # Reorder columns for filtered data
    cols = list(filtered_df.columns)
    cols.remove("DEVICE KERNEL DURATION [ns]")
    cols.remove("CORE COUNT")
    cols.remove("Empty 1")
    cols.remove("Empty 2")
    cols.remove("Empty 3")
    cols.insert(cols.index(adjUtil), "Empty 1")
    cols.insert(cols.index(adjUtil), "Empty 2")
    cols.insert(cols.index(adjUtil), "Empty 3")
    cols.insert(cols.index(adjUtil), "CORE COUNT")
    cols.insert(cols.index(adjUtil), "DEVICE KERNEL DURATION [ns]")
    filtered_df = filtered_df[cols]

    # Convert DataFrame --> CSV for filtered data
    # output = BytesIO()
    processed_data = filtered_df.to_csv(index=False)

    other_device_ops_df[adjUtil] = (
        (other_device_ops_df["PM IDEAL [ns]"] / other_device_ops_df["DEVICE KERNEL DURATION [ns]"])
        * (core_count / other_device_ops_df["CORE COUNT"])
        * 100
    )
    other_device_ops_df[adjUtil] = other_device_ops_df[adjUtil].replace([np.inf, -np.inf], np.nan).fillna(0)
    other_device_ops_df[adjUtil] = other_device_ops_df[adjUtil].astype(float)
    other_device_ops_df[adjUtil] = other_device_ops_df[adjUtil].astype(int).astype(str) + "%"
    other_device_ops_df[FPS_OTHER_COL_NAME] = round(fps_other, 3)

    # Reorder columns for other device ops
    cols_other = list(other_device_ops_df.columns)
    cols_other.remove("DEVICE KERNEL DURATION [ns]")
    cols_other.remove("CORE COUNT")
    cols_other.remove("Empty 1")
    cols_other.remove("Empty 2")
    cols_other.remove("Empty 3")
    cols_other.insert(cols_other.index(adjUtil), "Empty 1")
    cols_other.insert(cols_other.index(adjUtil), "Empty 2")
    cols_other.insert(cols_other.index(adjUtil), "Empty 3")
    cols_other.insert(cols_other.index(adjUtil), "CORE COUNT")
    cols_other.insert(cols_other.index(adjUtil), "DEVICE KERNEL DURATION [ns]")
    other_device_ops_df = other_device_ops_df[cols_other]

    # output_other_device_ops = BytesIO()
    other_device_ops_data = filtered_df.to_csv(index=False)

    # Create a simple DataFrame with overall FPS + Adj Util
    overall_df = df_csv.copy()
    overall_df[FPS_ALL_COL_NAME] = round(fps, 3)
    overall_df[FPS_FILETERED_COL_NAME] = round(fps_filtered, 3)
    overall_df[adjUtil] = (
        (overall_df["PM IDEAL [ns]"] / overall_df["DEVICE KERNEL DURATION [ns]"])
        * (core_count / overall_df["CORE COUNT"])
        * 100
    )
    overall_df[adjUtil] = overall_df[adjUtil].replace([np.inf, -np.inf], np.nan).fillna(0)
    overall_df[adjUtil] = overall_df[adjUtil].astype(float)
    overall_df[adjUtil] = overall_df[adjUtil].astype(int).astype(str) + "%"
    overall_df = overall_df[cols]
    overall_df[FPS_ALL_COL_NAME] = round(fps, 3)

    # output_overall = BytesIO()
    overall_data = filtered_df.to_csv(index=False)

    fps_df = pd.DataFrame(
        {
            FPS_FILETERED_COL_NAME: [round(fps_filtered, 3)],
            FPS_OTHER_COL_NAME: [round(fps_other, 3)],
            FPS_ALL_COL_NAME: [round(fps, 3)],
        }
    )
    fps_data = fps_df.to_csv(index=False)

    return processed_data, other_device_ops_data, overall_data, fps_data


def compute_fps(path: Union[str, Path], out_path: Union[str, Path, None] = None) -> None:
    # Assert and compute
    path = assert_file(path)

    try:
        processed_data, other_device_ops_data, overall_data, fps_data = compute_fps_from_perf_result(path)
    except Exception as e:
        logger.error(f"Error processing file: {path}")
        logger.error(e)
        return

    # Save the processed data to the same directory
    if out_path is None:
        logger.warning("No output path provided. Saving the processed data to the same directory.")
        logger.info(f"Saving the processed data to {path.parent}")
        out_path = path.parent
    out_path = Path(out_path)
    processed_data_path = out_path / str(FILTER_PERF_SHEET_NAME + ".csv")
    other_device_ops_data_path = out_path / str(OTHER_DEVICE_PERF_SHEET_NAME + ".csv")
    overall_data_path = out_path / str(FULL_PERF_SHEET_NAME + ".csv")
    fps_data_path = out_path / str(FPS_SHEET_NAME + ".csv")

    # Save the processed data to the same directory
    safe_save(processed_data_path, processed_data)
    safe_save(other_device_ops_data_path, other_device_ops_data)
    safe_save(overall_data_path, overall_data)
    safe_save(fps_data_path, fps_data)

    # Append the path to the cache file
    COMPUTE_FPS_CACHE_FILE.parent.mkdir(parents=True, exist_ok=True)
    safe_touch(COMPUTE_FPS_CACHE_FILE)
    cache_json: list = json.load(open(COMPUTE_FPS_CACHE_FILE))
    cache_json.append({"ts": str(datetime.now()), "path": str(out_path.absolute())})
    logger.debug(f"Save path: {str(out_path.absolute())}")
    save_json(COMPUTE_FPS_CACHE_FILE, cache_json)


if __name__ == "__main__":
    import argparse

    parser = argparse.ArgumentParser()

    parser.add_argument("--input", type=str, help="Path to the input file")
    parser.add_argument("--output", type=str, help="Path to the output directory")
    args = parser.parse_args()
    file = args.path
    out_path = args.output

    compute_fps(file, out_path)
