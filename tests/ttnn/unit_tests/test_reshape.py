# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import pytest

import torch

import ttnn
import tt_lib as ttl

from tests.ttnn.utils_for_testing import assert_with_pcc


@pytest.mark.parametrize("n", [16])
@pytest.mark.parametrize("c", [128])
@pytest.mark.parametrize("h", [128])
@pytest.mark.parametrize("w", [16])
def test_reshape_cw_div2_rm(device, n, c, h, w):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(n, c * 2, h, w // 2)

    input_tensor = ttnn.from_torch(
        torch_input_tensor, layout=ttnn.ROW_MAJOR_LAYOUT, device=device, memory_config=ttnn.L1_MEMORY_CONFIG
    )
    output_tensor = ttl.tensor.reshape(input_tensor, n, c * 2, h, w // 2, output_mem_config=ttnn.L1_MEMORY_CONFIG)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [16])
@pytest.mark.parametrize("c", [128])
@pytest.mark.parametrize("h", [128])
@pytest.mark.parametrize("w", [16])
def test_reshape_cw_mul2_rm(device, n, c, h, w):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(n, c // 2, h, w * 2)

    input_tensor = ttnn.from_torch(
        torch_input_tensor, layout=ttnn.ROW_MAJOR_LAYOUT, device=device, memory_config=ttnn.L1_MEMORY_CONFIG
    )
    output_tensor = ttl.tensor.reshape(input_tensor, n, c // 2, h, w * 2, output_mem_config=ttnn.L1_MEMORY_CONFIG)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [16])
@pytest.mark.parametrize("c", [128])
@pytest.mark.parametrize("h", [128])
@pytest.mark.parametrize("w", [16])
def test_reshape_hw_div2_rm(device, n, c, h, w):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(n, c, h * 2, w // 2)

    input_tensor = ttnn.from_torch(
        torch_input_tensor, layout=ttnn.ROW_MAJOR_LAYOUT, device=device, memory_config=ttnn.L1_MEMORY_CONFIG
    )
    output_tensor = ttl.tensor.reshape(input_tensor, n, c, h * 2, w // 2, output_mem_config=ttnn.L1_MEMORY_CONFIG)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [16])
@pytest.mark.parametrize("c", [128])
@pytest.mark.parametrize("h", [256])
@pytest.mark.parametrize("w", [8])
def test_reshape_hw_mul2_rm(device, n, c, h, w):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(n, c, h // 2, w * 2)

    input_tensor = ttnn.from_torch(
        torch_input_tensor, layout=ttnn.ROW_MAJOR_LAYOUT, device=device, memory_config=ttnn.L1_MEMORY_CONFIG
    )
    output_tensor = ttl.tensor.reshape(input_tensor, n, c, h // 2, w * 2, output_mem_config=ttnn.L1_MEMORY_CONFIG)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


def run_reshape_hw_rm_with_program_cache(device, n, c, h, w, use_program_cache):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(n, c, h // 2, w * 2)

    input_tensor = ttnn.from_torch(
        torch_input_tensor, layout=ttnn.ROW_MAJOR_LAYOUT, device=device, memory_config=ttnn.L1_MEMORY_CONFIG
    )
    output_tensor = ttl.tensor.reshape(input_tensor, n, c, h // 2, w * 2, output_mem_config=ttnn.L1_MEMORY_CONFIG)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [16])
@pytest.mark.parametrize("c", [128])
@pytest.mark.parametrize("h", [256])
@pytest.mark.parametrize("w", [8])
def test_reshape_hw_rm_with_program_cache(device, n, c, h, w, use_program_cache):
    for _ in range(2):
        run_reshape_hw_rm_with_program_cache(device, n, c, h, w, use_program_cache)
        # dummy tensor to change tensor alloc
        dummy_shape = [1, 1, 32, 32]
        py_dummy_tensor = torch.randn(dummy_shape)
        tt_dummy_tensor = ttnn.from_torch(
            py_dummy_tensor,
            dtype=ttnn.DataType.BFLOAT16,
            layout=ttnn.TILE_LAYOUT,
            device=device,
            memory_config=ttnn.L1_MEMORY_CONFIG,
        )
    assert device.num_program_cache_entries() == 1


@pytest.mark.parametrize("h", [32])
@pytest.mark.parametrize("w", [2 * 32])
def test_reshape(h, w):
    torch_input_tensor = torch.rand((h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(w, h)

    input_tensor = ttnn.from_torch(torch_input_tensor)
    output_tensor = ttnn.reshape(input_tensor, (w, h))
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("h", [32])
@pytest.mark.parametrize("w", [2 * 32])
def test_reshape_negative_1(h, w):
    torch_input_tensor = torch.rand((h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(-1)

    input_tensor = ttnn.from_torch(torch_input_tensor)
    output_tensor = ttnn.reshape(input_tensor, (-1,))
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [32, 32])
@pytest.mark.parametrize("c", [2 * 32, 2 * 32])
@pytest.mark.parametrize("h", [1, 4])
@pytest.mark.parametrize("w", [1, 4])
def test_reshape_in_4D(n, c, h, w):
    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(h, w, n, c)

    input_tensor = ttnn.from_torch(torch_input_tensor)
    output_tensor = ttnn.reshape(input_tensor, (h, w, n, c))
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.parametrize("n", [32, 64])
@pytest.mark.parametrize("c", [32, 64])
@pytest.mark.parametrize("h", [32, 64])
@pytest.mark.parametrize("w", [32, 64])
def test_reshape_in_4D_on_device(device, n, c, h, w):
    if w != c:
        pytest.skip("ttnn.reshape cannot modify width without causing a mismatch")

    torch_input_tensor = torch.rand((n, c, h, w), dtype=torch.bfloat16)
    torch_output_tensor = torch_input_tensor.reshape(h, w, n, c)

    input_tensor = ttnn.from_torch(torch_input_tensor)
    input_tensor = ttnn.to_device(input_tensor, device)
    output_tensor = ttnn.reshape(input_tensor, (h, w, n, c))
    output_tensor = ttnn.from_device(output_tensor)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)
    assert torch.allclose(torch_output_tensor, output_tensor)


@pytest.mark.skip(reason="ttnn.reshape cannot modify width without causing a mismatch")
def test_permute_reshape(device):
    input_shape = (1, 4, 64, 32)
    output_shape = (1, 64, 128)

    torch_input_tensor = torch.rand(input_shape, dtype=torch.bfloat16)
    torch_output_tensor = torch.permute(torch_input_tensor, (0, 2, 1, 3))
    torch_output_tensor = torch.reshape(torch_output_tensor, output_shape)

    output_tensor = ttnn.from_torch(torch_input_tensor)
    output_tensor = ttnn.to_device(output_tensor, device)
    output_tensor = ttnn.permute(output_tensor, (0, 2, 1, 3))
    output_tensor = ttnn.reshape(output_tensor, output_shape)
    output_tensor = ttnn.from_device(output_tensor)
    output_tensor = ttnn.to_layout(output_tensor, ttnn.ROW_MAJOR_LAYOUT)
    output_tensor = ttnn.to_torch(output_tensor)

    assert_with_pcc(torch_output_tensor, output_tensor, 0.9999)


def test_reshape_with_negative_dim(device):
    input_shape = (1, 4, 64, 32)
    output_shape = (1, -1, 2, 32)
    expected_output_shape = (1, 128, 2, 32)

    torch_input = torch.rand(input_shape, dtype=torch.bfloat16)
    torch_output = torch.reshape(torch_input, output_shape)

    tt_input = ttnn.from_torch(torch_input)
    tt_input = ttnn.to_device(tt_input, device)
    tt_output = ttnn.reshape(tt_input, output_shape)
    tt_output = ttnn.from_device(tt_output)
    tt_output = ttnn.to_torch(tt_output)

    assert list(expected_output_shape) == list(torch_output.shape)
    assert list(expected_output_shape) == list(tt_output.shape)
    assert_with_pcc(torch_output, tt_output, 0.9999)


def test_reshape_tile_layout_mamba(device):
    torch_input_tensor = torch.randn((1, 1, 2048, 64), dtype=torch.bfloat16)
    reshape_shape = (1, 2, 1024, 64)
    torch_result = torch_input_tensor.reshape(reshape_shape)

    input_tensor = ttnn.from_torch(torch_input_tensor, layout=ttnn.TILE_LAYOUT, dtype=ttnn.bfloat16, device=device)
    ttnn_output = ttnn.reshape(input_tensor, reshape_shape)

    output = ttnn.to_torch(ttnn_output)

    assert_with_pcc(torch_result, output, 0.9999)


def test_reshape_tile_layout_only_change_shape(device):
    torch_input_tensor = torch.randn((1, 64, 32, 4 * 32), dtype=torch.bfloat16)
    reshape_shape = (1, 32, 64, 4 * 32)
    torch_result = torch_input_tensor.reshape(reshape_shape)

    input_tensor = ttnn.from_torch(torch_input_tensor, layout=ttnn.TILE_LAYOUT, dtype=ttnn.bfloat16, device=device)
    ttnn_output = ttnn.reshape(input_tensor, reshape_shape)

    output = ttnn.to_torch(ttnn_output)

    assert_with_pcc(torch_result, output, 0.9999)
