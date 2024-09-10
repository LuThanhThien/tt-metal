from models.utility_functions import pad_and_fold_conv_activation_for_unity_stride
import torch
import ttnn
from dev.common import logger


def main():
    input_shape = (1, 1, 256, 256)
    padding_h = padding_w = 1
    stride_h = stride_w = 2

    logger.info(f"Input shape: {input_shape}")
    logger.info(f"Padding height x width: {padding_h} x {padding_w}")
    logger.info(f"Stride height x width: {stride_h} x {stride_w}")

    input_tensor = torch.arange(1, input_shape[2] * input_shape[3] + 1).reshape(input_shape).float()
    # logger.debug(f"Input tensor: \n{input_tensor}")
    logger.info(f"Input tensor shape: {input_tensor.shape}")

    pad_fold_input_tensor = pad_and_fold_conv_activation_for_unity_stride(
        input_tensor, padding_h, padding_w, stride_h, stride_w
    )

    # logger.debug(f"Pad and fold input tensor: \n{pad_fold_input_tensor}")
    logger.info(f"Pad and fold input tensor shape: {pad_fold_input_tensor.shape}")

    input_tensor = torch.permute(pad_fold_input_tensor, (0, 2, 3, 1))
    input_tensor = ttnn.from_torch(
        input_tensor,
        dtype=ttnn.bfloat16,
    )

    ttnn.set_printoptions(profile=ttnn.PrintOptions.FULL)

    # logger.debug(f"Input tensor after preprocessing: \n{input_tensor}")
    logger.info(f"Input tensor shape after preprocessing: {input_tensor.shape}")


if __name__ == "__main__":
    main()
