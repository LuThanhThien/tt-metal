import re
import subprocess
from pathlib import Path
import traceback
from typing import Dict, List, Union
from loguru import logger
from dev.py import TTPath, DEV_GEN_DIR

# TODO: Cache for collect pytest tests
PYTEST_FUNC_REGEX = re.compile(r"<Function (test_\w+\[.*?\])>")
GENERATED_LOGS_FOLDER : Path = DEV_GEN_DIR / str("report/collect_pytests")
COLLECT_PYTEST_CACHE_FILE = GENERATED_LOGS_FOLDER / "tests_cache.json"

def assert_path(path: Union[str, Path]) -> Path:
    path = TTPath(path) if not path.exists() else Path(path)
    if not path.exists():
        raise FileNotFoundError(f"Path {path} does not exist")
    return path

def collect_pytest_tests_from_file(path: Union[str, Path]) -> List[str]:
    command = ["pytest", str(path), "--collect-only"]
    logger.debug(f"Running command: {' '.join(command)}")
    result = subprocess.run(command, capture_output=True, text=True)
    output = result.stdout
    
    # Find all test matches
    tests = PYTEST_FUNC_REGEX.findall(output)

    # Construct the full pytest command for each test
    collected_commands = [f"{path}::{test}" for test in tests]
    return collected_commands

def collect_pytest_tests(path: Union[str, Path], wrapped_func : callable = None, **kargs) -> Dict[str, List[str]]:

    def run_wrapped_func(func, command_list, **kargs):
        try:
            func(command_list, **kargs)
        except Exception as e:
            logger.error(f"Error processing tests: {command_list}")
            logger.error(e)
            logger.error(traceback.format_exc())

    path = Path(path)
    logger.debug(f'wrapper_func: {wrapped_func}')
    logger.debug(f'kargs: {kargs}')
    
    # Asserting
    assert_path(path)
    
    # Collect pytests
    commands = {}
    if path.is_dir():
        for sub_path in path.iterdir():
            commands.update(collect_pytest_tests(sub_path, wrapped_func, **kargs))
    elif path.is_file():
        if path.suffix == ".py": 
            logger.info(f"Collecting pytest tests from {path}")
            commands[path] = collect_pytest_tests_from_file(path)
            logger.info(f"Num tests collected: {len(commands[path])}")
            if wrapped_func: run_wrapped_func(wrapped_func, commands[path], **kargs)
    else:
        logger.error(f"Unsupport type for {path}")

    return commands

# Example usage
if __name__ == "__main__":
    import argparse
    parser = argparse.ArgumentParser()

    parser.add_argument("-p", "--path", type=str, required=True, help="Path to the test file")
    args = parser.parse_args()

    path = args.path

    commands = collect_pytest_tests(path)
    for module in commands.keys():
        logger.success(f"\nModule: {module}")
        for command in commands[module]:
            print(f"Command: pytest {command}")

    logger.success("Done!")