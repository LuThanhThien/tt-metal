from dev.py import *
from dev.py.common import logger


ModelDirType = Dict[str, dict]
MODEL_NAME_PLACEHOLDER = "__model_name__"
MODEL_NAME_PLACEHOLDER_PY = f"{MODEL_NAME_PLACEHOLDER}.py"

MODEL_STRUCTURE : ModelDirType = {
    MODEL_NAME_PLACEHOLDER: {
        "reference": {
            MODEL_NAME_PLACEHOLDER_PY: {}
        },
        "tt": {
            MODEL_NAME_PLACEHOLDER_PY: {}
        },
        "demo": {},
        "tests": {},
    }
}


def create_model_dir(model_name: str, model_struct : ModelDirType = MODEL_STRUCTURE, parent_dir : Union[Path, str] = DEV_MODEL_DIR):
    parent_dir = Path(parent_dir)
    for model_dir, model_subdict in model_struct.items():
        # Replace model name placeholder
        model_dir_path = parent_dir / model_dir
        is_dir = True
        is_exist = False
        if str(model_dir_path).__contains__(MODEL_NAME_PLACEHOLDER):
            model_dir_path = str(model_dir_path).replace(MODEL_NAME_PLACEHOLDER, model_name)
            model_dir_path = Path(model_dir_path)
        # Create dir / file
        if model_dir_path.exists():
            logger.warning(f"Model directory / file already exists: {model_dir_path}")
            is_exist = True
            # continue
        if str(model_dir_path).__contains__(".py"):
            model_dir_path.touch(exist_ok=True)
            if not is_exist: logger.info(f"Creating model file: {model_dir_path}")
            is_dir = False
        else:
            model_dir_path.mkdir(exist_ok=True)
            if not is_exist: logger.info(f"Creating model directory: {model_dir_path}")
        
        if is_dir:
            create_model_dir(model_name, model_subdict, model_dir_path)


if __name__ == "__main__":
    import argparse 
    parser = argparse.ArgumentParser()

    parser.add_argument("model_name", type=str, help="Model name")
    parser.add_argument("--parent_dir", type=str, default=DEV_MODEL_DIR, help="Parent directory")

    args = parser.parse_args()

    logger.info(f"Creating model directory for model `{args.model_name}` in parent directory `{args.parent_dir}`")
    create_model_dir(
        model_name = args.model_name, 
        model_struct = MODEL_STRUCTURE, 
        parent_dir = args.parent_dir
    )

