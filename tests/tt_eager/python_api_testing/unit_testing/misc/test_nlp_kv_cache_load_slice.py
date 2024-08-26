# SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
from loguru import logger

import numpy as np
import torch

import tt_lib as ttl
from models.utility_functions import is_grayskull, comp_pcc


def unpadding_test(
    kv_cache_shape,
    seq_len_start,
    seq_len_end,
    device,
    dtype,
):
    if dtype == ttl.tensor.DataType.FLOAT32:
        inp = torch.rand(*kv_cache_shape, dtype=torch.float)
    else:
        inp = torch.rand(*kv_cache_shape, dtype=torch.bfloat16)

    test_tensor = (
        ttl.tensor.Tensor(
            inp.reshape(-1).tolist(),
            inp.shape,
            dtype,
            ttl.tensor.Layout.ROW_MAJOR,
        )
        .to(ttl.tensor.Layout.TILE)
        .to(device)
    )
    test_tensor_tt = ttl.tensor.nlp_kv_cache_load_slice(test_tensor, seq_len_start, seq_len_end)

    test_tensor_pt = test_tensor_tt.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()

    # Pytorch reference
    test_tensor_ref = inp[:, :, seq_len_start:seq_len_end]

    return test_tensor_pt, test_tensor_ref, test_tensor_tt.memory_config(), device.num_program_cache_entries()


@pytest.mark.parametrize(
    "dtype",
    (ttl.tensor.DataType.BFLOAT8_B, ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.FLOAT32),
    ids=["bfloat8_b", "bfloat16", "float"],
)
@pytest.mark.parametrize(
    "kv_cache_shape, seq_len_start, seq_len_end",
    (
        ((9, 1, 64, 64), 0, 32),
        ((9, 2, 64, 64), 0, 32),
        ((9, 3, 64, 64), 32, 64),
        ((3, 2, 64, 64), 32, 64),
        ((1, 1, 64, 64), 32, 64),
        ((1, 1, 128, 96), 32, 64),
        ((1, 1, 128, 96), 64, 96),
        ((1, 3, 32, 32), 0, 32),
        ((1, 6, 32, 32), 0, 32),
        ((1, 6, 128, 64), 32, 128),
        ((4, 6, 128, 64), 96, 128),
        ((1, 32, 2048, 64), 0, 1024),
        ((1, 32, 2048, 64), 128, 1024),
        ((32, 1, 2048, 64), 1024, 1056),
        ((32, 1, 2048, 64), 0, 2016),
        ((32, 1, 2048, 64), 0, 2048),
        ((32, 1, 2048 + 128, 128), 0, 2048),  # llama2 70B use case
        ((1, 32, 2048 + 128, 128), 0, 2048),  # llama2 70B use case
    ),
)
def test_run_unpadding_test(
    kv_cache_shape,
    seq_len_start,
    seq_len_end,
    device,
    dtype,
    use_program_cache,
):
    if is_grayskull():
        pytest.skip("Skipping test on Grayskull")

    for i in range(3):
        # shift input/output tensor by creating very small tensor between loop
        inp = torch.rand(1, 1, 32, 32)
        test_tensor = (
            ttl.tensor.Tensor(
                inp.reshape(-1).tolist(),
                inp.shape,
                dtype,
                ttl.tensor.Layout.ROW_MAJOR,
            )
            .to(ttl.tensor.Layout.TILE)
            .to(device)
        )
        shard_spec_1_cores_grid = ttl.tensor.CoreRangeSet(
            {
                ttl.tensor.CoreRange(
                    ttl.tensor.CoreCoord(0, 0),
                    ttl.tensor.CoreCoord(0, 0),
                ),
            }
        )
        test_mem_cfg = ttl.tensor.MemoryConfig(
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED,
            ttl.tensor.BufferType.L1,
            ttl.tensor.ShardSpec(
                shard_spec_1_cores_grid,  # Volume must match # of attn heads
                [
                    32,  # Each core has 32 users
                    32,  # head dim
                ],
                ttl.tensor.ShardOrientation.ROW_MAJOR,
                False,
            ),
        )
        test_tensor = ttl.tensor.interleaved_to_sharded(test_tensor, sharded_mem_config=test_mem_cfg)

        a_pt, a_ref, memory_config, num_cache_entries = unpadding_test(
            kv_cache_shape,
            seq_len_start,
            seq_len_end,
            device,
            dtype,
        )
        assert a_pt.shape == a_ref.shape
        assert num_cache_entries == 2
        if dtype == ttl.tensor.DataType.BFLOAT8_B:
            # inevitable precision loss for bfloat8_b
            eq, pcc = comp_pcc(a_pt, a_ref, 0.999)
            logger.info(f"comp_pcc: {pcc}")
        else:
            eq = torch.equal(a_pt, a_ref)
        assert eq

        logger.info(memory_config)
        assert memory_config.memory_layout == ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED
        assert memory_config.buffer_type == ttl.tensor.BufferType.L1
        assert memory_config.shard_spec.shape[0] == a_ref.shape[-2]
        assert memory_config.shard_spec.shape[1] == a_ref.shape[-1]

    # hardcoded test 2 to check program caching
    kv_cache_shape = (2, 2, 128, 32)
    seq_len_start = 0
    seq_len_end = 64
    for i in range(2):
        a_pt, a_ref, memory_config, num_cache_entries = unpadding_test(
            kv_cache_shape,
            seq_len_start,
            seq_len_end,
            device,
            dtype,
        )
        assert a_pt.shape == a_ref.shape
        assert num_cache_entries == 3
        if dtype == ttl.tensor.DataType.BFLOAT8_B:
            # inevitable precision loss for bfloat8_b
            eq, pcc = comp_pcc(a_pt, a_ref, 0.999)
            logger.info(f"comp_pcc: {pcc}")
        else:
            eq = torch.equal(a_pt, a_ref)
        assert eq

        logger.info(memory_config)
        assert memory_config.memory_layout == ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED
        assert memory_config.buffer_type == ttl.tensor.BufferType.L1
        assert memory_config.shard_spec.shape[0] == a_ref.shape[-2]
        assert memory_config.shard_spec.shape[1] == a_ref.shape[-1]

        # shift input/output tensor by creating very small tensor between loop
        inp = torch.rand(1, 1, 32, 32)
        test_tensor = (
            ttl.tensor.Tensor(
                inp.reshape(-1).tolist(),
                inp.shape,
                dtype,
                ttl.tensor.Layout.ROW_MAJOR,
            )
            .to(ttl.tensor.Layout.TILE)
            .to(device)
        )
