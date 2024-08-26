// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "ttsim.h"
#include "../bmm_op.hpp"

void reader_bmm_tile_layout_in0_sender_in1_sender(

    // in0 in1
    uint32_t in0_tensor_start_tile_id,
    uint32_t in0_tensor_stride_w,
    uint32_t in0_tensor_stride_h,
    uint32_t in0_tensor_next_block_stride,
    uint32_t in0_block_w,
    uint32_t in0_block_h,
    uint32_t in0_block_num_tiles,
    
    uint32_t in1_tensor_start_tile_id,
    uint32_t in1_tensor_stride_w,
    uint32_t in1_tensor_stride_h,
    uint32_t in1_tensor_next_block_stride,
    uint32_t in1_block_w,
    uint32_t in1_block_h,
    uint32_t in1_block_num_tiles,

    // in0/in1 common args
    uint32_t num_blocks,

    // noc
    uint32_t in0_mcast_dest_noc_start_x,
    uint32_t in0_mcast_dest_noc_start_y,
    uint32_t in0_mcast_dest_noc_end_x,
    uint32_t in0_mcast_dest_noc_end_y,
    uint32_t in0_mcast_num_dests,
    uint32_t in0_mcast_sender_noc_x,
    uint32_t in0_mcast_sender_noc_y,

    uint32_t in1_mcast_dest_noc_start_x,
    uint32_t in1_mcast_dest_noc_start_y,
    uint32_t in1_mcast_dest_noc_end_x,
    uint32_t in1_mcast_dest_noc_end_y,
    uint32_t in1_mcast_num_dests,
    uint32_t in1_mcast_sender_noc_x,
    uint32_t in1_mcast_sender_noc_y,

    // bmm
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(READER, core_id)

    PRINT << " READER reader_bmm_tile_layout_in0_sender_in1_sender" << ENDL;
    PRINT_SEP;

    PRINT << "Input Parameters:" << ENDL;
    PRINT << "in0_tensor_stride_w=" << in0_tensor_stride_w << ", in0_tensor_stride_h=" << in0_tensor_stride_h << ENDL;
    PRINT << "in0_tensor_next_block_stride=" << in0_tensor_next_block_stride << ENDL;
    PRINT << "in0_block_w=" << in0_block_w << ", in0_block_h=" << in0_block_h << ", in0_block_num_tiles=" << in0_block_num_tiles << ENDL;
    PRINT << "in1_tensor_stride_w=" << in1_tensor_stride_w << ", in1_tensor_stride_h=" << in1_tensor_stride_h << ENDL;
    PRINT << "in1_tensor_next_block_stride=" << in1_tensor_next_block_stride << ENDL;
    PRINT << "in1_block_w=" << in1_block_w << ", in1_block_h=" << in1_block_h << ", in1_block_num_tiles=" << in1_block_num_tiles << ENDL;
    PRINT << "num_blocks=" << num_blocks << ENDL;
    PRINT << "MtKt=" << MtKt << ", KtNt=" << KtNt << ", batch=" << batch << ", bcast_B=" << bcast_B << ENDL;
    PRINT_SEP;

    for (uint32_t b = 0; b < batch; b++) {
        PRINT_LEVEL(0); PRINT << "Processing batch " << b + 1 << "/" << batch << ENDL;

        uint32_t in0_tensor_current_block_start_tile_id = in0_tensor_start_tile_id;
        uint32_t in1_tensor_current_block_start_tile_id = in1_tensor_start_tile_id;

        for (uint32_t block = 0; block < num_blocks; block++) {
            PRINT_LEVEL(1); PRINT << "Processing block " << block + 1 << "/" << num_blocks << ENDL;

            // cb_reserve_back(cb_id_in0, in0_block_num_tiles);
            // l1_write_addr_in0 = get_write_ptr(cb_id_in0);

            uint32_t in0_tensor_row_start_tile_id = in0_tensor_current_block_start_tile_id;
            for (uint32_t h = 0; h < in0_block_h; h++) {
                // PRINT_LEVEL(2); PRINT << "Processing in0 block height " << h + 1 << "/" << in0_block_h << ENDL;

                uint32_t in0_tensor_tile_id = in0_tensor_row_start_tile_id;
                for (uint32_t w = 0; w < in0_block_w; w++) {
                    // PRINT_LEVEL(3); PRINT << "Processing in0 block width " << w + 1 << "/" << in0_block_w << ENDL;

                    // noc_async_read_tile(in0_tensor_tile_id, s0, l1_write_addr_in0);
                    core_args->dram_read(1);
                    // l1_write_addr_in0 += single_tile_size_bytes;
                    PRINT_LEVEL(2); PRINT << "Processing in0 block height " << h + 1 << "/" << in0_block_h << " block width " << w + 1 << "/" << in0_block_w << " READ in0_tensor_tile_id: " << in0_tensor_tile_id << ENDL;
                    in0_tensor_tile_id += in0_tensor_stride_w;
                }
                in0_tensor_row_start_tile_id += in0_tensor_stride_h;
            }
            in0_tensor_current_block_start_tile_id += in0_tensor_next_block_stride;

            // noc_async_read_barrier();

            // noc_semaphore_wait(in0_mcast_sender_semaphore_addr_ptr, in0_mcast_num_dests);
            // noc_semaphore_set(in0_mcast_sender_semaphore_addr_ptr, 0);

            // cb_push_back(cb_id_in0, in0_block_num_tiles);
    
            // Repeat similar process for in1 block
            // cb_reserve_back(cb_id_in1, in1_block_num_tiles);
            // l1_write_addr_in1 = get_write_ptr(cb_id_in1);

            uint32_t in1_tensor_row_start_tile_id = in1_tensor_current_block_start_tile_id;
            for (uint32_t h = 0; h < in1_block_h; h++) {
                // PRINT_LEVEL(2); PRINT << "Processing in1 block height " << h + 1 << "/" << in1_block_h << ENDL;

                uint32_t in1_tensor_tile_id = in1_tensor_row_start_tile_id;
                for (uint32_t w = 0; w < in1_block_w; w++) {
                    // PRINT_LEVEL(3); PRINT << "Processing in1 block width " << w + 1 << "/" << in1_block_w << ENDL;

                    // noc_async_read_tile(in1_tensor_tile_id, s1, l1_write_addr_in1);
                    core_args->dram_read(1);
                    // l1_write_addr_in1 += single_tile_size_bytes;
                    PRINT_LEVEL(2); PRINT << "Processing in1 block height " << h + 1 << "/" << in1_block_h << " block width " << w + 1 << "/" << in1_block_w << " READ in1_tensor_tile_id: " << in1_tensor_tile_id << ENDL;

                    in1_tensor_tile_id += in1_tensor_stride_w;
                }
                in1_tensor_row_start_tile_id += in1_tensor_stride_h;
            }
            in1_tensor_current_block_start_tile_id += in1_tensor_next_block_stride;

            // noc_async_read_barrier();

            // noc_semaphore_wait(in1_mcast_sender_semaphore_addr_ptr, in1_mcast_num_dests);
            // noc_semaphore_set(in1_mcast_sender_semaphore_addr_ptr, 0);

            // cb_push_back(cb_id_in1, in1_block_num_tiles);

            PRINT_LINE;
        }

        if (bcast_B == 0) {
            in1_tensor_start_tile_id += KtNt;
        }
        in0_tensor_start_tile_id += MtKt;
        PRINT_LEVEL(0); PRINT_SEP;
    }

    PRINT_LINE;
}
