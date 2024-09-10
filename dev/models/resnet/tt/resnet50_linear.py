from torch import nn
import ttnn

from dev.models import ModelConfig


class TTResnet50Linear(nn.Module):
    hardcoded_matmul_config_linear = {
        8: ttnn.MatmulMultiCoreReuseMultiCast1DProgramConfig(
            compute_with_storage_grid_size=(8, 4),
            in0_block_w=2,
            out_subblock_h=1,
            out_subblock_w=1,
            per_core_M=1,
            per_core_N=1,
            fuse_batch=True,
            fused_activation=None,
            mcast_in0=True,
        ),
        16: ttnn.MatmulMultiCoreReuseMultiCast1DProgramConfig(
            compute_with_storage_grid_size=(8, 4),
            in0_block_w=2,
            out_subblock_h=1,
            out_subblock_w=1,
            per_core_M=1,
            per_core_N=1,
            fuse_batch=True,
            fused_activation=None,
            mcast_in0=True,
        ),
        20: ttnn.MatmulMultiCoreReuseMultiCast1DProgramConfig(
            compute_with_storage_grid_size=(8, 4),
            in0_block_w=2,
            out_subblock_h=1,
            out_subblock_w=1,
            per_core_M=1,
            per_core_N=1,
            fuse_batch=True,
            fused_activation=None,
            mcast_in0=True,
        ),
    }

    def __init__(
        self,
        in_features: int,
        out_features: int,
        weight: ttnn.Tensor,  # type: ignore
        bias: ttnn.Tensor,  # type: ignore
        output_mem_config: ttnn.MemoryConfig,  # type: ignore
        model_config: ModelConfig,
        device: str,
        batch_size: int,
        compute_kernel_config: ttnn.DeviceComputeKernelConfig,  # type: ignore
    ):
        self.matmul_config = TTResnet50Linear.hardcoded_matmul_config_linear[batch_size]
        self.weight_shape = weight.get_legacy_shape()
        self.weight = weight.reshape(1, 1, self.weight_shape[-2], self.weight_shape[-1])
        self.bias_shape = bias.get_legacy_shape()
        self.bias = bias.reshape(1, 1, self.bias_shape[-2], self.bias_shape[-1])
        self.output_mem_config = output_mem_config
        self.model_config = model_config
        self.device = device
        self.batch_size = batch_size
        self.compute_kernel_config = compute_kernel_config

    def forward(self, x):
        x = ttnn.linear(
            x,
            self.weight,
            bias=self.bias,
            program_config=self.matmul_config,
            memory_config=self.output_mem_config,
            dtype=self.model_config.activations_dtype,
            compute_kernel_config=self.compute_kernel_config,
        )
        return x
