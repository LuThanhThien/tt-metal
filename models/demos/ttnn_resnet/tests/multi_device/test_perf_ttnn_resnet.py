# SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import torch
from loguru import logger
from transformers import AutoImageProcessor
import pytest
import ttnn
import tt_lib
from ttnn.model_preprocessing import (
    preprocess_model_parameters,
)

from models.utility_functions import (
    profiler,
    disable_persistent_kernel_cache,
    run_for_wormhole_b0,
)

from models.perf.perf_utils import prep_perf_report

from models.demos.ttnn_resnet.tests.multi_device.test_ttnn_resnet50_performant import (
    setup_l1_sharded_input,
    setup_dram_sharded_input,
)
from models.demos.ttnn_resnet.tests.ttnn_resnet_test_infra import load_resnet50_model
from models.demos.ttnn_resnet.tt.custom_preprocessing import create_custom_mesh_preprocessor
from models.demos.ttnn_resnet.tt.ttnn_functional_resnet50_new_conv_api import resnet50

try:
    from tracy import signpost

    use_signpost = True
except ModuleNotFoundError:
    use_signpost = False


def create_event(device):
    event = []
    if isinstance(device, ttnn.Device):
        event.append(tt_lib.device.CreateEvent())
    else:
        for dev in device.get_device_ids():
            event.append(tt_lib.device.CreateEvent())
    return event


def wait_for_event(device, cq_id, event):
    if isinstance(device, ttnn.Device):
        tt_lib.device.WaitForEvent(device, cq_id, event)
    else:
        for dev, eve in zip(device.get_device_ids(), event):
            tt_lib.device.WaitForEvent(device.get_device(dev), cq_id, eve)


def record_event(device, cq_id, event):
    if isinstance(device, ttnn.Device):
        tt_lib.device.RecordEvent(device, cq_id, event)
    else:
        for dev, eve in zip(device.get_device_ids(), event):
            tt_lib.device.RecordEvent(device.get_device(dev), cq_id, eve)


def buffer_address(tensor):
    addr = []
    for ten in ttnn.get_device_tensors(tensor):
        addr.append(ten.buffer_address())
    return addr


def dump_device_profiler(device):
    if isinstance(device, ttnn.Device):
        tt_lib.device.DumpDeviceProfiler(device)
    else:
        for dev in device.get_device_ids():
            tt_lib.device.DumpDeviceProfiler(device.get_device(dev))


# TODO: Create ttnn apis for these
ttnn.create_event = create_event
ttnn.wait_for_event = wait_for_event
ttnn.record_event = record_event
ttnn.buffer_address = buffer_address
ttnn.dump_device_profiler = dump_device_profiler

model_config = {
    "MATH_FIDELITY": ttnn.MathFidelity.LoFi,
    "WEIGHTS_DTYPE": ttnn.bfloat8_b,
    "ACTIVATIONS_DTYPE": ttnn.bfloat8_b,
}


def run_model(
    device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer, num_warmup_iterations, num_measurement_iterations
):
    ops_parallel_config = {}
    tt_inputs_host, input_mem_config = setup_l1_sharded_input(
        device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer
    )
    profiler.start("compile")
    tt_inputs = tt_inputs_host.to(device, input_mem_config)
    _ = ttnn.from_device(tt_resnet50(tt_inputs, device, ops_parallel_config), blocking=True)
    profiler.end("compile")
    ttnn.dump_device_profiler(device)

    profiler.start("cache")
    tt_inputs = tt_inputs_host.to(device, input_mem_config)
    _ = ttnn.from_device(tt_resnet50(tt_inputs, device, ops_parallel_config), blocking=True)
    profiler.end("cache")
    ttnn.dump_device_profiler(device)

    for iter in range(0, num_warmup_iterations):
        tt_inputs = tt_inputs_host.to(device, input_mem_config)
        _ = ttnn.from_device(tt_resnet50(tt_inputs, device, ops_parallel_config), blocking=True)
        ttnn.dump_device_profiler(device)

    ttnn.synchronize_devices(device)
    if use_signpost:
        signpost(header="start")
    outputs = []
    profiler.start(f"run")
    for iter in range(0, num_measurement_iterations):
        tt_inputs = tt_inputs_host.to(device, input_mem_config)
        outputs.append(ttnn.from_device(tt_resnet50(tt_inputs, device, ops_parallel_config), blocking=False))
    ttnn.synchronize_devices(device)
    profiler.end(f"run")
    if use_signpost:
        signpost(header="stop")
    ttnn.dump_device_profiler(device)


