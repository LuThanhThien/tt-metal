// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

using namespace std;

void reader_bmm_8bank_output_tiles_partitioned(
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
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(READER, core_id)
    // same arg indices as in reader_binary_diff_lenghts for compat
    PRINT << "READER reader_bmm_8bank_output_tiles_partitioned" << endl;
    PRINT_SEP;

    PRINT << "Mt=" << Mt << " Kt=" << Kt << " Nt=" << Nt << " MtKt=" << MtKt << " KtNt=" << KtNt << endl;
    PRINT << "batch=" << batch << " output_tile_start_id=" << output_tile_start_id << " num_output_tiles=" << num_output_tiles << endl;

    constexpr uint32_t cb_id_in0 = 0;
    constexpr uint32_t cb_id_in1 = 1;

    constexpr uint32_t onetile = 1;
  
    uint32_t itileA = output_tile_start_id / Nt * Kt; // input0 row = output row * input0 width

    // Keep track of end of output row and end of output batch
    uint32_t outbatch = output_tile_start_id % MtNt;
    uint32_t itileB_batch = output_tile_start_id % Nt;
    uint32_t itileB = itileB_batch; // input1 col = output col if we are bcasting
    if (bcast_B == 0)
        itileB += output_tile_start_id / MtNt * KtNt; // offset into correct batch if not bcasting

    PRINT << "Initial values:: outbatch=" << outbatch << " itileB_batch=" << itileB_batch << " itileB=" << itileB << " itileA=" << itileA << endl;

    for (uint32_t n = 0; n < num_output_tiles; n++) {
        PRINT_LEVEL(0); PRINT << "Inner loop:: Processing output tile " << n + 1 << "/" << num_output_tiles << endl;
        PRINT_LEVEL(0); PRINT << "Inner loop:: outbatch=" << outbatch << " itileB_batch=" << itileB_batch << " itileB=" << itileB << " itileA=" << itileA << endl;
        for (uint32_t kt = 0; kt < Kt; kt++) {
            PRINT_LEVEL(1); PRINT << "Processing kt " << kt + 1 << "/" << Kt << endl;
            PRINT_LEVEL(1); PRINT << "Pushing itileA=" << itileA << " itileB=" << itileB << endl;
            core_args->dram_read(2);
            itileA += 1; // A is MK
            itileB += Nt; // B is KN, so to get k++ we stride by Nt
        } // Kt loop
        outbatch += 1;
        itileB_batch += 1;
        itileB -= KtNt; // revert B to previous state before the K loop (to avoid multiplies)
        itileB += 1; // Move to next B col

        if (itileB_batch == Nt) {
            itileB_batch = 0;
            itileB -= Nt; // Go back to first column in batch
            if (outbatch == MtNt) {
                if (bcast_B == 0)
                    itileB += KtNt; // Move B to start of next batch
                outbatch = 0;
            }
        } else {
            itileA -= Kt; // resets tileA to kt=0, keep the same mt
        }
        PRINT_SEP;
    } // batch loop
    PRINT_LINE;
}
