// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

using namespace std;

void reader_bmm_tile_layout(
    // in0 tensor args
    uint32_t in0_tensor_start_tile_id,
    uint32_t in0_tensor_stride_w,
    uint32_t in0_tensor_stride_h,
    uint32_t in0_tensor_next_block_stride,
    // in0 block args
    uint32_t in0_block_w,
    uint32_t in0_block_h,
    uint32_t in0_block_num_tiles,
    // in1 tensor args
    uint32_t in1_tensor_start_tile_id,
    uint32_t in1_tensor_stride_w,
    uint32_t in1_tensor_stride_h,
    uint32_t in1_tensor_next_block_stride,
    // in1 block args
    uint32_t in1_block_w,
    uint32_t in1_block_h,
    uint32_t in1_block_num_tiles,
    // in0/in1 common args
    uint32_t num_blocks,
    // batch args
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    PRINT_CORE(READER, core_id) << " READER reader_bmm_tile_layout" << endl;
    PRINT_SEP;
    printf("[READER THREAD %d] IN0 TENSOR\n", core_id);
    printf("[READER THREAD %d] in0_tensor_start_tile_id=%d in0_tensor_stride_w=%d in0_tensor_stride_h=%d in0_tensor_next_block_stride=%d\n", core_id, in0_tensor_start_tile_id, in0_tensor_stride_w, in0_tensor_stride_h, in0_tensor_next_block_stride);
    printf("[READER THREAD %d] in0_block_w=%d in0_block_h=%d in0_block_num_tiles=%d\n", core_id, in0_block_w, in0_block_h, in0_block_num_tiles);
    PRINT_LINE;
    printf("[READER THREAD %d] IN1 TENSOR\n", core_id);
    printf("[READER THREAD %d] in1_tensor_start_tile_id=%d in1_tensor_stride_w=%d in1_tensor_stride_h=%d in1_tensor_next_block_stride=%d\n", core_id, in1_tensor_start_tile_id, in1_tensor_stride_w, in1_tensor_stride_h, in1_tensor_next_block_stride);
    printf("[READER THREAD %d] in1_block_w=%d in1_block_h=%d in1_block_num_tiles=%d\n", core_id, in1_block_w, in1_block_h, in1_block_num_tiles);
    PRINT_LINE;
    printf("[READER THREAD %d] num_blocks=%d\n", core_id, num_blocks);
    printf("[READER THREAD %d] MtKt=%d KtNt=%d batch=%d bcast_B=%d\n", core_id, MtKt, KtNt, batch, bcast_B);
    PRINT_SEP;

    for (uint32_t b = 0; b < batch; b++) {
        PRINT_CORE(READER, core_id) << " in0_tensor_start_tile_id = " << in0_tensor_start_tile_id << " in1_tensor_start_tile_id = " << in1_tensor_start_tile_id << endl;
        uint32_t in0_tensor_current_block_start_tile_id = in0_tensor_start_tile_id;
        uint32_t in1_tensor_current_block_start_tile_id = in1_tensor_start_tile_id;
        for(uint32_t block = 0; block < num_blocks; block++) {
            PRINT_CORE(READER, core_id) << " in0_tensor_current_block_start_tile_id = " << in0_tensor_current_block_start_tile_id << " in1_tensor_current_block_start_tile_id = " << in1_tensor_current_block_start_tile_id << endl;
            PRINT_LINE;
            // cb_reserve_back(cb_id_in0, in0_block_num_tiles);
            // cb_reserve_back(cb_id_in1, in1_block_num_tiles);

            // l1_write_addr_in0 = get_write_ptr(cb_id_in0);
            // l1_write_addr_in1 = get_write_ptr(cb_id_in1);

            PRINT_CORE(READER, core_id) << " Start loop in0:: b = " << b << " block = " << block << endl;
            uint32_t in0_tensor_row_start_tile_id = in0_tensor_current_block_start_tile_id;
            for(uint32_t h = 0; h < in0_block_h; h++) {
                PRINT_CORE(READER, core_id) << " in0_tensor_row_start_tile_id = " << in0_tensor_row_start_tile_id << endl;
                uint32_t in0_tensor_tile_id = in0_tensor_row_start_tile_id;
                for(uint32_t w = 0; w < in0_block_w; w++) {
                    PRINT_CORE(READER, core_id) << " READ in0_tensor_tile_id = " << in0_tensor_tile_id << endl;
                    core_args->dram_read(1);
                    // noc_async_read_tile(in0_tensor_tile_id, s0, l1_write_addr_in0);
                    // l1_write_addr_in0 += in0_single_tile_size_bytes;
                    in0_tensor_tile_id += in0_tensor_stride_w;
                }
                in0_tensor_row_start_tile_id += in0_tensor_stride_h;
            }
            in0_tensor_current_block_start_tile_id += in0_tensor_next_block_stride;
            PRINT_LINE;

            PRINT_CORE(READER, core_id) << " Start loop in1:: b = " << b << " block = " << block << endl;
            uint32_t in1_tensor_row_start_tile_id = in1_tensor_current_block_start_tile_id;
            for(uint32_t h = 0; h < in1_block_h; h++) {
                PRINT_CORE(READER, core_id) << " in1_tensor_row_start_tile_id = " << in1_tensor_row_start_tile_id << endl;
                uint32_t in1_tensor_tile_id = in1_tensor_row_start_tile_id;
                for(uint32_t w = 0; w < in1_block_w; w++) {
                    PRINT_CORE(READER, core_id) << " READ in1_tensor_tile_id = " << in1_tensor_tile_id << endl;
                    core_args->dram_read(1);
                    // noc_async_read_tile(in1_tensor_tile_id, s1, l1_write_addr_in1);
                    // l1_write_addr_in1 += in1_single_tile_size_bytes;
                    in1_tensor_tile_id += in1_tensor_stride_w;
                }
                in1_tensor_row_start_tile_id += in1_tensor_stride_h;
            }
            in1_tensor_current_block_start_tile_id += in1_tensor_next_block_stride;
            PRINT_LINE;
            // noc_async_read_barrier();

            // cb_push_back(cb_id_in0, in0_block_num_tiles);
            // cb_push_back(cb_id_in1, in1_block_num_tiles);
            PRINT_CORE(READER, core_id) << " End loop:: b = " << b << " block = " << block << endl;
            PRINT_LINE;
        }
        if (bcast_B == 0) {
            in1_tensor_start_tile_id += KtNt;
        }
        in0_tensor_start_tile_id += MtKt;
        PRINT_SEP;
    }
    PRINT_LINE;
}
