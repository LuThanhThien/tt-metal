# SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.

# SPDX-License-Identifier: Apache-2.0

import contextlib
import os

import ttnn
from ttnn._ttnn.deprecated.device import Arch


def get_device_core_grid(device):
    compute_with_storage_grid_size = device.compute_with_storage_grid_size()
    return ttnn.CoreGrid(y=compute_with_storage_grid_size.y, x=compute_with_storage_grid_size.x)


# TODO: Device = ttnn._ttnn.Device
Device = ttnn._ttnn.deprecated.device.Device
Device.core_grid = property(get_device_core_grid)


def open_device(
    device_id: int,
    l1_small_size: int = ttnn._ttnn.deprecated.device.DEFAULT_L1_SMALL_SIZE,
    trace_region_size: int = ttnn._ttnn.deprecated.device.DEFAULT_TRACE_REGION_SIZE,
):
    """
    open_device(device_id: int) -> ttnn.Device:

    Open a device with the given device_id. If the device is already open, return the existing device.
    """
    return ttnn._ttnn.device.open_device(
        device_id=device_id, l1_small_size=l1_small_size, trace_region_size=trace_region_size
    )


def close_device(device):
    """
    close_device(device: ttnn.Device) -> None:

    Close the device and remove it from the device cache.
    """
    synchronize_device(device)
    ttnn._ttnn.device.close_device(device)


def enable_program_cache(device):
    ttnn._ttnn.device.enable_program_cache(device)


def disable_and_clear_program_cache(device):
    ttnn._ttnn.device.disable_and_clear_program_cache(device)


def synchronize_device(device):
    """
    synchronize_device(device: ttnn.Device) -> None:

    Synchronize the device with host by waiting for all operations to complete.
    """
    ttnn._ttnn.deprecated.device.Synchronize(device)


@contextlib.contextmanager
def manage_device(device_id: int):
    """
    manage_device(device_id: int) -> ttnn.Device:

    Context manager for opening and closing a device.
    """
    device = open_device(device_id=device_id)
    try:
        yield device
    finally:
        close_device(device)


def dump_device_memory_state(device, prefix=""):
    ttnn._ttnn.deprecated.device.DumpDeviceMemoryState(device, prefix)


def is_wormhole_b0(device):
    return device.arch() == Arch.WORMHOLE_B0


def is_grayskull(device):
    return device.arch() == Arch.GRAYSKULL


__all__ = []
