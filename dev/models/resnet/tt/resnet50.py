import torch
from torch import nn
import ttnn
from models.utility_functions import (
    is_grayskull,
    is_wormhole_b0,
    pad_and_fold_conv_activation_for_unity_stride,
)
from ttnn.model_preprocessing import (
    preprocess_model_parameters,
)

from dev.models import ModelConfig
from dev.models.resnet.tt.resnet50_bottleneck import TTResnet50Bottleneck
from dev.models.resnet.tt.resnet50_linear import TTResnet50Linear


class TTResnet50(nn.Module):
    def __init__(
        self,
        device: str,
        parameters: dict,
        batch_size: int,
        model_config: ModelConfig,
        preprocessor: callable = None,
        postprocessor: callable = None,
        output_memory_config: ttnn.MemoryConfig = ttnn.L1_MEMORY_CONFIG,  # type: ignore
        model_name: str = "tt_resnet50",
        **kwargs,
    ):
        super(TTResnet50, self).__init__()

        # MODEL CONFIG
        self.device = device
        self.batch_size = batch_size
        self.model_config = model_config
        self.preprocessor = preprocessor
        self.postprocessor = postprocessor
        self.model_name = model_name

        # MODEL PARAMS
        self.num_classes = 1000

        # OTHER ARGS
        self.output_memory_config = output_memory_config
        self.kwargs = kwargs

        # LAYERS
        ## 1. First conv1
        self.conv_op_cache = {}
        self.conv1_weight_tensor = parameters.conv1.weight
        self.conv1_bias_tensor = parameters.conv1.bias
        self.conv1_input_channels = self.conv1_weight_tensor.shape[1]
        self.conv1_output_channels = self.conv1_weight_tensor.shape[0]
        assert self.conv1_weight_tensor.shape[2] == 4
        self.transpose_shards = True
        conv_config_params = self._get_conv1_config_params()
        self.conv1_config = ttnn.Conv2dConfig(**conv_config_params)
        self.conv1_kernel_size = (4, 4)  # **
        self.conv1_stride = (1, 1)  # Aldready stride in preprocessing
        self.conv1_padding = (0, 0)  # Aldready padding in preprocessing
        self.conv1_input_height = 115  # ** reasons for this hardcoded value
        self.conv1_input_width = 115  # ** reasons for this hardcoded value

        ## 2. Maxpooling
        self.max_pool_kernel_size = (3, 3)
        self.max_pool_stride = (2, 2)
        self.max_pool_padding = (1, 1)
        self.max_pool_dilation = (1, 1)

        ## 3. Resnet50 bottleneck blocks
        self.bottleneck_inplanes = 64
        self.bottleneck_layers = [3, 4, 6, 3]
        self.bottleneck_planes = [64, 128, 256, 512]
        self.bottleneck_strides = [1, 2, 2, 2]
        self.layer1 = self._make_botleneck_blocks(
            parameters=parameters.layer1,
            planes=self.bottleneck_planes[0],
            num_blocks=self.bottleneck_layers[0],
            stride=self.bottleneck_strides[0],
            model_config=model_config,
        )
        self.layer2 = self._make_botleneck_blocks(
            parameters=parameters.layer2,
            planes=self.bottleneck_planes[1],
            num_blocks=self.bottleneck_layers[1],
            stride=self.bottleneck_strides[1],
            model_config=model_config,
        )
        self.layer3 = self._make_botleneck_blocks(
            parameters=parameters.layer3,
            planes=self.bottleneck_planes[2],
            num_blocks=self.bottleneck_layers[2],
            stride=self.bottleneck_strides[2],
            model_config=model_config,
        )
        self.layer4 = self._make_botleneck_blocks(
            parameters=parameters.layer4,
            planes=self.bottleneck_planes[3],
            num_blocks=self.bottleneck_layers[3],
            stride=self.bottleneck_strides[3],
            model_config=model_config,
        )

        ## 4. Average pooling
        self.avg_pool = ttnn.global_avg_pool2d

        ## 5. Resnet50 linear
        if is_grayskull():
            linear_compute_kernel_config = ttnn.GrayskullComputeKernelConfig(
                math_fidelity=model_config.math_fidelity,
                math_approx_mode=True,
            )
        else:
            linear_compute_kernel_config = ttnn.WormholeComputeKernelConfig(
                math_fidelity=model_config.math_fidelity,
                math_approx_mode=True,
                fp32_dest_acc_en=False,
                packer_l1_acc=True,
            )
        self.fc = TTResnet50Linear(
            in_features=512 * TTResnet50Bottleneck.expansion,
            out_features=1024,
            weight=ttnn.to_device(parameters.fc.weight, device),
            bias=ttnn.to_device(parameters.fc.bias, device),
            output_mem_config=ttnn.L1_WIDTH_SHARDED_MEMORY_CONFIG,
            model_config=model_config,
            device=self.device,
            batch_size=batch_size,
            compute_kernel_config=linear_compute_kernel_config,
        )  # num_classes = 1000

    def __del__(self):
        # Need to clear global configs for each Resnet run
        self.conv_op_cache.clear()

    def _get_conv1_config_params(self):
        whb0_and_b16 = is_wormhole_b0() and self.batch_size == 16
        if is_wormhole_b0():
            self.transpose_shards = False
            if self.batch_size == 16:
                act_block_h_override = 1568
            elif self.batch_size == 20:
                act_block_h_override = 640
        elif whb0_and_b16:
            act_block_h_override = 256
        else:
            act_block_h_override = 0
        if not is_wormhole_b0():
            input_channels_alignment = 16
        elif whb0_and_b16:
            input_channels_alignment = 16
        else:
            input_channels_alignment = 32
        packer_l1_accum_enabled = (True if whb0_and_b16 else False,)
        enable_act_double_buffer = (True if whb0_and_b16 else False,)
        enable_split_reader = (True if whb0_and_b16 else False,)

        return dict(
            {
                "dtype": self.model_config.activations_dtype,
                "weights_dtype": self.model_config.weights_dtype,
                "math_fidelity": self.model_config.math_fidelity,
                "activation": "relu",
                "input_channels_alignment": input_channels_alignment,
                "act_block_h_override": act_block_h_override,
                "packer_l1_accum_enabled": packer_l1_accum_enabled,
                "enable_act_double_buffer": enable_act_double_buffer,
                "enable_split_reader": enable_split_reader,
                "dealloc_input": True,
                "enable_subblock_padding": False,
            }
        )

    def _make_botleneck_blocks(
        self, parameters: dict, planes: int, num_blocks: int, stride: int, model_config: ModelConfig
    ) -> list[TTResnet50Bottleneck]:
        bottleneck_blocks = []
        downsample = stride != 1 or self.bottleneck_inplanes != planes * TTResnet50Bottleneck.expansion
        bottleneck_blocks.append(
            TTResnet50Bottleneck(
                parameters=parameters[0],
                downsample=downsample,
                stride=stride,
                model_config=model_config,
            )
        )
        self.bottleneck_inplanes = planes * TTResnet50Bottleneck.expansion
        for i in range(1, num_blocks):
            bottleneck_blocks.append(
                TTResnet50Bottleneck(
                    parameters=parameters[i],
                    downsample=False,
                    stride=1,
                    model_config=model_config,
                )
            )
        return bottleneck_blocks

    def preprocess(self, torch_input_tensor, inputs_mesh_mapper=None) -> ttnn.Tensor:  # type: ignore
        resnet50_first_conv_kernel_size = 3
        resnet50_first_conv_stride = 2
        input_tensor = pad_and_fold_conv_activation_for_unity_stride(
            torch_input_tensor,
            resnet50_first_conv_kernel_size,
            resnet50_first_conv_kernel_size,
            resnet50_first_conv_stride,
            resnet50_first_conv_stride,
        )
        print("input_tensor.shape after pad_and_fold_conv_activation_for_unity_stride: ", input_tensor.shape)
        input_tensor = torch.permute(input_tensor, (0, 2, 3, 1))
        input_tensor = ttnn.from_torch(input_tensor, dtype=ttnn.bfloat16, mesh_mapper=inputs_mesh_mapper)
        return input_tensor

    def postprocess(self, x, *args, **kwargs) -> ttnn.Tensor:  # type: ignore
        return self.postprocessor(x, *args, **kwargs) if self.postprocessor else x

    def forward(self, x) -> ttnn.Tensor:  # type: ignore
        return x
