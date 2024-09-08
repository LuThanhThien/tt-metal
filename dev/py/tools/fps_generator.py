from datetime import datetime
import json
import pandas as pd
import traceback
from dev.py import *
from dev.py.common import logger, save_json, safe_touch
from dev.py.report.collect_pytests import collect_pytest_tests
from dev.py.report.compute_fps import FPS_FILETERED_COL_NAME, FPS_OTHER_COL_NAME, FPS_ALL_COL_NAME, COMPUTE_FPS_CACHE_FILE, FPS_SHEET_NAME

DEMO_FOLDER : Path = TT_METAL_HOME / str("models/demos")
GENERATED_LOGS_FOLDER : Path = DEV_GEN_DIR / str("tools/fps_generator")
LOG_FILE_FILE = GENERATED_LOGS_FOLDER /  "fps_generator.log"
TESTS_CACHE_FILE = GENERATED_LOGS_FOLDER / "tests_cache.json"
TEST_CACHE: list = []
ALREADY_TEST_COMMANDS = set([])
TOTAL_TEST = 0
TOTAL_TEST_SUCCESS = 0
START_TIME = datetime.now()

def init_variables():
    # init_variables logger and cache
    global TEST_CACHE
    global ALREADY_TEST_COMMANDS
    
    GENERATED_LOGS_FOLDER.mkdir(parents=True, exist_ok=True)
    logger.add(LOG_FILE_FILE)

    safe_touch(TESTS_CACHE_FILE)
    TEST_CACHE = json.load(open(TESTS_CACHE_FILE))
    # ALREADY_TEST_COMMANDS = set([test["profile_command"] for test in TEST_CACHE if test["success"]])
    ALREADY_TEST_COMMANDS = set([test["profile_command"] for test in TEST_CACHE]) # include all tests

# TODO: Cache for collect pytest tests

def run_command(profile_command: list):
    global TEST_CACHE
    global ALREADY_TEST_COMMANDS
    global TOTAL_TEST
    global TOTAL_TEST_SUCCESS

    cache_json = {
        "ts_start": str(datetime.now()),
        "ts_end": '',
        "profile_command": str(profile_command),
        "success": False,
        "fps_data": {
            FPS_FILETERED_COL_NAME: -1,
            FPS_OTHER_COL_NAME: -1,
            FPS_ALL_COL_NAME: -1
        },
        "output_folder": ''
    }
    try: 
        if ALREADY_TEST_COMMANDS.__contains__(cache_json["profile_command"]):
            logger.warning(f"Command already run: {profile_command}")
            return

        logger.debug(f"Current cache json:\n{json.dumps(cache_json, indent=4)}")
        result = subprocess.run(profile_command)
        logger.info(f"Finished with return code: {result.returncode}")

        # Read the compute_fps_cache file and get last path
        compute_fps_cache = json.load(open(COMPUTE_FPS_CACHE_FILE))
        last_outpath_dict = compute_fps_cache[-1]

        # Check last outpath timestamps
        last_ts = datetime.fromisoformat(last_outpath_dict["ts"])
        output_folder = last_outpath_dict["path"]
        cache_json["success"] = True
        if last_ts >= START_TIME:
            fps_df = pd.read_csv(Path(output_folder) / str(FPS_SHEET_NAME + ".csv"))
            fps_fileterd = fps_df[FPS_FILETERED_COL_NAME].mean()
            fps_other = fps_df[FPS_OTHER_COL_NAME].mean()
            fps = fps_df[FPS_ALL_COL_NAME].mean()

            logger.debug(f"Output folder: {output_folder}")
            logger.debug(f"FPS: {fps_fileterd}, {fps_other}, {fps}")

            cache_json["fps_data"] = {
                FPS_FILETERED_COL_NAME: fps_fileterd,
                FPS_OTHER_COL_NAME: fps_other,
                FPS_ALL_COL_NAME: fps
            }
            cache_json["output_folder"] = str(output_folder)
        else:
            logger.warning(f"No output was generated in: {output_folder}")

        TOTAL_TEST_SUCCESS += 1
    except Exception as e:
        logger.error(f"Error processing command: {profile_command}")
        logger.error(e, exc_info=True)
        logger.error(traceback.format_exc())

    TOTAL_TEST += 1
    TEST_CACHE.append(cache_json)
    cache_json["ts_end"] = str(datetime.now())
    if cache_json["success"]:
        ALREADY_TEST_COMMANDS.add(cache_json["profile_command"])
    save_json(TESTS_CACHE_FILE, TEST_CACHE)
    # logger.debug(f"Test cache: {TEST_CACHE}")
    # logger.debug(f"Already test commands: {ALREADY_TEST_COMMANDS}")


def process_commands(command_list: List[str], save_folder: str = 'demos'):
    # logger.info(f"Running tests: {command_list}")
    for command in command_list:
        # Get command for profiler
        extra_args = " --device-id 2 "
        pytest_command = "\"" + "pytest " +  command + extra_args + "\""
        profile_command = [
            str(TTPath("tt_metal/tools/profiler/profile_this.py")),
            "-n", save_folder,
            "-c",  pytest_command
        ]
        # Run the command
        logger.debug(f"Running profiler test: {profile_command}")
        run_command(profile_command)

def main():
    global TOTAL_TEST
    global TOTAL_TEST_SUCCESS

    init_variables()       
    current_total_test = 0
    current_total_success = 0
    for folder in DEMO_FOLDER.iterdir():
        if folder.is_dir():
            # running the tests
            logger.info(f"Collecting tests from {folder}")
            commands = collect_pytest_tests(folder, wrapped_func=process_commands, save_folder=folder.name)
            
            # summary test for this folder
            current_total_test = TOTAL_TEST - current_total_test
            current_total_success = TOTAL_TEST_SUCCESS - current_total_success  
            logger.success(f"Tests collected from {folder}")
            logger.success(f"Dir: {folder} finished with TOTAL TEST: {current_total_test}; TOTAL SUCCESS: {current_total_success}; TOTAL FAILED: {current_total_test - current_total_success}")
            current_total_test = TOTAL_TEST
            current_total_success = TOTAL_TEST_SUCCESS
        else:
            logger.info(f"Skipping {folder}")


if __name__=="__main__":
    main()

