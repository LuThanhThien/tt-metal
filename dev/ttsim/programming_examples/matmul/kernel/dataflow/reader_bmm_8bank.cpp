

#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

// Function to handle the "reader" logic
void reader_bmm_8bank(
    uint32_t batch, 
    uint32_t Mt, uint32_t Kt, uint32_t Nt, 
    uint32_t MtKt, uint32_t KtNt, 
    uint32_t bcast_B,
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(READER, core_id)

    uint32_t itileA_batch = 0;
    uint32_t itileB_batch = 0;

    PRINT << " READER reader_bmm_8bank.cpp" << std::endl;
    for (uint32_t nb = 0; nb < batch; nb++) {
        PRINT_LEVEL(0); PRINT << "Batch " << nb << std::endl;
        uint32_t itileA = itileA_batch;
        for (uint32_t mt = 0; mt < Mt; mt++) {
            PRINT_SEP;
            uint32_t itileB = itileB_batch;
            for (uint32_t nt = 0; nt < Nt; nt++) {
                for (uint32_t kt = 0; kt < Kt; kt++) {
                    PRINT_LEVEL(1); PRINT << " Pushed itileA=" << itileA << " itileB=" << itileB << std::endl;
                    core_args->dram_read(2);
                    itileA += 1;  // A is MK
                    itileB += Nt; // B is KN, so to get k++ we stride by Nt
                } // Kt loop
                itileB -= KtNt; // revert B to previous state before the K loop (to avoid multiplies)
                itileB += 1;    // B is KN, so here in the end of Nt loop we increment N by 1
                itileA -= Kt;   // resets tileA to kt=0, keep the same mt
            } // Nt loop
            itileA += Kt; // A is MK, advance to next M
        } // Mt loop
        itileA_batch += MtKt; // update batch strides
        if (bcast_B == 0)     // don't increment batch if we broadcast matrix B
            itileB_batch += KtNt;
        PRINT_LINE;
    } // batch loop
}
