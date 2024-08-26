# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest
import torch
import math

import tt_lib as ttl
from tests.tt_eager.python_api_testing.sweep_tests.comparison_funcs import (
    comp_equal,
    comp_pcc,
)

from enum import Enum

from models.utility_functions import skip_for_wormhole_b0, skip_for_grayskull


def run_reshard_test(
    device,
    input_shape,
    input_layout,
    input_shard_grid,
    input_shard_shape,
    input_shard_orientation,
    input_sharding_scheme,
    output_shard_grid,
    output_shard_shape,
    output_shard_orientation,
    output_sharding_scheme,
    tt_dtype,
):
    input_shard_grid_set = set()
    for _input_shard_grid in input_shard_grid:
        compute_grid_start = ttl.tensor.CoreCoord(_input_shard_grid[0][0], _input_shard_grid[0][1])
        compute_grid_end = ttl.tensor.CoreCoord(_input_shard_grid[1][0], _input_shard_grid[1][1])
        input_shard_grid_set.add(ttl.tensor.CoreRange(compute_grid_start, compute_grid_end))

    input_shard_grid = ttl.tensor.CoreRangeSet(input_shard_grid_set)

    output_shard_grid_set = set()
    for _output_shard_grid in output_shard_grid:
        compute_grid_start = ttl.tensor.CoreCoord(_output_shard_grid[0][0], _output_shard_grid[0][1])
        compute_grid_end = ttl.tensor.CoreCoord(_output_shard_grid[1][0], _output_shard_grid[1][1])
        output_shard_grid_set.add(ttl.tensor.CoreRange(compute_grid_start, compute_grid_end))

    output_shard_grid = ttl.tensor.CoreRangeSet(output_shard_grid_set)

    output_shard_spec = ttl.tensor.ShardSpec(output_shard_grid, output_shard_shape, output_shard_orientation, False)
    output_mem_config = ttl.tensor.MemoryConfig(output_sharding_scheme, ttl.tensor.BufferType.L1, output_shard_spec)
    if input_layout == ttl.tensor.Layout.ROW_MAJOR and tt_dtype == ttl.tensor.DataType.BFLOAT8_B:
        pytest.skip("Illegal layout/dtype config")

    dram_memory_config = ttl.tensor.MemoryConfig(
        memory_layout=ttl.tensor.TensorMemoryLayout.INTERLEAVED,
        buffer_type=ttl.tensor.BufferType.DRAM,
    )
    torch_tensor = torch.randn(input_shape).bfloat16()
    tt_tensor_sharded = ttl.tensor.Tensor(torch_tensor, tt_dtype).to(input_layout)
    tt_tensor_sharded = tt_tensor_sharded.to(device, dram_memory_config)
    tt_tensor_sharded = ttl.tensor.interleaved_to_sharded(
        tt_tensor_sharded,
        input_shard_grid,
        input_shard_shape,
        input_sharding_scheme,
        input_shard_orientation,
        output_dtype=tt_dtype,
    )

    tt_tensor_reshard = ttl.tensor.reshard(tt_tensor_sharded, output_mem_config)

    tt_tensor_interleaved = ttl.tensor.sharded_to_interleaved(
        tt_tensor_reshard,
        dram_memory_config,
    )

    tt_tensor_interleaved = tt_tensor_interleaved.cpu().to(ttl.tensor.Layout.ROW_MAJOR)
    torch_tensor_after_round_trip = tt_tensor_interleaved.to_torch()

    return torch_tensor, torch_tensor_after_round_trip


