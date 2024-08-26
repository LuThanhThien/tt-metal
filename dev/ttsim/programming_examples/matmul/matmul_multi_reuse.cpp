// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <thread>
#include "kernel/dataflow/reader_bmm_tile_layout.cpp"  // Replace with your alternate reader file
#include "kernel/compute/bmm_large_block_zm.cpp"     // Replace with your alternate compute file
#include "kernel/dataflow/writer_bmm_tile_layout.cpp" // Replace with your alternate writer file
#include "ttsim.h"

using namespace std;

void thread_func(
    // Reader args
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
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,

    // Compute args
    uint32_t in0_num_subblocks,
    uint32_t in0_subblock_num_tiles,
    uint32_t in1_num_subblocks,
    uint32_t in1_per_core_w,
    uint32_t out_subblock_h,
    uint32_t out_subblock_w,
    uint32_t out_subblock_num_tiles,

    // Writer args
    uint32_t out_tensor_start_tile_id,
    uint32_t out_tensor_stride_w,
    uint32_t out_tensor_stride_h,
    uint32_t out_tensor_next_subblock_stride_w,
    uint32_t out_tensor_next_subblock_stride_h,
    uint32_t out_subblock_tile_count,
    uint32_t out_num_subblocks_w,
    uint32_t out_num_subblocks_h,
    uint32_t MtNt,

    CoreArgs *core_args
) {
    // Call the reader function
    in0_tensor_start_tile_id = 0;
    in0_tensor_stride_w = 1;
    in0_tensor_stride_h = 16;
    in0_tensor_next_block_stride = 2;
    in0_block_w = 2;
    in0_block_h = 16;
    in0_block_num_tiles = 32;

    in1_tensor_start_tile_id = 0;
    in1_tensor_stride_w = 1;
    in1_tensor_stride_h = 16;
    in1_tensor_next_block_stride = 32;
    in1_block_w = 2;
    in1_block_h = 2;
    in1_block_num_tiles = 4;

    num_blocks = 8;
    MtKt = 256;
    KtNt = 256;
    batch = 2;
    bcast_B = 0;
    reader_bmm_tile_layout(
        in0_tensor_start_tile_id, in0_tensor_stride_w, in0_tensor_stride_h, in0_tensor_next_block_stride,
        in0_block_w, in0_block_h, in0_block_num_tiles,
        in1_tensor_start_tile_id, in1_tensor_stride_w, in1_tensor_stride_h, in1_tensor_next_block_stride,
        in1_block_w, in1_block_h, in1_block_num_tiles,
        num_blocks, MtKt, KtNt, batch, bcast_B, core_args
    );

    in0_block_w = 2;
    in0_num_subblocks = 4;
    in0_block_num_tiles = 32;
    in0_subblock_num_tiles = 8;
    in1_num_subblocks = 1;
    in1_block_num_tiles = 4;
    in1_per_core_w = 2;
    num_blocks = 8;
    out_subblock_h = 4;
    out_subblock_w = 2;
    out_subblock_num_tiles = 8;

    // Call the compute function
    bmm_large_block_zm(
        in0_block_w, in0_num_subblocks, in0_block_num_tiles, in0_subblock_num_tiles,
        in1_num_subblocks, in1_block_num_tiles, in1_per_core_w, num_blocks,
        out_subblock_h, out_subblock_w, out_subblock_num_tiles, batch, core_args
    );

    out_tensor_start_tile_id = 0;
    out_tensor_stride_w = 1;
    out_tensor_stride_h = 16;
    out_tensor_next_subblock_stride_w = 2;
    out_tensor_next_subblock_stride_h = 64;
    out_subblock_w = 2;
    out_subblock_h = 4;
    out_subblock_tile_count = 8; //out_subblocks_w * out_subblocks_h = 8;
    out_num_subblocks_w = 1;
    out_num_subblocks_h = 4;
    MtNt = 256;
    batch = 2;

    // Call the writer function
    writer_bmm_tile_layout(
        out_tensor_start_tile_id, out_tensor_stride_w, out_tensor_stride_h,
        out_tensor_next_subblock_stride_w, out_tensor_next_subblock_stride_h,
        out_subblock_w, out_subblock_h, out_subblock_tile_count,
        out_num_subblocks_w, out_num_subblocks_h, MtNt, batch, core_args
    );
}
int main() {
    constexpr bool src0_is_dram = true;
    constexpr bool src1_is_dram = true;

    // READER KERNEL ARGS
    // in0_tensor_start_tile_id: 0
    // in0_tensor_stride_w: 1
    // in0_tensor_stride_h: 16
    // in0_tensor_next_block_stride: 2
    // in0_block_w: 2
    // in0_block_h: 16
    // in0_block_num_tiles: 32

    // in1_tensor_start_tile_id: 0
    // in1_tensor_stride_w: 1
    // in1_tensor_stride_h: 16
    // in1_tensor_next_block_stride: 32
    // in1_block_w: 2
    // in1_block_h: 2
    // in1_block_num_tiles: 4

    // num_blocks: 8
    // MtKt: 256
    // KtNt: 256
    // batch: 2
    // bcast_B: 0

    uint32_t src0_addr = 0;
    uint32_t src1_addr = 0;
    uint32_t Mt = 2;
    uint32_t Kt = 2;
    uint32_t Nt = 2;
    uint32_t MtKt = Mt * Kt;
    uint32_t KtNt = Kt * Nt;
    uint32_t batch = 2;
    uint32_t bcast_B = 0; // if 1 we broadcast B to batch
    
    // Parameters for compute and writer functions
    uint32_t in0_block_w = 2;
    uint32_t in0_num_subblocks = 1; // Example value
    uint32_t in0_block_num_tiles = 4; // Example value
    uint32_t in0_subblock_num_tiles = 2; // Example value

    uint32_t in1_block_w = 2;
    uint32_t in1_num_subblocks = 1; // Example value
    uint32_t in1_block_num_tiles = 4; // Example value
    uint32_t in1_per_core_w = 2; // Example value

    uint32_t num_blocks = 1; // Example value
    uint32_t out_subblock_h = 2; // Example value
    uint32_t out_subblock_w = 2; // Example value
    uint32_t out_subblock_num_tiles = 4; // Example value

    uint32_t out_tensor_start_tile_id = 0; // Example value
    uint32_t out_tensor_stride_w = 1; // Example value
    uint32_t out_tensor_stride_h = 1; // Example value
    uint32_t out_tensor_next_subblock_stride_w = 1; // Example value
    uint32_t out_tensor_next_subblock_stride_h = 1; // Example value
    uint32_t out_subblock_tile_count = 2; // Example value
    uint32_t out_num_subblocks_w = 1; // Example value
    uint32_t out_num_subblocks_h = 1; // Example value
    uint32_t MtNt = Mt * Nt;

    uint32_t num_cores = 2; // Example value
    uint32_t output_tile_start_id = 0; // Initialize this according to your logic
    uint32_t num_output_tiles = Mt * Nt / num_cores * batch;

    PRINT_TITLE("MATMUL_MULTI_CORE");
    std::cout << "Mt=" << Mt << "; Kt=" << Kt << "; Nt=" << Nt << "; MtKt=" << MtKt << "; KtNt=" << KtNt << std::endl;
    std::cout << "src0=" << src0_addr << "; src1=" << src1_addr << std::endl;
    std::cout << "batch=" << batch << std::endl;
    std::cout << "num_cores=" << num_cores << std::endl;
    PRINT_LINE;

    CoreArgs shared_core_args;
    vector<CoreArgs> core_args(num_cores);

    for (uint32_t i = 0; i < num_cores; i++) {
        core_args[i] = shared_core_args;
        core_args[i].set_core_id(i);
        thread_func(
            // Reader args
            src0_addr, 1, 1, 1, // Example values
            in0_block_w, 1, 4, // Example values
            src1_addr, 1, 1, 1, // Example values
            in1_block_w, 1, 4, // Example values
            num_blocks, MtKt, KtNt, batch, bcast_B,

            // Compute args
            in0_num_subblocks, in0_subblock_num_tiles,
            in1_num_subblocks, in1_per_core_w,
            out_subblock_h, out_subblock_w, out_subblock_num_tiles,

            // Writer args
            out_tensor_start_tile_id, out_tensor_stride_w, out_tensor_stride_h,
            out_tensor_next_subblock_stride_w, out_tensor_next_subblock_stride_h,
            out_subblock_tile_count, out_num_subblocks_w, out_num_subblocks_h, MtNt,
            
            &core_args[i]
        );
        output_tile_start_id += num_output_tiles;
    }

    shared_core_args.print_stats();

    return 0;
}
