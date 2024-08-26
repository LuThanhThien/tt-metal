// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "dataflow_api.h"



void kernel_main() {
    uint32_t num_sticks_per_shard_core = get_arg_val<uint32_t>(0);
    uint32_t num_cores_read = get_arg_val<uint32_t>(1);
    uint32_t read_stick_stride = get_arg_val<uint32_t>(2);
    tt_l1_ptr uint32_t * read_stick_offset = (tt_l1_ptr uint32_t*)(get_arg_addr(3));
    tt_l1_ptr uint32_t * noc_coord_x = (tt_l1_ptr uint32_t*)(get_arg_addr(3 + num_cores_read));
    tt_l1_ptr uint32_t * noc_coord_y = (tt_l1_ptr uint32_t*)(get_arg_addr(3 + num_cores_read * 2));

    constexpr uint32_t cb_in0 = get_compile_time_arg_val(0);
    constexpr uint32_t cb_out0 = get_compile_time_arg_val(1);
    constexpr uint32_t stick_size_bytes = get_compile_time_arg_val(2);

    uint32_t write_stick_stride = stick_size_bytes * num_cores_read;


    uint32_t l1_write_offset = 0;
    for (uint32_t core = 0; core < num_cores_read; ++core) {
        uint32_t l1_read_addr = get_read_ptr(cb_in0) + read_stick_offset[core];
        uint64_t noc_read_addr = get_noc_addr(noc_coord_x[core], noc_coord_y[core], l1_read_addr);
        uint32_t l1_write_addr = get_write_ptr(cb_out0) + l1_write_offset;

        noc_async_read_one_packet_set_state(noc_read_addr, stick_size_bytes);

        for (uint32_t i = 0; i < num_sticks_per_shard_core; ++i) {
            noc_async_read_one_packet_with_state(noc_read_addr, l1_write_addr);
            noc_read_addr += read_stick_stride;
            l1_write_addr += write_stick_stride;
        }
        l1_write_offset += stick_size_bytes;
        noc_async_read_barrier();
    }


}
