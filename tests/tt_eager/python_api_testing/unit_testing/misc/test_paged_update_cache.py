# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import torch
import pytest

import tt_lib as ttl
from loguru import logger
from models.utility_functions import nearest_32, pad_by_zero
from tests.tt_eager.python_api_testing.sweep_tests.comparison_funcs import comp_pcc, comp_equal
from models.utility_functions import is_grayskull


def run_test_update_cache_decode(
    cache_idx, head_dim, max_seq_len, num_users, num_heads, input_dtype, cache_dtype, device
):
    input_shape = [1, num_users, num_heads, head_dim]
    cache_shape = [num_users, num_heads, max_seq_len, head_dim]
    cache = torch.randn(cache_shape).bfloat16().float()
    cachett = ttl.tensor.Tensor(cache, cache_dtype).to(ttl.tensor.Layout.TILE).to(device)
    x = torch.randn(input_shape).bfloat16().float()
    x_pad = torch.nn.functional.pad(x, (0, 0, 0, 32 - num_heads), "constant", 0)

    xt = ttl.tensor.Tensor(x_pad, input_dtype).to(ttl.tensor.Layout.TILE)
    # Input is sharded
    compute_grid_size = device.compute_with_storage_grid_size()
    num_cores = num_users
    shard_grid = ttl.tensor.CoreRangeSet(ttl.tensor.num_cores_to_corerange_set(num_cores, compute_grid_size, True))
    input_shard_spec = ttl.tensor.ShardSpec(
        shard_grid,
        [
            xt.volume() // xt.get_legacy_shape()[-1] // num_cores,
            xt.get_legacy_shape()[-1],
        ],
        ttl.tensor.ShardOrientation.ROW_MAJOR,
        False,
    )
    input_mem_config = ttl.tensor.MemoryConfig(
        ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED, ttl.tensor.BufferType.L1, input_shard_spec
    )
    xt = xt.to(device, input_mem_config)

    # Create arbitrary update indices
    cache_idxs = [cache_idx + i * 17 for i in range(num_users)]

    cachett = ttl.operations.primary.transformers.paged_update_cache(cachett, xt, cache_idxs)

    for i in range(num_users):
        update_idx = cache_idxs[i]
        x_view = x.permute(1, 0, 2, 3)[i, ...]
        cache[i, 0:num_heads, update_idx : update_idx + x.shape[-2], 0 : x.shape[-1]] = x_view

    tt_got_back = cachett.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()

    tt_updated_slice = []
    for i in range(num_users):
        update_idx = cache_idxs[i]
        tt_slice = tt_got_back[i, 0:num_heads, update_idx : update_idx + x.shape[-2], 0 : x.shape[-1]]
        tt_updated_slice.append(tt_slice)
    tt_updated_slice = torch.stack(tt_updated_slice, dim=0).permute(1, 0, 2, 3)

    if input_dtype == ttl.tensor.DataType.BFLOAT16 and cache_dtype == input_dtype:
        eq_cache, output_cache = comp_equal(cache, tt_got_back)  # checks the entire kv cache
        eq_update, output_update = comp_equal(x, tt_updated_slice)  # checks the updated parts
    else:
        eq_cache, output_cache = comp_pcc(cache, tt_got_back)  # checks the entire kv cache
        eq_update, output_update = comp_pcc(x, tt_updated_slice)  # checks the updated parts
    logger.debug(output_cache)
    logger.debug(output_update)
    assert eq_cache and eq_update


