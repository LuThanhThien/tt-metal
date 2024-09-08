# SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
import torch
import ttnn
from models.utility_functions import (
    is_wormhole_b0,
)
from models.utility_functions import run_for_wormhole_b0, run_for_grayskull
from models.demos.ttnn_resnet.tests.ttnn_resnet_test_infra import create_test_infra

try:
    from tracy import signpost

    use_signpost = True
except ModuleNotFoundError:
    use_signpost = False


def run_resnet50_inference(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    if batch_size == 8:
        pytest.skip("Skipping batch size 8 due to memory config issue")
    if is_wormhole_b0() and batch_size == 20:
        pytest.skip("Skipping batch size 20 for Wormhole B0 due to fitting issue")
    test_infra = create_test_infra(
        device,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        dealloc_input=True,
        final_output_mem_config=ttnn.L1_MEMORY_CONFIG,
        model_location_generator=model_location_generator,
    )
    tt_inputs_host, input_mem_config = test_infra.setup_l1_sharded_input(device)

    # First run configures convs JIT
    test_infra.input_tensor = tt_inputs_host.to(device, input_mem_config)
    test_infra.run()
    # Optimized run
    test_infra.input_tensor = tt_inputs_host.to(device, input_mem_config)
    test_infra.run()
    # More optimized run with caching
    if use_signpost:
        signpost(header="start")
    test_infra.input_tensor = tt_inputs_host.to(device, input_mem_config)
    test_infra.run()
    if use_signpost:
        signpost(header="stop")
    test_infra.validate()


def run_resnet50_trace_inference(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    if batch_size == 8:
        pytest.skip("Skipping batch size 8 due to memory config issue")
    if is_wormhole_b0() and batch_size == 20:
        pytest.skip("Skipping batch size 20 for Wormhole B0 due to fitting issue")

    device.enable_async(enable_async)

    test_infra = create_test_infra(
        device,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        dealloc_input=True,
        final_output_mem_config=ttnn.DRAM_MEMORY_CONFIG,
        model_location_generator=model_location_generator,
    )
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = test_infra.setup_dram_sharded_input(device)
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)

    # First run configures convs JIT
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 0)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    test_infra.run()

    # Optimized run
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 0)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    test_infra.run()

    # Capture
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 0)
    tid = ttnn.begin_trace_capture(device, cq_id=0)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    test_infra.run()
    ttnn.end_trace_capture(device, tid, cq_id=0)

    # More optimized run with caching
    if use_signpost:
        signpost(header="start")
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 0)
    ttnn.execute_trace(device, tid, cq_id=0, blocking=True)
    if use_signpost:
        signpost(header="stop")
    test_infra.validate()

    device.enable_async(False)