def run_2cq_model(
    device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer, num_warmup_iterations, num_measurement_iterations
):
    ops_parallel_config = {}
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = setup_dram_sharded_input(
        device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer
    )
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)
    op_event = ttnn.create_event(device)
    write_event = ttnn.create_event(device)
    # Initialize the op event so we can write
    ttnn.record_event(device, 0, op_event)

    profiler.start("compile")
    ttnn.wait_for_event(device, 1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(device, 1, write_event)
    ttnn.wait_for_event(device, 0, write_event)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(device, 0, op_event)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("compile")
    ttnn.dump_device_profiler(device)

    profiler.start("cache")
    ttnn.wait_for_event(device, 1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(device, 1, write_event)
    ttnn.wait_for_event(device, 0, write_event)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(device, 0, op_event)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("cache")
    ttnn.dump_device_profiler(device)

    for iter in range(0, num_warmup_iterations):
        ttnn.wait_for_event(device, 1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(device, 1, write_event)
        ttnn.wait_for_event(device, 0, write_event)
        reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
        ttnn.record_event(device, 0, op_event)
        _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
        ttnn.dump_device_profiler(device)

    ttnn.synchronize_devices(device)
    if use_signpost:
        signpost(header="start")
    outputs = []
    profiler.start(f"run")
    for iter in range(0, num_measurement_iterations):
        ttnn.wait_for_event(device, 1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(device, 1, write_event)
        ttnn.wait_for_event(device, 0, write_event)
        reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
        ttnn.record_event(device, 0, op_event)
        outputs.append(ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=False))
    ttnn.synchronize_devices(device)
    profiler.end(f"run")
    if use_signpost:
        signpost(header="stop")
    ttnn.dump_device_profiler(device)


def run_trace_model(
    device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer, num_warmup_iterations, num_measurement_iterations
):
    ops_parallel_config = {}
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = setup_dram_sharded_input(
        device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer
    )
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)
    # Compile
    profiler.start("compile")
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("compile")
    ttnn.dump_device_profiler(device)

    profiler.start("cache")
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("cache")
    ttnn.dump_device_profiler(device)

    # Capture
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res)
    tid = ttnn.begin_trace_capture(device, cq_id=0)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    tt_output_res = tt_resnet50(reshard_out, device, ops_parallel_config)
    ttnn.end_trace_capture(device, tid, cq_id=0)
    ttnn.dump_device_profiler(device)

    for iter in range(0, num_warmup_iterations):
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res)
        ttnn.execute_trace(device, tid, cq_id=0, blocking=False)
        _ = ttnn.from_device(tt_output_res, blocking=True)
        ttnn.dump_device_profiler(device)

    ttnn.synchronize_devices(device)
    if use_signpost:
        signpost(header="start")
    outputs = []
    profiler.start(f"run")
    for iter in range(0, num_measurement_iterations):
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res)
        ttnn.execute_trace(device, tid, cq_id=0, blocking=False)
        outputs.append(ttnn.from_device(tt_output_res, blocking=False))
    ttnn.synchronize_devices(device)
    profiler.end(f"run")
    if use_signpost:
        signpost(header="stop")
    ttnn.dump_device_profiler(device)