@pytest.mark.parametrize("check_memory", [False])
@pytest.mark.parametrize("head_dim", [128])
@pytest.mark.parametrize("max_seq_len", [2048])
@pytest.mark.parametrize("num_users", [32])
@pytest.mark.parametrize("num_heads", [1])
@pytest.mark.parametrize("input_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
@pytest.mark.parametrize("cache_idx", [0, 1, 127, 1057])
@pytest.mark.parametrize("cache_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
def test_update_cache_decode(
    check_memory,
    cache_idx,
    head_dim,
    max_seq_len,
    num_users,
    num_heads,
    input_dtype,
    cache_dtype,
    device,
    use_program_cache,
):
    if check_memory:
        # Create dram tensors to check for overflow
        cache_shape = [num_users, num_heads, max_seq_len, head_dim]
        dram_low = ttl.tensor.Tensor(torch.zeros(cache_shape), cache_dtype).to(ttl.tensor.Layout.TILE).to(device)
        reserved_space = ttl.tensor.Tensor(torch.zeros(cache_shape), cache_dtype).to(ttl.tensor.Layout.TILE).to(device)
        dram_high = ttl.tensor.Tensor(torch.zeros(cache_shape), cache_dtype).to(ttl.tensor.Layout.TILE).to(device)
        reserved_space.deallocate(True)

        # Create sharded tensors to check for overflow
        input_shape = [1, num_users, num_heads, head_dim]
        x = torch.zeros(input_shape)
        x_pad = torch.nn.functional.pad(x, (0, 0, 0, 32 - num_heads), "constant", 0)

        xt = ttl.tensor.Tensor(x_pad, input_dtype).to(ttl.tensor.Layout.TILE)
        # Input is sharded
        compute_grid_size = device.compute_with_storage_grid_size()
        num_cores = num_users
        shard_grid = ttl.tensor.CoreRangeSet(ttl.tensor.num_cores_to_corerange_set(num_cores, compute_grid_size, True))
        input_shard_spec = ttl.tensor.ShardSpec(
            shard_grid,
            [
                xt.volume() // xt.get_legacy_shape()[-1] // num_cores,
                xt.get_legacy_shape()[-1],
            ],
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            False,
        )
        input_mem_config = ttl.tensor.MemoryConfig(
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED, ttl.tensor.BufferType.L1, input_shard_spec
        )
        sharded_low = xt.to(device, input_mem_config)
        sharded_reserved = ttl.tensor.Tensor(x_pad, input_dtype).to(ttl.tensor.Layout.TILE).to(device, input_mem_config)
        sharded_high = ttl.tensor.Tensor(x_pad, input_dtype).to(ttl.tensor.Layout.TILE).to(device, input_mem_config)
        sharded_reserved.deallocate(True)

    run_test_update_cache_decode(
        cache_idx, head_dim, max_seq_len, num_users, num_heads, input_dtype, cache_dtype, device
    )

    if check_memory:
        # Check for overflow
        def check_zero(tensor):
            assert (tensor == 0).all()

        dram_low = dram_low.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()
        dram_high = dram_high.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()
        sharded_low = sharded_low.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()
        sharded_high = sharded_high.cpu().to(ttl.tensor.Layout.ROW_MAJOR).to_torch()

        check_zero(dram_low)
        check_zero(dram_high)
        check_zero(sharded_low)
        check_zero(sharded_high)


@pytest.mark.parametrize("head_dim", [128])
@pytest.mark.parametrize("max_seq_len", [2048])
@pytest.mark.parametrize("num_users", [32])
@pytest.mark.parametrize("num_heads", [1])
@pytest.mark.parametrize("input_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
@pytest.mark.parametrize("cache_idx", [127, 1057])
@pytest.mark.parametrize("cache_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
def test_update_cache_decode_program_cache(
    cache_idx,
    head_dim,
    max_seq_len,
    num_users,
    num_heads,
    input_dtype,
    cache_dtype,
    device,
    use_program_cache,
):
    dummy_tensors = []
    for i in range(2):
        # Create dram tensors to check for overflow
        cache_shape = [num_users, num_heads, max_seq_len, head_dim]
        dram_low = ttl.tensor.Tensor(torch.zeros(cache_shape), cache_dtype).to(ttl.tensor.Layout.TILE).to(device)
        dummy_tensors.append(dram_low)

        # Create sharded tensors to check for overflow
        input_shape = [1, num_users, num_heads, head_dim]
        x = torch.zeros(input_shape)
        x_pad = torch.nn.functional.pad(x, (0, 0, 0, 32 - num_heads), "constant", 0)

        xt = ttl.tensor.Tensor(x_pad, input_dtype).to(ttl.tensor.Layout.TILE)
        # Input is sharded
        compute_grid_size = device.compute_with_storage_grid_size()
        num_cores = num_users
        shard_grid = ttl.tensor.CoreRangeSet(ttl.tensor.num_cores_to_corerange_set(num_cores, compute_grid_size, True))
        input_shard_spec = ttl.tensor.ShardSpec(
            shard_grid,
            [
                xt.volume() // xt.get_legacy_shape()[-1] // num_cores,
                xt.get_legacy_shape()[-1],
            ],
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            False,
        )
        input_mem_config = ttl.tensor.MemoryConfig(
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED, ttl.tensor.BufferType.L1, input_shard_spec
        )
        sharded_low = xt.to(device, input_mem_config)
        dummy_tensors.append(sharded_low)

        run_test_update_cache_decode(
            cache_idx, head_dim, max_seq_len, num_users, num_heads, input_dtype, cache_dtype, device
        )
        # Test that cache_idx is correctly updated between cached runs
        run_test_update_cache_decode(
            cache_idx + 1, head_dim, max_seq_len, num_users, num_heads, input_dtype, cache_dtype, device
        )

    assert device.num_program_cache_entries() == 1
