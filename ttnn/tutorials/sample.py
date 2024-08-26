import argparse
import torch
import ttnn
from loguru import logger

# Argument parsing for H and W
parser = argparse.ArgumentParser(description="Process tensor dimensions.")
parser.add_argument('--H', type=int, default=32, help='Height of the tensor')
parser.add_argument('--W', type=int, default=32, help='Width of the tensor')
args = parser.parse_args()

H = args.H
W = args.W

# Disable summarization for printing torch.Tensor, showing all elements
torch.set_printoptions(threshold=torch.inf)
ttnn.set_printoptions(profile=ttnn.FULL)

# Initialize the tensor with specified dimensions
torch_tensor = torch.zeros(W, H, dtype=torch.int32)
for i in range(W):
    for j in range(H):
        torch_tensor[i][j] = i * H + j

logger.info(torch_tensor)
ttnn_tensor = ttnn.from_torch(torch_tensor)
logger.info(f"Original values:\n{ttnn_tensor}")

# Convert the tensor to the specified layout
layout = ttnn.TILE_LAYOUT  
ttnn_tensor = ttnn.to_layout(ttnn_tensor, layout)

# Extract a slice from the tensor
ttnn_slice = 1
logger.info(f"ttnn_tensor[{ttnn_slice}]:\n{ttnn_tensor}")
