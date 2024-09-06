# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import torch
import timm
import pytest
from loguru import logger
from dev.py import *
from PIL import Image

from models.experimental.inceptionV4.reference.inception import InceptionV4
from models.utility_functions import (
    comp_pcc,
)

_batch_size = 1


@pytest.mark.parametrize("fuse_ops", [False, True], ids=["Not Fused", "Ops Fused"])
def test_inception_inference(fuse_ops, imagenet_sample_input):
    image = imagenet_sample_input
    batch_size = _batch_size

    with torch.no_grad():
        torch_model = timm.create_model("inception_v4", pretrained=True)
        torch_model.eval()

        torch_output = torch_model(image)

        tt_model = InceptionV4(state_dict=torch_model.state_dict())
        tt_model.eval()

        tt_output : torch.Tensor = tt_model(image)
        passing = comp_pcc(torch_output, tt_output)

        logger.info(f"PCC passing: {passing}")
        logger.debug(f"Output TT \n{tt_output}")
        save_dir = Path(DEV_GEN_DIR, "models", "inceptionV4")
        save_dir.mkdir(parents=True, exist_ok=True)
        # save output
        logger.info(f"tt_output type: {type(tt_output)}")
        logger.info(f"tt_output shape: {tt_output.shape}")
        logger.info(f"Max value in tt_output: {tt_output.max()}")
        logger.info(f"Max value index in tt_output: {tt_output.argmax()}")

        assert passing[0], passing[1:]

    logger.info(f"PASSED {passing[1]}")
