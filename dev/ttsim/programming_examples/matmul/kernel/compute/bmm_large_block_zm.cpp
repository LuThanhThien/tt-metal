// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <cstdint>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"


void bmm_large_block_zm(
    uint32_t in0_block_w,
    uint32_t in0_num_subblocks,
    uint32_t in0_block_num_tiles,
    uint32_t in0_subblock_num_tiles,
    uint32_t in1_num_subblocks,
    uint32_t in1_block_num_tiles,
    uint32_t in1_per_core_w,
    uint32_t num_blocks,
    uint32_t out_subblock_h,
    uint32_t out_subblock_w,
    uint32_t out_subblock_num_tiles,
    uint32_t batch,
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(COMPUTE, core_id)
    PRINT << " COMPUTE bmm_large_block_zm" << ENDL;
    PRINT_SEP;

    // Print out the compile time arguments
    PRINT << "Compile Time Args:" << core_id << ENDL;
    PRINT << "in0_block_w=" << in0_block_w << ", in0_num_subblocks=" << in0_num_subblocks << ", in0_block_num_tiles=" << in0_block_num_tiles << ENDL;
    PRINT << "in0_subblock_num_tiles=" << in0_subblock_num_tiles << ", in1_num_subblocks=" << in1_num_subblocks << ", in1_block_num_tiles=" << in1_block_num_tiles << ENDL;
    PRINT << "in1_per_core_w=" << in1_per_core_w << ", num_blocks=" << num_blocks << ", out_subblock_h=" << out_subblock_h << ENDL;
    PRINT << "out_subblock_w=" << out_subblock_w << ", out_subblock_num_tiles=" << out_subblock_num_tiles << ", batch=" << batch << ENDL;
    PRINT_SEP;
    
    for (uint32_t b = 0; b < batch; b++){
        bool spill = num_blocks > 1;
        bool enable_reload = false;
        uint32_t out_num_tiles_to_wait = out_subblock_num_tiles;
        PRINT_LEVEL(0); PRINT << " Processing batch " << b + 1 << "/" << batch << ENDL;
        PRINT_LEVEL(0); PRINT << " spill = " << spill << ENDL;
        PRINT_LINE;

        for(uint32_t block = 0; block < num_blocks; block++)
        {
            bool last_out = block == (num_blocks-1);
            PRINT_LEVEL(1); PRINT << " Processing block " << block + 1 << "/" << num_blocks << ENDL;
            PRINT_LEVEL(1); PRINT << " out_num_tiles_to_wait = " << out_num_tiles_to_wait << ENDL;
            PRINT_LEVEL(1); PRINT << " enable_reload = " << enable_reload << ENDL;
            PRINT_LEVEL(1); PRINT << " last_out = " << last_out << ENDL;

            // cb_wait_front(tt::CB::c_in0, in0_block_num_tiles);
            // cb_wait_front(tt::CB::c_in1, in1_block_num_tiles);
            int in0_index_subblock_offset = 0;
            for (uint32_t in0_subblock = 0; in0_subblock < in0_num_subblocks; in0_subblock++) {
                int in1_index_subblock_offset = 0;
                PRINT_LEVEL(2); PRINT << " Processing in0 subblock " << in0_subblock + 1 << "/" << in0_num_subblocks << ENDL;
                // PRINT_LEVEL(2); PRINT << " in0_index_subblock_offset = " << in0_index_subblock_offset << ENDL;

                for (uint32_t in1_subblock = 0; in1_subblock < in1_num_subblocks; in1_subblock++) {
                    PRINT_LEVEL(3); PRINT << " Processing in1 subblock " << in1_subblock + 1 << "/" << in1_num_subblocks << ENDL;
                    // PRINT_LEVEL(3); PRINT << " in1_index_subblock_offset = " << in1_index_subblock_offset << ENDL;

                    // acquire_dst(tt::DstMode::Half);

                    if (enable_reload) {
                        // copy_tile_to_dst_init_short();
                        // cb_wait_front(tt::CB::c_intermed0, out_subblock_num_tiles);
                        for (uint32_t i = 0; i < out_subblock_num_tiles; i++) {
                            // copy_tile(tt::CB::c_intermed0, i, i);
                        }
                        // cb_pop_front(tt::CB::c_intermed0, out_subblock_num_tiles);
                        // mm_init_short();
                        PRINT_LEVEL(4); PRINT << " Reloaded interm buffer" << ENDL;
                    }

                    // Compute output sub-block from in0_subblock x in1_subblock
                    int dst_index = 0;
                    int in0_index_h_offset = 0;
                    for (uint32_t h = 0; h < out_subblock_h; h++) {
                        PRINT_LEVEL(5); PRINT << " Processing h = " << h << ENDL;
                        // PRINT_LEVEL(5); PRINT << " in0_index_h_offset = " << in0_index_h_offset << ENDL;
                        for (uint32_t w = 0; w < out_subblock_w; w++) {
                            PRINT_LEVEL(6); PRINT << " Processing W = " << w << ENDL;
                            // PRINT_LEVEL(6); PRINT << " dst_index = " << dst_index << ENDL;
                            int in1_index_inner_dim_offset = 0;
                            for (uint32_t inner_dim = 0; inner_dim < in0_block_w; inner_dim++) {
                                PRINT_LEVEL(7); PRINT << " Processing inner_dim = " << inner_dim << ENDL;
                                // PRINT_LEVEL(7); PRINT << " in1_index_inner_dim_offset = " << in1_index_inner_dim_offset << ENDL;
                                int in0_index = in0_index_subblock_offset + in0_index_h_offset + inner_dim;
                                int in1_index = in1_index_subblock_offset + in1_index_inner_dim_offset + w;
                                // matmul_tiles(tt::CB::c_in0, tt::CB::c_in1, in0_index, in1_index, dst_index, false /* transpose */);
                                PRINT_LEVEL(7); PRINT << " matmul_tiles(tt::CB::c_in0, tt::CB::c_in1, " << in0_index << ", " << in1_index << ", " << dst_index << ", false /* transpose */);" << ENDL;
                                core_args->unit_compute(1);
                                in1_index_inner_dim_offset += in1_per_core_w;
                            }
                            dst_index++;
                            PRINT_LEVEL(6); PRINT_SEP;
                        }
                        in0_index_h_offset += in0_block_w;
                    }

                    if (last_out) {
                        // Pack out to output buffer
                        // cb_reserve_back(tt::CB::c_out0, out_subblock_num_tiles);
                        for (uint32_t i = 0; i < out_subblock_num_tiles; i++) {
                            // pack_tile(i, tt::CB::c_out0);
                        }
                        // cb_push_back(tt::CB::c_out0, out_subblock_num_tiles);
                    } else {
                        // Wait for tiles in output buffer to be written out since interm and output share memory
                        if (block == 0) {
                            // cb_reserve_back(tt::CB::c_out0, out_num_tiles_to_wait);
                            out_num_tiles_to_wait += out_subblock_num_tiles;
                        }
                        // Move partial result to interm buffer
                        // cb_reserve_back(tt::CB::c_intermed0, out_subblock_num_tiles);
                        for (uint32_t i = 0; i < out_subblock_num_tiles; i++) {
                            // pack_tile(i, tt::CB::c_intermed0);
                        }
                        // cb_push_back(tt::CB::c_intermed0, out_subblock_num_tiles);
                    }

                    // release_dst(tt::DstMode::Half);
                    in1_index_subblock_offset += out_subblock_w;
                }
                in0_index_subblock_offset += in0_subblock_num_tiles;
            }

            if (spill) enable_reload = true;

            // cb_pop_front(tt::CB::c_in0, in0_block_num_tiles);
            // cb_pop_front(tt::CB::c_in1, in1_block_num_tiles);
        }
        PRINT_SEP;
    }
    PRINT_LINE;
}
