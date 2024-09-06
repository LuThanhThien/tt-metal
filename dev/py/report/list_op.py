import os
import pathlib
import re
from loguru import logger
from dev.py import *

EXCLUDE_SCAN = [
	pathlib.Path(TT_METAL_HOME) / "python_env",
]

def get_py_files(directory):
	python_files = list(pathlib.Path(directory).rglob('*.py'))
	logger.info(f"Found {len(python_files)} Python files in {directory}.")
	return python_files

def is_excluded(path):
    """Check if the file or directory is within any of the excluded directories."""
    for exclude_path in EXCLUDE_SCAN:
        try:
            path.relative_to(exclude_path)
            return True
        except ValueError:
            continue
    return False

def collect_ttnn_apis(python_files):
    ttnn_apis = set()
    
    ttnn_func_obj_pattern = re.compile(r'ttnn\.\w+')  # Regex to match ttnn.function_name(

    for py_file in python_files:
        # Skip files in excluded directories
        if is_excluded(py_file):
            # logger.info(f"Skipping excluded file: {py_file}")
            continue
        
        try:
            with open(py_file, 'r', encoding='utf-8') as file:
                content = file.read()
                matches = ttnn_func_obj_pattern.findall(content)
                for match in matches:
                    # Remove the trailing parenthesis for cleaner function names
                    function_name = match
                    ttnn_apis.add(function_name)
        except Exception as e:
            logger.error(f"Failed to read {py_file}: {e}")
    
    return ttnn_apis

def main():
    logger.info(f"TT HOME: {TT_METAL_HOME}")
    python_files = get_py_files(TT_METAL_HOME)
    ttnn_apis = collect_ttnn_apis(python_files)
    
    logger.info(f"Collected {len(ttnn_apis)} unique ttnn.* functions/ classes:")
    for function in sorted(ttnn_apis):
        logger.info(f" - {function}")

if __name__ == "__main__":
    main()
