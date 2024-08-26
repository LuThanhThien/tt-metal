// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include "ttsim.h"
#include "../bmm_op.hpp"

void reader_bmm_tile_layout_in0_sender_in1_receiver(
    // tensor
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
    PRINT << " READER reader_bmm_tile_layout_in0_sender_in1_receiver" << std::endl;
    PRINT_SEP;
    
    PRINT << "Input Parameters:" << std::endl;
    PRINT << "in0_tensor_stride_w=" << in0_tensor_stride_w << ", in0_tensor_stride_h=" << in0_tensor_stride_h << std::endl;
    PRINT << "in0_tensor_next_block_stride=" << in0_tensor_next_block_stride << std::endl;
    PRINT << "in0_block_w=" << in0_block_w << ", in0_block_h=" << in0_block_h << ", in0_block_num_tiles=" << in0_block_num_tiles << std::endl;
    PRINT << "in1_tensor_stride_w=" << in1_tensor_stride_w << ", in1_tensor_stride_h=" << in1_tensor_stride_h << std::endl;
    PRINT << "in1_tensor_next_block_stride=" << in1_tensor_next_block_stride << std::endl;
    PRINT << "in1_block_w=" << in1_block_w << ", in1_block_h=" << in1_block_h << ", in1_block_num_tiles=" << in1_block_num_tiles << std::endl;
    PRINT << "num_blocks=" << num_blocks << std::endl;
    PRINT << "in0_mcast_dest_noc_start_x=" << in0_mcast_dest_noc_start_x << ", in0_mcast_dest_noc_start_y=" << in0_mcast_dest_noc_start_y << std::endl;
    PRINT << "in0_mcast_dest_noc_end_x=" << in0_mcast_dest_noc_end_x << ", in0_mcast_dest_noc_end_y=" << in0_mcast_dest_noc_end_y << std::endl;
    PRINT << "in0_mcast_num_dests=" << in0_mcast_num_dests << ", in0_mcast_sender_noc_x=" << in0_mcast_sender_noc_x << std::endl;
    PRINT << "in0_mcast_sender_noc_y=" << in0_mcast_sender_noc_y << std::endl;
    PRINT << "in1_mcast_dest_noc_start_x=" << in1_mcast_dest_noc_start_x << ", in1_mcast_dest_noc_start_y=" << in1_mcast_dest_noc_start_y << std::endl;
    PRINT << "in1_mcast_dest_noc_end_x=" << in1_mcast_dest_noc_end_x << ", in1_mcast_dest_noc_end_y=" << in1_mcast_dest_noc_end_y << std::endl;
    PRINT << "in1_mcast_num_dests=" << in1_mcast_num_dests << ", in1_mcast_sender_noc_x=" << in1_mcast_sender_noc_x << std::endl;
    PRINT << "in1_mcast_sender_noc_y=" << in1_mcast_sender_noc_y << std::endl;
    PRINT << "MtKt=" << MtKt << ", KtNt=" << KtNt << ", batch=" << batch << ", bcast_B=" << bcast_B << std::endl;
    PRINT_SEP;

    constexpr uint32_t cb_id_in0 = 0;
    constexpr uint32_t cb_id_in1 = 1;

    uint32_t l1_write_addr_in0;

    // Set your local VALID value, to be mcasted to destinations flag address after the data has been mcasted
    // volatile tt_l1_ptr uint32_t* in0_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in0_mcast_receiver_semaphore_addr);
    // *(in0_mcast_receiver_semaphore_addr_ptr) = VALID;
    
    // Local address that will be atomically incremented by mcast receivers
    // volatile tt_l1_ptr uint32_t* in0_mcast_sender_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in0_mcast_sender_semaphore_addr);
    // volatile tt_l1_ptr uint32_t* in1_mcast_receiver_semaphore_addr_ptr = reinterpret_cast<volatile tt_l1_ptr uint32_t*>(in1_mcast_receiver_semaphore_addr);

    for (uint32_t b = 0; b < batch; b++) {
        PRINT_LEVEL(0); PRINT << "Processing batch " << b + 1 << "/" << batch << std::endl;

        uint32_t in0_tensor_current_block_start_tile_id = in0_tensor_start_tile_id;
        for (uint32_t block = 0; block < num_blocks; block++) {
            PRINT_LEVEL(1); PRINT << "Processing block " << block + 1 << "/" << num_blocks << std::endl;

            // cb_reserve_back(cb_id_in0, in0_block_num_tiles);
            // l1_write_addr_in0 = get_write_ptr(cb_id_in0);

            uint32_t in0_start_address = l1_write_addr_in0;  // Copy start address of block, to be used for mcasting
            uint32_t in0_block_size_bytes = 0;

            // Copy in0 block into CB
            uint32_t in0_tensor_row_start_tile_id = in0_tensor_current_block_start_tile_id;
            for (uint32_t h = 0; h < in0_block_h; h++) {
                PRINT_LEVEL(2); PRINT << "Processing in0 block height " << h + 1 << "/" << in0_block_h << std::endl;

                uint32_t in0_tensor_tile_id = in0_tensor_row_start_tile_id;
                for (uint32_t w = 0; w < in0_block_w; w++) {
                    PRINT_LEVEL(3); PRINT << "Processing in0 block width " << w + 1 << "/" << in0_block_w << std::endl;

                    // noc_async_read_tile(in0_tensor_tile_id, s0, l1_write_addr_in0);
                    core_args->dram_read(1);
                    // l1_write_addr_in0 += single_tile_size_bytes;
                    in0_tensor_tile_id += in0_tensor_stride_w;
                    // in0_block_size_bytes += single_tile_size_bytes;
                }
                in0_tensor_row_start_tile_id += in0_tensor_stride_h;
            }
            in0_tensor_current_block_start_tile_id += in0_tensor_next_block_stride;

            // Barrier! Make sure the reads are done
            // noc_async_read_barrier();

            // Wait until all in0 mcast destinations have atomically incremented the in0 semaphore_addr
            // noc_semaphore_wait(in0_mcast_sender_semaphore_addr_ptr, in0_mcast_num_dests);
            // noc_semaphore_set(in0_mcast_sender_semaphore_addr_ptr, 0);

            // Now we have the block in the CB address, we can mcast to dests

            // noc_async_write_multicast(in0_start_address, in0_multicast_data_addr, in0_block_size_bytes, in0_mcast_num_dests);
                
            // noc_semaphore_set_multicast(in0_mcast_receiver_semaphore_addr, in0_mcast_receiver_semaphore_noc_addr, in0_mcast_num_dests);

            // cb_push_back(cb_id_in0, in0_block_num_tiles);

            // // Operand 1
            // cb_reserve_back(cb_id_in1, in1_block_num_tiles);

            // noc_semaphore_set(in1_mcast_receiver_semaphore_addr_ptr, INVALID);

            // noc_semaphore_inc(in1_mcast_sender_semaphore_noc_addr, 1);

            // // Wait on in1 semaphore value to become VALID
            // noc_semaphore_wait(in1_mcast_receiver_semaphore_addr_ptr, VALID);

            // cb_push_back(cb_id_in1, in1_block_num_tiles);
            core_args->neighboard_read(1);

            PRINT_LINE;
        }
        in0_tensor_start_tile_id += MtKt;
        PRINT_LEVEL(0); PRINT_SEP;
    }
    PRINT_LINE;
}
