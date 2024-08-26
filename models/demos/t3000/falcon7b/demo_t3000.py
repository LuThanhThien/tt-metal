# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
from models.demos.falcon7b_common.demo.demo import run_falcon_demo_kv
from models.utility_functions import is_wormhole_b0, get_devices_for_t3000


@pytest.mark.parametrize(
    "perf_mode, max_seq_len, expected_perf_metrics, greedy_sampling, expected_greedy_output_path",
    (
        (True, 128, {"prefill_t/s": 4350, "decode_t/s": 1000, "decode_t/s/u": 3.9}, False, None),
        (True, 1024, {"prefill_t/s": 9500, "decode_t/s": 950, "decode_t/s/u": 3.7}, False, None),
        (True, 2048, {"prefill_t/s": 7800, "decode_t/s": 950, "decode_t/s/u": 3.7}, False, None),
        (True, 128, None, False, None),
        (True, 1024, None, False, None),
        (True, 2048, None, False, None),
        (False, 1024, None, True, "models/demos/t3000/falcon7b/expected_greedy_output.json"),
        (False, 1024, None, True, None),
        (False, 1024, None, False, None),
    ),
    ids=[
        "perf_mode_128_stochastic_verify",
        "perf_mode_1024_stochastic_verify",
        "perf_mode_2048_stochastic_verify",
        "perf_mode_128_stochastic",
        "perf_mode_1024_stochastic",
        "perf_mode_2048_stochastic",
        "default_mode_1024_greedy_verify",
        "default_mode_1024_greedy",
        "default_mode_1024_stochastic",
    ],
)
@pytest.mark.parametrize("async_mode", (True,))  # Option to run Falcon in Async mode
@pytest.mark.parametrize("num_devices", (1, 2, 3, 4, 5, 6, 7, 8))
def test_demo_multichip(
    perf_mode,  # Option to measure perf using max seq length (with invalid outputs) and expected perf (t/s)
    max_seq_len,
    expected_perf_metrics,  # Expected perf (t/s) for prefill and decode in perf mode
    greedy_sampling,  # Option to use greedy decoding instead of top-k/p
    expected_greedy_output_path,  # Path for expected outputs for greedy decoding
    num_devices,
    user_input,
    model_location_generator,
    get_tt_cache_path,
    all_devices,
    use_program_cache,
    async_mode,
    is_ci_env,
):
    if is_ci_env:
        if num_devices != 8 or (not expected_greedy_output_path and not expected_perf_metrics):
            pytest.skip("Skipping test in CI since it provides redundant testing")
    elif expected_greedy_output_path or expected_perf_metrics:
        assert num_devices == 8, "8 devices are expected for perf and greedy output verification"

    assert is_wormhole_b0(), "Multi-chip is only supported for Wormhole B0"
    devices = get_devices_for_t3000(all_devices, num_devices)

    for device in devices:
        device.enable_async(async_mode)

    batch_size = 32
    if perf_mode:
        csv_perf_targets = {
            "prefill_t/s": {128: 3531, 1024: 70000, 2048: None}[max_seq_len],
            "decode_t/s": 26 * batch_size * num_devices,
            "decode_t/s/u": 26,
        }  # performance targets that we aim for (t3000)
    else:
        csv_perf_targets = {}

    return run_falcon_demo_kv(
        user_input=user_input,
        batch_size=batch_size,
        max_seq_len=max_seq_len,
        model_config_strs_prefill_decode=["BFLOAT16-DRAM", "BFLOAT16-L1_SHARDED"],
        model_location_generator=model_location_generator,
        get_tt_cache_path=get_tt_cache_path,
        devices=devices,
        perf_mode=perf_mode,
        greedy_sampling=greedy_sampling,
        expected_perf_metrics=expected_perf_metrics,
        expected_greedy_output_path=expected_greedy_output_path,
        csv_perf_targets=csv_perf_targets,
    )