@skip_for_wormhole_b0()
@pytest.mark.parametrize(
    "input_shape, input_layout, input_shard_grid,  input_shard_shape, input_shard_orientation, input_sharding_scheme, output_shard_grid, output_shard_shape, output_shard_orientation, output_sharding_scheme",
    [
        (
            [1, 1, 64, 64],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (0, 1)]],
            (64, 32),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.WIDTH_SHARDED,
            [[(0, 0), (0, 1)]],
            (32, 64),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED,
        ),
        (
            [1, 1, 128, 64],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (0, 1)]],
            (64, 64),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
            [[(0, 0), (1, 3)]],
            (32, 32),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
        (
            [1, 1, 32, 128],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (0, 3)]],
            (32, 32),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
            [[(0, 0), (0, 1)]],
            (32, 64),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
        (
            [1, 1, 32, 2304],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (0, 7)]],
            (32, 288),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
            [[(0, 0), (0, 1)]],
            (32, 1152),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
        (
            [1, 1, 32, 16],
            ttl.tensor.Layout.ROW_MAJOR,
            [[(0, 0), (0, 0)]],
            (32, 16),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
            [[(0, 0), (0, 1)]],
            (16, 16),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
        (
            [1, 1, 32, 8192],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (7, 7)]],
            (32, 128),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.WIDTH_SHARDED,
            [[(0, 0), (0, 7)]],
            (32, 1024),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
    ],
)
@pytest.mark.parametrize("tt_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
def test_reshard(
    device,
    input_shape,
    input_layout,
    input_shard_grid,
    input_shard_shape,
    input_shard_orientation,
    input_sharding_scheme,
    output_shard_grid,
    output_shard_shape,
    output_shard_orientation,
    output_sharding_scheme,
    tt_dtype,
):
    torch_tensor, torch_tensor_after_round_trip = run_reshard_test(
        device,
        input_shape,
        input_layout,
        input_shard_grid,
        input_shard_shape,
        input_shard_orientation,
        input_sharding_scheme,
        output_shard_grid,
        output_shard_shape,
        output_shard_orientation,
        output_sharding_scheme,
        tt_dtype,
    )

    assert torch_tensor.shape == torch_tensor_after_round_trip.shape
    if tt_dtype != ttl.tensor.DataType.BFLOAT8_B:
        passing, output = comp_equal(torch_tensor, torch_tensor_after_round_trip)
    else:
        passing, output = comp_pcc(torch_tensor, torch_tensor_after_round_trip)
    assert passing, output


@skip_for_grayskull()
@pytest.mark.parametrize(
    "input_shape, input_layout, input_shard_grid,  input_shard_shape, input_shard_orientation, input_sharding_scheme, output_shard_grid, output_shard_shape, output_shard_orientation, output_sharding_scheme",
    [
        (
            [1, 1, 62720, 256],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (7, 6)]],
            (1120, 256),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED,
            [[(0, 0), (7, 5)], [(0, 6), (0, 6)]],
            (1280, 256),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.HEIGHT_SHARDED,
        ),
    ],
)
@pytest.mark.parametrize("tt_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
def test_reshard_rn50(
    device,
    input_shape,
    input_layout,
    input_shard_grid,
    input_shard_shape,
    input_shard_orientation,
    input_sharding_scheme,
    output_shard_grid,
    output_shard_shape,
    output_shard_orientation,
    output_sharding_scheme,
    tt_dtype,
):
    torch_tensor, torch_tensor_after_round_trip = run_reshard_test(
        device,
        input_shape,
        input_layout,
        input_shard_grid,
        input_shard_shape,
        input_shard_orientation,
        input_sharding_scheme,
        output_shard_grid,
        output_shard_shape,
        output_shard_orientation,
        output_sharding_scheme,
        tt_dtype,
    )

    assert torch_tensor.shape == torch_tensor_after_round_trip.shape
    if tt_dtype != ttl.tensor.DataType.BFLOAT8_B:
        passing, output = comp_equal(torch_tensor, torch_tensor_after_round_trip)
    else:
        passing, output = comp_pcc(torch_tensor, torch_tensor_after_round_trip)
    assert passing, output


@pytest.mark.parametrize(
    "input_shape, input_layout, input_shard_grid,  input_shard_shape, input_shard_orientation, input_sharding_scheme, output_shard_grid, output_shard_shape, output_shard_orientation, output_sharding_scheme",
    [
        (
            [1, 1, 32, 6272],
            ttl.tensor.Layout.TILE,
            [[(0, 0), (6, 6)]],
            (32, 128),
            ttl.tensor.ShardOrientation.ROW_MAJOR,
            ttl.tensor.TensorMemoryLayout.WIDTH_SHARDED,
            [[(0, 0), (0, 6)]],
            (32, 1024),
            ttl.tensor.ShardOrientation.COL_MAJOR,
            ttl.tensor.TensorMemoryLayout.BLOCK_SHARDED,
        ),
    ],
)
@pytest.mark.parametrize("tt_dtype", [ttl.tensor.DataType.BFLOAT16, ttl.tensor.DataType.BFLOAT8_B])
def test_reshard_with_program_cache(
    device,
    use_program_cache,
    input_shape,
    input_layout,
    input_shard_grid,
    input_shard_shape,
    input_shard_orientation,
    input_sharding_scheme,
    output_shard_grid,
    output_shard_shape,
    output_shard_orientation,
    output_sharding_scheme,
    tt_dtype,
):
    torch_tensor, torch_tensor_after_round_trip = run_reshard_test(
        device,
        input_shape,
        input_layout,
        input_shard_grid,
        input_shard_shape,
        input_shard_orientation,
        input_sharding_scheme,
        output_shard_grid,
        output_shard_shape,
        output_shard_orientation,
        output_sharding_scheme,
        tt_dtype,
    )

    assert torch_tensor.shape == torch_tensor_after_round_trip.shape
    if tt_dtype != ttl.tensor.DataType.BFLOAT8_B:
        passing, output = comp_equal(torch_tensor, torch_tensor_after_round_trip)
    else:
        passing, output = comp_pcc(torch_tensor, torch_tensor_after_round_trip)
    assert passing, output

    torch_tensor1, torch_tensor_after_round_trip1 = run_reshard_test(
        device,
        input_shape,
        input_layout,
        input_shard_grid,
        input_shard_shape,
        input_shard_orientation,
        input_sharding_scheme,
        output_shard_grid,
        output_shard_shape,
        output_shard_orientation,
        output_sharding_scheme,
        tt_dtype,
    )

    assert torch_tensor1.shape == torch_tensor_after_round_trip1.shape
    if tt_dtype != ttl.tensor.DataType.BFLOAT8_B:
        passing, output = comp_equal(torch_tensor1, torch_tensor_after_round_trip1)
    else:
        passing, output = comp_pcc(torch_tensor1, torch_tensor_after_round_trip1)
    assert passing, output

    assert device.num_program_cache_entries() == 3
