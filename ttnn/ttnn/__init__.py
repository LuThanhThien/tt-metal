# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import contextlib
import json
import importlib
import os
import pathlib
from types import ModuleType

from loguru import logger

# Sets env and updates shared libs rpath
# This is a tweak required for a proper wheel functioning
import ttnn.library_tweaks

library_tweaks.setup_ttnn_so()

import ttnn._ttnn


Config = ttnn._ttnn.core.Config
CONFIG = ttnn._ttnn.CONFIG
CONFIG_PATH = None
if "TTNN_CONFIG_PATH" in os.environ:
    CONFIG_PATH = pathlib.Path(os.environ["TTNN_CONFIG_PATH"])

CONFIG_OVERRIDES = os.environ.get("TTNN_CONFIG_OVERRIDES", None)


def load_config_from_dictionary(config, from_file=False):
    global CONFIG
    for key, value in config.items():
        if hasattr(CONFIG, key):
            if getattr(CONFIG, key) is not None:
                value = type(getattr(CONFIG, key))(value)
            setattr(CONFIG, key, value)
        elif from_file:
            logger.warning(
                f"Unknown configuration key: {key}. Please update your configuration file: {CONFIG_PATH}. Or delete it to get the new default config"
            )
        else:
            raise ValueError(f"Unknown configuration key: {key}")


def load_config_from_json_file(json_path):
    global CONFIG
    try:
        with open(json_path, "r") as f:
            config = json.load(f)
        load_config_from_dictionary(config, from_file=True)
    except Exception as e:
        logger.warning(f"Failed to load ttnn configuration from {json_path}: {e}")


def save_config_to_json_file(json_path):
    with open(json_path, "w") as f:
        normalized_config = {}
        for key in dir(CONFIG):
            if "__" in key:
                continue
            value = getattr(CONFIG, key)
            if isinstance(value, pathlib.Path):
                value = str(value)
            normalized_config[key] = value
        json.dump(normalized_config, f, indent=4)


if CONFIG_PATH is not None:
    if CONFIG_PATH.exists():
        logger.debug(f"Loading ttnn configuration from {CONFIG_PATH}")
        load_config_from_json_file(CONFIG_PATH)
    else:
        CONFIG_PATH.parent.mkdir(parents=True, exist_ok=True)
        save_config_to_json_file(CONFIG_PATH)

if CONFIG_OVERRIDES is not None:
    logger.debug(f"Loading ttnn configuration overrides from environment variable TTNN_CONFIG_OVERRIDES")
    load_config_from_dictionary(json.loads(CONFIG_OVERRIDES))

logger.debug(f"Initial ttnn.CONFIG:\n{CONFIG}")


@contextlib.contextmanager
def manage_config(name, value):
    global CONFIG
    original_value = getattr(CONFIG, name)
    setattr(CONFIG, name, value)
    logger.debug(f"Set ttnn.CONFIG.{name} to {value}")
    yield
    setattr(CONFIG, name, original_value)
    logger.debug(f"Restored ttnn.CONFIG.{name} to {original_value}")


from ttnn._ttnn.multi_device import get_device_tensor, get_device_tensors, aggregate_as_tensor

from ttnn.types import (
    TILE_SIZE,
    DataType,
    uint8,
    uint16,
    int32,
    uint32,
    bfloat8_b,
    bfloat4_b,
    bfloat16,
    float32,
    MathFidelity,
    MemoryConfig,
    BufferType,
    TensorMemoryLayout,
    DRAM_MEMORY_CONFIG,
    L1_MEMORY_CONFIG,
    L1_BLOCK_SHARDED_MEMORY_CONFIG,
    L1_HEIGHT_SHARDED_MEMORY_CONFIG,
    L1_WIDTH_SHARDED_MEMORY_CONFIG,
    ShardStrategy,
    ShardOrientation,
    ShardSpec,
    CoreRangeSet,
    CoreRange,
    CoreCoord,
    Layout,
    ROW_MAJOR_LAYOUT,
    TILE_LAYOUT,
    StorageType,
    DEVICE_STORAGE_TYPE,
    CoreGrid,
    CoreRange,
    Shape,
    Tensor,
    DeviceComputeKernelConfig,
    WormholeComputeKernelConfig,
    GrayskullComputeKernelConfig,
    DeviceGrid,
    UnaryWithParam,
    UnaryOpType,
    # Lu Thien 2024.08.21 TensorPrintProfile
    PrintOptions,
)

from ttnn.device import (
    Device,
    open_device,
    close_device,
    enable_program_cache,
    disable_and_clear_program_cache,
    manage_device,
    synchronize_device,
    dump_device_memory_state,
)

from ttnn.multi_device import (
    DeviceMesh,
    open_device_mesh,
    close_device_mesh,
    get_num_pcie_devices,
    get_num_devices,
    get_pcie_device_ids,
    get_device_ids,
    create_device_mesh,
    synchronize_devices,
    TensorToMesh,
    ShardTensorToMesh,
    ShardTensor2dMesh,
    ReplicateTensorToMesh,
    MeshToTensor,
    ConcatMeshToTensor,
    ListMeshToTensor,
    visualize_device_mesh,
    ConcatMesh2dToTensor,
)

