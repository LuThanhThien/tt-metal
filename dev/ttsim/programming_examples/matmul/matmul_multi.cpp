// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <iostream>
#include <thread>
#include "kernel/dataflow/reader_bmm_8bank_output_tiles_partitioned.cpp"
#include "kernel/compute/bmm.cpp"
#include "kernel/dataflow/writer_unary_interleaved_start_id.cpp"
#include "ttsim.h"

using namespace std;

void thread_func(
    uint32_t Mt,
    uint32_t Kt,
    uint32_t Nt,
    uint32_t MtKt,
    uint32_t KtNt,
    uint32_t batch,
    uint32_t bcast_B,
    uint32_t output_tile_start_id,
    uint32_t num_output_tiles,
    uint32_t MtNt,
    CoreArgs *core_args
) {
    reader_bmm_8bank_output_tiles_partitioned(Mt, Kt, Nt, MtKt, KtNt, batch, bcast_B, output_tile_start_id, num_output_tiles, MtNt, core_args);
    bmm(batch, 1, Kt, Nt, core_args);
    writer_unary_interleaved_start_id(num_output_tiles, output_tile_start_id, core_args);
}

int main() {
    constexpr bool src0_is_dram = true;
    constexpr bool src1_is_dram = true;

    uint32_t src0_addr = 0;
    uint32_t src1_addr = 0;
    uint32_t Mt = 2;
    uint32_t Kt = 2;
    uint32_t Nt = 2;
    uint32_t MtKt = Mt * Kt; // if 0
    uint32_t KtNt = Kt * Nt;
    uint32_t batch = 2;
    uint32_t bcast_B = 0; // if 1 we broadcast B to batch

    // Set values for the parameters used in your functions
    uint32_t num_cores = 2;                             // Example value
    uint32_t output_tile_start_id = 0;                  // Initialize this according to your logic
    uint32_t num_output_tiles = Mt * Nt / num_cores * batch;     
    uint32_t MtNt = Mt * Nt;                            // Example calculation

    
    PRINT_TITLE("MATMUL_MULTI_CORE");
    std::cout << "Mt=" << Mt << "; Kt=" << Kt << "; Nt=" << Nt << "; MtKt=" << MtKt << "; KtNt=" << KtNt << std::endl;
    std::cout << "src0=" << src0_addr << "; src1=" << src1_addr << std::endl;
    std::cout << "batch=" << batch << std::endl;
    std::cout << "num_cores=" << num_cores << std::endl;
    PRINT_LINE;


    // Call the separated functions in threading order

    CoreArgs shared_core_args;
    vector<CoreArgs> core_args(num_cores);
    for (uint32_t i = 0; i < num_cores; i++) {
        core_args[i] = shared_core_args;
        core_args[i].set_core_id(i);
        // workers[i] = thread(thread_func, Mt, Kt, Nt, MtKt, KtNt, batch, bcast_B, output_tile_start_id, num_output_tiles, MtNt, i);
        thread_func(
            Mt, 
            Kt, 
            Nt, 
            MtKt, 
            KtNt, 
            batch, 
            bcast_B, 
            output_tile_start_id, 
            num_output_tiles, 
            MtNt, 
            &core_args[i]
        );
        output_tile_start_id += num_output_tiles;
    }
    for (uint32_t i = 0; i < num_cores; i++) {
        // workers[i].join();
    }
    shared_core_args.print_stats();

    return 0;
}
