from loguru import logger

import ttnn
from models.utility_functions import (
    comp_pcc,
    comp_allclose,
    skip_for_grayskull,
)
from dev.models.resnet.tt.custom_preprocessing import custom_preprocessor


from dev.models.resnet.reference.torch_resnet50 import TorchResnet50
from dev.models.resnet.tt.resnet50 import TTResnet50
from dev.models import ModelConfig


# Resnet50 Model Config
RESNET50_MODEL_CONFIG = ModelConfig(
    math_fidelity=ttnn.MathFidelity.HIGH,
    weights_dtype=ttnn.bfloat16,
    activation_dtype=ttnn.bfloat16,
)


@skip_for_grayskull("Requires wormhole_b0 to run")
def test_resnet50_hr():
    logger.info("Testing the TorchResnet50 and TTResnet50 models")
    # Load the TorchResnet50 model
    torch_resnet50 = TorchResnet50()
    # Load the TTResnet50 model
    tt_resnet50 = TTResnet50()

    logger.debug(torch_resnet50.model_name)


if __name__ == "__main__":
    test_resnet50_hr()