from ttnn.core import (
    set_printoptions,
    has_storage_type_of,
    is_tensor_storage_on_device,
    has_tile_padding,
    is_sharded,
    get_memory_config,
    create_sharded_memory_config,
    create_sharded_memory_config_,
    dump_memory_config,
    load_memory_config,
    dump_stack_trace_on_segfault,
)

import ttnn.reflection
import ttnn.database


begin_trace_capture = ttnn._ttnn.operations.core.begin_trace_capture
end_trace_capture = ttnn._ttnn.operations.core.end_trace_capture
execute_trace = ttnn._ttnn.operations.core.execute_trace
release_trace = ttnn._ttnn.operations.core.release_trace


from ttnn.decorators import (
    log_runtime,
    register_python_operation,
    register_cpp_operation,
    attach_golden_function,
    query_registered_operations,
    dump_operations,
    register_pre_operation_hook,
    register_post_operation_hook,
    get_golden_function,
    get_fallback_function,
)


def auto_register_ttnn_cpp_operations(module):
    for attribute_name in dir(module):
        attribute = getattr(module, attribute_name)
        if hasattr(attribute, "__ttnn_operation__") and attribute.__ttnn_operation__ is None:
            setattr(module, attribute_name, ttnn.register_cpp_operation()(attribute))
        elif isinstance(attribute, ModuleType):
            auto_register_ttnn_cpp_operations(attribute)


auto_register_ttnn_cpp_operations(ttnn._ttnn)

import ttnn.experimental_loader
import ttnn.experimental_loader.golden_functions

import ttnn.operations

sub = ttnn.subtract
sub_ = ttnn.subtract_
mul = ttnn.multiply
mul_ = ttnn.multiply_


def prelu(*args, **kwargs):  # Alias for leaky_relu. TODO(#8544): implement PReLU properly
    return ttnn.leaky_relu(*args, **kwargs)


# TODO: pybind the overloaded operators below
ttnn.Tensor.__add__ = lambda self, *args, **kwargs: ttnn.add(self, *args, **kwargs)
ttnn.Tensor.__radd__ = lambda self, *args, **kwargs: ttnn.add(self, *args, **kwargs)
ttnn.Tensor.__sub__ = lambda self, *args, **kwargs: ttnn.subtract(self, *args, **kwargs)
ttnn.Tensor.__mul__ = lambda self, *args, **kwargs: ttnn.multiply(self, *args, **kwargs)
ttnn.Tensor.__rmul__ = lambda self, *args, **kwargs: ttnn.multiply(self, *args, **kwargs)
ttnn.Tensor.__eq__ = lambda self, *args, **kwargs: ttnn.eq(self, *args, **kwargs)
ttnn.Tensor.__ne__ = lambda self, *args, **kwargs: ttnn.ne(self, *args, **kwargs)
ttnn.Tensor.__gt__ = lambda self, *args, **kwargs: ttnn.gt(self, *args, **kwargs)
ttnn.Tensor.__ge__ = lambda self, *args, **kwargs: ttnn.ge(self, *args, **kwargs)
ttnn.Tensor.__lt__ = lambda self, *args, **kwargs: ttnn.lt(self, *args, **kwargs)
ttnn.Tensor.__le__ = lambda self, *args, **kwargs: ttnn.le(self, *args, **kwargs)
ttnn.Tensor.__getitem__ = lambda self, *args, **kwargs: ttnn.operations.core.__getitem__(self, *args, **kwargs)

from ttnn.operations.matmul import (
    MatmulMultiCoreReuseProgramConfig,
    MatmulMultiCoreReuseMultiCastProgramConfig,
    MatmulMultiCoreReuseMultiCast1DProgramConfig,
    MatmulMultiCoreReuseMultiCastDRAMShardedProgramConfig,
)

from ttnn.operations.normalization import (
    SoftmaxProgramConfig,
    SoftmaxDefaultProgramConfig,
    SoftmaxShardedMultiCoreProgramConfig,
    LayerNormDefaultProgramConfig,
    LayerNormShardedMultiCoreProgramConfig,
    create_group_norm_weight_bias_rm,
    create_group_norm_input_mask,
    determine_expected_group_norm_sharded_config_and_grid_size,
)

from ttnn.operations.embedding import (
    EmbeddingsType,
)

from ttnn.operations.losses import (
    LossReductionMode,
)

from ttnn.operations.conv2d import Conv2d, Conv2dConfig, get_conv_output_dim, get_conv_padded_input_shape_and_mem_config
from ttnn.operations.pool import TTPyMaxPool, max_pool2d, max_pool2d_legacy, MaxPool2d, global_avg_pool2d, avg_pool2d
from ttnn.operations.conv1d import Conv1d, Conv1dConfig


if importlib.util.find_spec("torch") is not None:
    import ttnn.tracer
