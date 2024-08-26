// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

using namespace std;

void writer_bmm_tile_layout(
    uint32_t out_tensor_start_tile_id,
    uint32_t out_tensor_stride_w,
    uint32_t out_tensor_stride_h,
    uint32_t out_tensor_next_subblock_stride_w,
    uint32_t out_tensor_next_subblock_stride_h,
    uint32_t out_subblock_w,
    uint32_t out_subblock_h,
    uint32_t out_subblock_tile_count,
    uint32_t out_num_subblocks_w,
    uint32_t out_num_subblocks_h,
    uint32_t MtNt,
    uint32_t batch,
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    PRINT_CORE(WRITER, core_id) << " WRITER writer_bmm_tile_layout" << endl;
    PRINT_SEP;
    printf("[WRITER THREAD %d] OUT TENSOR\n", core_id);
    printf("[WRITER THREAD %d] out_tensor_start_tile_id=%u out_tensor_stride_w=%u out_tensor_stride_h=%u\n", core_id, out_tensor_start_tile_id, out_tensor_stride_w, out_tensor_stride_h);
    printf("[WRITER THREAD %d] out_tensor_next_subblock_stride_w=%u out_tensor_next_subblock_stride_h=%u\n", core_id, out_tensor_next_subblock_stride_w, out_tensor_next_subblock_stride_h);
    PRINT_LINE;
    printf("[WRITER THREAD %d] OUT SUBBLOCK\n", core_id);
    printf("[WRITER THREAD %d] out_subblock_w=%u out_subblock_h=%u out_subblock_tile_count=%u\n", core_id, out_subblock_w, out_subblock_h, out_subblock_tile_count);
    printf("[WRITER THREAD %d] out_num_subblocks_w=%u out_num_subblocks_h=%u\n", core_id, out_num_subblocks_w, out_num_subblocks_h);
    PRINT_LINE;
    printf("[WRITER THREAD %d] MtNt=%u batch=%u\n", core_id, MtNt, batch);
    PRINT_SEP;


    for (uint32_t b = 0; b < batch; b++) {
        PRINT_CORE(WRITER, core_id) << " Processing batch " << b << "/" << batch << endl;
        PRINT_CORE(WRITER, core_id) << " out_tensor_start_tile_id = " << out_tensor_start_tile_id << endl;
        uint32_t out_tensor_sbh_start_tile_id = out_tensor_start_tile_id;

        for (uint32_t sbh = 0; sbh < out_num_subblocks_h; sbh++) {
            PRINT_CORE(WRITER, core_id) << " Processing subblock h " << sbh << "/" << out_num_subblocks_h << endl;
            PRINT_CORE(WRITER, core_id) << " out_tensor_sbh_start_tile_id = " << out_tensor_sbh_start_tile_id << endl;
            uint32_t out_tensor_sbw_start_tile_id = out_tensor_sbh_start_tile_id;

            for (uint32_t sbw = 0; sbw < out_num_subblocks_w; sbw++) {
                PRINT_CORE(WRITER, core_id) << " Processing subblock w " << sbw << "/" << out_num_subblocks_w << endl;
                PRINT_CORE(WRITER, core_id) << " out_tensor_sbw_start_tile_id = " << out_tensor_sbw_start_tile_id << endl;
                uint32_t out_tensor_sb_row_start_tile_id = out_tensor_sbw_start_tile_id;

                // cb_wait_front(cb_id_out0, out_subblock_tile_count);
                // uint32_t l1_read_addr = get_read_ptr(cb_id_out0);

                for (uint32_t h = 0; h < out_subblock_h; h++) {
                    PRINT_CORE(WRITER, core_id) << " Processing row " << h << "/" << out_subblock_h << endl;
                    PRINT_CORE(WRITER, core_id) << " out_tensor_sb_row_start_tile_id = " << out_tensor_sb_row_start_tile_id << endl;
                    uint32_t out_tensor_tile_id = out_tensor_sb_row_start_tile_id;

                    for (uint32_t w = 0; w < out_subblock_w; w++) {
                        PRINT_CORE(WRITER, core_id) << " Writing out_tensor_tile_id=" << out_tensor_tile_id << endl;
                        // noc_async_write_tile(out_tensor_tile_id, s, l1_read_addr);
                        // l1_read_addr += single_tile_size_bytes;

                        out_tensor_tile_id += out_tensor_stride_w;
                    }
                    out_tensor_sb_row_start_tile_id += out_tensor_stride_h;
                }

                // noc_async_write_barrier();
                // cb_pop_front(cb_id_out0, out_subblock_tile_count);
                out_tensor_sbw_start_tile_id += out_tensor_next_subblock_stride_w;
            }
            out_tensor_sbh_start_tile_id += out_tensor_next_subblock_stride_h;
        }
        out_tensor_start_tile_id += MtNt;
        PRINT_LINE;
    }
    PRINT_LINE;
}
