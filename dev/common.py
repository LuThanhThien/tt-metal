from loguru import logger
import json
from pathlib import Path
from typing import Union


INDENT_NUMBER = 4


def safe_save(file_path, content):
    try:
        with open(file_path, "w") as file:
            file.write(content)
        logger.info(f"File saved at {file_path}")
    except Exception as e:
        logger.error(f"Error saving file: {e}")
        return False


def save_json(file_path, content):
    try:
        with open(file_path, "w") as file:
            json.dump(content, file, indent=INDENT_NUMBER)
        logger.info(f"File saved at {file_path}")
    except Exception as e:
        logger.error(f"Error saving file: {e}")


def safe_append(file_path, content):
    try:
        with open(file_path, "a") as file:
            json.dump(content, file, indent=INDENT_NUMBER)
        logger.info(f"File saved at {file_path}")
    except Exception as e:
        logger.error(f"Error saving file: {e}")
        return False


def safe_touch(file_path: Union[str, Path], write_if_empty=True, write_content="[]", exist_ok=True):
    try:
        file_path = Path(file_path)
        file_path.touch(exist_ok=exist_ok)
        if write_if_empty and file_path.stat().st_size == 0:  # Check if the file is empty
            with open(file_path, "w") as file:
                logger.info(f"Creating empty file: {file_path}")
                file.write(write_content)  # Write an empty list if the file is empty
        return file_path
    except Exception as e:
        logger.error(f"Error touching file: {e}")
        return file_path