def run_resnet50_2cqs_inference(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    if batch_size == 8:
        pytest.skip("Skipping batch size 8 due to memory config issue")
    if is_wormhole_b0() and batch_size == 20:
        pytest.skip("Skipping batch size 20 for Wormhole B0 due to fitting issue")
    test_infra = create_test_infra(
        device,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        dealloc_input=True,
        final_output_mem_config=ttnn.L1_MEMORY_CONFIG,
        model_location_generator=model_location_generator,
    )
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = test_infra.setup_dram_sharded_input(device)
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)
    op_event = ttnn.create_event(device)
    write_event = ttnn.create_event(device)
    # Initialize the op event so we can write
    ttnn.record_event(0, op_event)

    # First run configures convs JIT
    ttnn.wait_for_event(1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(1, write_event)
    ttnn.wait_for_event(0, write_event)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(0, op_event)
    test_infra.run()
    test_infra.validate()

    # Optimized run
    ttnn.wait_for_event(1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(1, write_event)
    ttnn.wait_for_event(0, write_event)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(0, op_event)
    test_infra.run()
    test_infra.validate()

    # More optimized run with caching
    if use_signpost:
        signpost(header="start")
    outputs = []
    for iter in range(0, 2):
        ttnn.wait_for_event(1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(1, write_event)
        ttnn.wait_for_event(0, write_event)
        test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
        ttnn.record_event(0, op_event)
        outputs.append(ttnn.from_device(test_infra.run(), blocking=False))
    ttnn.synchronize_device(device)
    if use_signpost:
        signpost(header="stop")
    for output in outputs:
        test_infra.validate(output)


def run_resnet50_trace_2cqs_inference(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    if batch_size == 8:
        pytest.skip("Skipping batch size 8 due to memory config issue")
    if is_wormhole_b0() and batch_size == 20:
        pytest.skip("Skipping batch size 20 for Wormhole B0 due to fitting issue")

    device.enable_async(enable_async)

    test_infra = create_test_infra(
        device,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        dealloc_input=True,
        final_output_mem_config=ttnn.DRAM_MEMORY_CONFIG,
        model_location_generator=model_location_generator,
    )
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = test_infra.setup_dram_sharded_input(device)
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)
    op_event = ttnn.create_event(device)
    write_event = ttnn.create_event(device)
    # Initialize the op event so we can write
    ttnn.record_event(0, op_event)

    # First run configures convs JIT
    ttnn.wait_for_event(1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(1, write_event)
    ttnn.wait_for_event(0, write_event)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    shape = test_infra.input_tensor.shape
    dtype = test_infra.input_tensor.dtype
    layout = test_infra.input_tensor.layout
    ttnn.record_event(0, op_event)
    test_infra.run()
    test_infra.validate()

    # Optimized run
    ttnn.wait_for_event(1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(1, write_event)
    ttnn.wait_for_event(0, write_event)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    first_out_addr = test_infra.input_tensor.buffer_address()
    ttnn.record_event(0, op_event)
    test_infra.run()
    test_infra.validate()

    # Capture
    ttnn.wait_for_event(1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(1, write_event)
    ttnn.wait_for_event(0, write_event)
    test_infra.input_tensor = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(0, op_event)
    tid = ttnn.begin_trace_capture(device, cq_id=0)
    test_infra.run()
    input_tensor = ttnn.allocate_tensor_on_device(
        shape,
        dtype,
        layout,
        device,
        input_mem_config,
    )
    ttnn.end_trace_capture(device, tid, cq_id=0)
    assert first_out_addr == input_tensor.buffer_address()
    test_infra.validate()

    # More optimized run with caching
    if use_signpost:
        signpost(header="start")
    outputs = []
    for iter in range(0, 2):
        ttnn.wait_for_event(1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(1, write_event)
        ttnn.wait_for_event(0, write_event)
        # TODO: Add in place support to ttnn to_memory_config
        input_tensor = ttnn.reshard(tt_image_res, input_mem_config, input_tensor)
        ttnn.record_event(0, op_event)
        ttnn.execute_trace(device, tid, cq_id=0, blocking=False)
        outputs.append(ttnn.from_device(test_infra.output_tensor, blocking=False))

    ttnn.synchronize_device(device)
    if use_signpost:
        signpost(header="stop")
    for output in outputs:
        test_infra.validate(output)

    device.enable_async(False)


@run_for_grayskull()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 32768}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((20, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
def test_run_resnet50_inference_gs(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    run_resnet50_inference(
        device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
    )


@run_for_grayskull()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 32768, "trace_region_size": 1332224}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((20, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
@pytest.mark.parametrize("enable_async", [True, False])
def test_run_resnet50_trace_inference_gs(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    run_resnet50_trace_inference(
        device,
        use_program_cache,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        enable_async,
        model_location_generator,
    )


@run_for_grayskull()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 32768, "num_hw_cqs": 2}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((20, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
def test_run_resnet50_2cqs_inference_gs(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    run_resnet50_2cqs_inference(
        device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
    )


@run_for_grayskull()
@pytest.mark.parametrize(
    "device_params", [{"l1_small_size": 32768, "trace_region_size": 1332224, "num_hw_cqs": 2}], indirect=True
)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((20, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
@pytest.mark.parametrize("enable_async", [True, False])
def test_run_resnet50_trace_2cqs_inference_gs(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    run_resnet50_trace_2cqs_inference(
        device,
        use_program_cache,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        enable_async,
        model_location_generator,
    )


@run_for_wormhole_b0()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 24576}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((16, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
def test_run_resnet50_inference_wh(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    run_resnet50_inference(
        device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
    )


@run_for_wormhole_b0()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 24576, "trace_region_size": 800768}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((16, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
@pytest.mark.parametrize("enable_async", [True, False])
def test_run_resnet50_trace_inference_wh(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    run_resnet50_trace_inference(
        device,
        use_program_cache,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        enable_async,
        model_location_generator,
    )


@run_for_wormhole_b0()
@pytest.mark.parametrize("device_params", [{"l1_small_size": 24576, "num_hw_cqs": 2}], indirect=True)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((16, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
def test_run_resnet50_2cqs_inference_wh(
    device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
):
    run_resnet50_2cqs_inference(
        device, use_program_cache, batch_size, act_dtype, weight_dtype, math_fidelity, model_location_generator
    )


@run_for_wormhole_b0()
@pytest.mark.parametrize(
    "device_params", [{"l1_small_size": 24576, "trace_region_size": 800768, "num_hw_cqs": 2}], indirect=True
)
@pytest.mark.parametrize(
    "batch_size, act_dtype, weight_dtype, math_fidelity",
    ((16, ttnn.bfloat8_b, ttnn.bfloat8_b, ttnn.MathFidelity.LoFi),),
)
@pytest.mark.parametrize("enable_async", [True, False])
def test_run_resnet50_trace_2cqs_inference_wh(
    device,
    use_program_cache,
    batch_size,
    act_dtype,
    weight_dtype,
    math_fidelity,
    enable_async,
    model_location_generator,
):
    run_resnet50_trace_2cqs_inference(
        device,
        use_program_cache,
        batch_size,
        act_dtype,
        weight_dtype,
        math_fidelity,
        enable_async,
        model_location_generator,
    )