def run_trace_2cq_model(
    device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer, num_warmup_iterations, num_measurement_iterations
):
    ops_parallel_config = {}
    tt_inputs_host, sharded_mem_config_DRAM, input_mem_config = setup_dram_sharded_input(
        device, tt_inputs, tt_resnet50, mesh_mapper, mesh_composer
    )
    tt_image_res = tt_inputs_host.to(device, sharded_mem_config_DRAM)

    op_event = ttnn.create_event(device)
    write_event = ttnn.create_event(device)
    # Initialize the op event so we can write
    ttnn.record_event(device, 0, op_event)

    profiler.start("compile")
    ttnn.wait_for_event(device, 1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(device, 1, write_event)
    ttnn.wait_for_event(device, 0, write_event)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(device, 0, op_event)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("compile")
    ttnn.dump_device_profiler(device)

    profiler.start("cache")
    ttnn.wait_for_event(device, 1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(device, 1, write_event)
    ttnn.wait_for_event(device, 0, write_event)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    first_out_addr = ttnn.buffer_address(reshard_out)
    ttnn.record_event(device, 0, op_event)
    _ = ttnn.from_device(tt_resnet50(reshard_out, device, ops_parallel_config), blocking=True)
    profiler.end("cache")
    ttnn.dump_device_profiler(device)

    # Capture
    ttnn.wait_for_event(device, 1, op_event)
    ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
    ttnn.record_event(device, 1, write_event)

    ttnn.wait_for_event(device, 0, write_event)
    reshard_out = ttnn.to_memory_config(tt_image_res, input_mem_config)
    ttnn.record_event(device, 0, op_event)

    tid = ttnn.begin_trace_capture(device, cq_id=0)
    tt_output_res = tt_resnet50(reshard_out, device, ops_parallel_config)
    reshard_out = ttnn.allocate_tensor_on_device(
        reshard_out.shape, reshard_out.dtype, reshard_out.layout, device, input_mem_config
    )
    ttnn.end_trace_capture(device, tid, cq_id=0)
    assert first_out_addr == ttnn.buffer_address(reshard_out)
    ttnn.dump_device_profiler(device)

    for iter in range(0, num_warmup_iterations):
        ttnn.wait_for_event(device, 1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(device, 1, write_event)
        ttnn.wait_for_event(device, 0, write_event)
        reshard_out = ttnn.experimental.tensor.reshard(tt_image_res, input_mem_config, reshard_out)
        ttnn.record_event(device, 0, op_event)
        ttnn.execute_trace(device, tid, cq_id=0, blocking=True)
        ttnn.dump_device_profiler(device)

    ttnn.synchronize_devices(device)
    if use_signpost:
        signpost(header="start")
    outputs = []
    profiler.start(f"run")
    for iter in range(0, num_measurement_iterations):
        ttnn.wait_for_event(device, 1, op_event)
        ttnn.copy_host_to_device_tensor(tt_inputs_host, tt_image_res, 1)
        ttnn.record_event(device, 1, write_event)
        ttnn.wait_for_event(device, 0, write_event)
        # TODO: Add in place support to ttnn to_memory_config
        reshard_out = ttnn.experimental.tensor.reshard(tt_image_res, input_mem_config, reshard_out)
        ttnn.record_event(device, 0, op_event)
        ttnn.execute_trace(device, tid, cq_id=0, blocking=False)
        outputs.append(tt_output_res.cpu(blocking=False))
    ttnn.synchronize_devices(device)
    profiler.end(f"run")
    if use_signpost:
        signpost(header="stop")
    ttnn.dump_device_profiler(device)


def run_perf_resnet(
    device_batch_size,
    expected_inference_time,
    expected_compile_time,
    hf_cat_image_sample_input,
    device,
    model_version,
    model_location_generator,
):
    profiler.clear()
    disable_persistent_kernel_cache()
    if device_batch_size <= 2:
        pytest.skip("Batch size 1 and 2 are not supported with sharded data")
    num_devices = 1 if isinstance(device, ttnn.Device) else device.get_num_devices()
    batch_size = device_batch_size * num_devices
    first_key = f"first_iter_batchsize{batch_size}"
    second_key = f"second_iter_batchsize{batch_size}"
    cpu_key = f"ref_key_batchsize{batch_size}"
    model_name = "microsoft/resnet-50"

    image = hf_cat_image_sample_input
    image_processor = AutoImageProcessor.from_pretrained(model_name)
    inputs = image_processor(image, return_tensors="pt")

    inputs = inputs["pixel_values"].bfloat16()
    comments = f"{list(inputs.shape)[-2]}x{list(inputs.shape)[-1]}_batchsize{batch_size}"

    inputs1 = inputs
    for i in range(batch_size - 1):
        inputs = torch.cat((inputs, inputs1), dim=0)

    inputs_mesh_mapper = ttnn.ShardTensorToMesh(device, dim=0)
    weights_mesh_mapper = ttnn.ReplicateTensorToMesh(device)
    output_mesh_composer = ttnn.ConcatMeshToTensor(device, dim=0)

    torch_resnet50 = load_resnet50_model(model_location_generator)
    torch_resnet50.eval()

    parameters = preprocess_model_parameters(
        initialize_model=lambda: torch_resnet50,
        custom_preprocessor=create_custom_mesh_preprocessor(weights_mesh_mapper),
        device=None,
    )
    torch_resnet50.to(torch.bfloat16)

    tt_resnet50 = resnet50(
        device=device,
        parameters=parameters,
        batch_size=device_batch_size,
        model_config=model_config,
        dealloc_input=True,
        final_output_mem_config=ttnn.DRAM_MEMORY_CONFIG if "trace" in model_version else ttnn.L1_MEMORY_CONFIG,
        mesh_mapper=weights_mesh_mapper,
    )
    ttnn.synchronize_devices(device)

    num_warmup_iterations = 5
    num_measurement_iterations = 15

    with torch.no_grad():
        profiler.start(cpu_key)
        logits = torch_resnet50(inputs)
        profiler.end(cpu_key)

        tt_inputs = tt_resnet50.preprocessing(inputs, inputs_mesh_mapper)
        if "resnet50_trace_2cqs" in model_version:
            run_trace_2cq_model(
                device,
                tt_inputs,
                tt_resnet50,
                inputs_mesh_mapper,
                output_mesh_composer,
                num_warmup_iterations,
                num_measurement_iterations,
            )
        elif "resnet50_2cqs" in model_version:
            run_2cq_model(
                device,
                tt_inputs,
                tt_resnet50,
                inputs_mesh_mapper,
                output_mesh_composer,
                num_warmup_iterations,
                num_measurement_iterations,
            )
        elif "resnet50_trace" in model_version:
            run_trace_model(
                device,
                tt_inputs,
                tt_resnet50,
                inputs_mesh_mapper,
                output_mesh_composer,
                num_warmup_iterations,
                num_measurement_iterations,
            )
        elif "resnet50" in model_version:
            run_model(
                device,
                tt_inputs,
                tt_resnet50,
                inputs_mesh_mapper,
                output_mesh_composer,
                num_warmup_iterations,
                num_measurement_iterations,
            )
        else:
            assert False, f"Model version to run {model_version} not found"

    first_iter_time = profiler.get(f"compile") + profiler.get(f"cache")

    # ensuring inference time fluctuations is not noise
    inference_time_avg = profiler.get("run") / num_measurement_iterations

    cpu_time = profiler.get(cpu_key)
    compile_time = first_iter_time - 2 * inference_time_avg
    prep_perf_report(
        model_name=f"ttnn_{model_version}_batch_size{batch_size}",
        batch_size=batch_size,
        inference_and_compile_time=first_iter_time,
        inference_time=inference_time_avg,
        expected_compile_time=expected_compile_time,
        expected_inference_time=expected_inference_time,
        comments=comments,
        inference_time_cpu=cpu_time,
    )

    logger.info(f"{model_name} {comments} inference time (avg): {inference_time_avg}")
    logger.info(f"{model_name} compile time: {compile_time}")


@run_for_wormhole_b0()
@pytest.mark.model_perf_t3000
@pytest.mark.parametrize("device_params", [{"l1_small_size": 24576}], indirect=True)
@pytest.mark.parametrize(
    "device_batch_size, enable_async_mode, expected_inference_time, expected_compile_time",
    (
        (16, True, 0.0094, 60),
        (16, False, 0.0230, 60),
    ),
    indirect=["enable_async_mode"],
)
def test_perf_t3000(
    device_mesh,
    use_program_cache,
    device_batch_size,
    expected_inference_time,
    expected_compile_time,
    hf_cat_image_sample_input,
    enable_async_mode,
    model_location_generator,
):
    mode = "async" if enable_async_mode else "sync"
    run_perf_resnet(
        device_batch_size,
        expected_inference_time,
        expected_compile_time,
        hf_cat_image_sample_input,
        device_mesh,
        f"resnet50_{mode}",
        model_location_generator,
    )


@run_for_wormhole_b0()
@pytest.mark.model_perf_t3000
@pytest.mark.parametrize("device_params", [{"l1_small_size": 32768, "trace_region_size": 1500000}], indirect=True)
@pytest.mark.parametrize(
    "device_batch_size, enable_async_mode, expected_inference_time, expected_compile_time",
    (
        (16, True, 0.0068, 60),
        (16, False, 0.0111, 60),
    ),
    indirect=["enable_async_mode"],
)
def test_perf_trace_t3000(
    device_mesh,
    use_program_cache,
    device_batch_size,
    expected_inference_time,
    expected_compile_time,
    hf_cat_image_sample_input,
    enable_async_mode,
    model_location_generator,
):
    mode = "async" if enable_async_mode else "sync"
    run_perf_resnet(
        device_batch_size,
        expected_inference_time,
        expected_compile_time,
        hf_cat_image_sample_input,
        device_mesh,
        f"resnet50_trace_{mode}",
        model_location_generator,
    )


@run_for_wormhole_b0()
@pytest.mark.model_perf_t3000
@pytest.mark.parametrize("device_params", [{"l1_small_size": 32768, "num_command_queues": 2}], indirect=True)
@pytest.mark.parametrize(
    "device_batch_size, enable_async_mode, expected_inference_time, expected_compile_time",
    (
        (16, True, 0.0110, 60),
        (16, False, 0.0220, 60),
    ),
    indirect=["enable_async_mode"],
)
def test_perf_2cqs_t3000(
    device_mesh,
    use_program_cache,
    device_batch_size,
    expected_inference_time,
    expected_compile_time,
    hf_cat_image_sample_input,
    enable_async_mode,
    model_location_generator,
):
    mode = "async" if enable_async_mode else "sync"
    run_perf_resnet(
        device_batch_size,
        expected_inference_time,
        expected_compile_time,
        hf_cat_image_sample_input,
        device_mesh,
        f"resnet50_2cqs_{mode}",
        model_location_generator,
    )


@run_for_wormhole_b0()
@pytest.mark.model_perf_t3000
@pytest.mark.parametrize(
    "device_params", [{"l1_small_size": 32768, "num_command_queues": 2, "trace_region_size": 1332224}], indirect=True
)
@pytest.mark.parametrize(
    "device_batch_size, enable_async_mode, expected_inference_time, expected_compile_time",
    (
        (16, True, 0.0043, 60),
        (16, False, 0.009, 60),
    ),
    indirect=["enable_async_mode"],
)
def test_perf_trace_2cqs_t3000(
    device_mesh,
    use_program_cache,
    device_batch_size,
    expected_inference_time,
    expected_compile_time,
    hf_cat_image_sample_input,
    enable_async_mode,
    model_location_generator,
):
    mode = "async" if enable_async_mode else "sync"
    run_perf_resnet(
        device_batch_size,
        expected_inference_time,
        expected_compile_time,
        hf_cat_image_sample_input,
        device_mesh,
        f"resnet50_trace_2cqs_{mode}",
        model_location_generator,
    )
