#include <stdint.h>
#include <iostream>
#include "ttsim.h"
#include "../bmm_op.hpp"

// Function to handle the "main" logic (matmul)
void bmm(
    uint32_t batch, 
    uint32_t Mt, 
    uint32_t Kt, 
    uint32_t Nt, 
    CoreArgs *core_args
) {
    int core_id = core_args->core_id;
    #define PRINT PRINT_CORE(COMPUTE, core_id)
    PRINT << "COMPUTE bmm.cpp" << ENDL;
    // the simplest possible version of outer product blocked matmul
    // the reader is expected to read the A's and B's tile rows and tile columns for each output tile
    for (uint32_t nb = 0; nb < batch; nb++) {
        PRINT_LEVEL(0); PRINT << "Processing batch " << nb + 1 << "/" << batch << ENDL;
        for (uint32_t mt_C = 0; mt_C < Mt; ++mt_C) // output tile of C
        {
            PRINT_LEVEL(1); PRINT << "Processing mt_C " << mt_C + 1 << "/" << Mt << ENDL;
            for (uint32_t nt_C = 0; nt_C < Nt; ++nt_C) // output tile index of C
            {
                PRINT_LEVEL(2); PRINT << "Processing nt_C " << nt_C + 1 << "/" << Nt << ENDL;
                for (uint32_t kt = 0; kt < Kt; kt++) {
                    PRINT_LEVEL(3); PRINT << "Processing kt " << kt + 1 << "/" << Kt << ENDL;
                    core_args->unit_compute(1);
                }
                PRINT_LEVEL(2); PRINT_SEP;
            }
        }
        PRINT_LINE;
    }
}
