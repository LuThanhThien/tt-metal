
#include <iostream>
#include <cstdlib>
#include "kernel/bmm_op.hpp"
#include "kernel/work_split.hpp"
#include "tt_metal/common/constants.hpp"
#include "ttsim.h"


int main(int argc, char* argv[]) {
    uint32_t core_coord_x = 3;
    uint32_t core_coord_y = 4;
    uint32_t num_output_tiles = 9;
    bool row_wise = false;

    // Parsing command-line arguments
    for (int i = 1; i < argc; ++i) {
        string arg = argv[i];
        if (arg == "-c" && i + 2 < argc) {
            core_coord_x = static_cast<uint32_t>(stoi(argv[++i]));
            core_coord_y = static_cast<uint32_t>(stoi(argv[++i]));
        } else if (arg == "-n" && i + 1 < argc) {
            num_output_tiles = static_cast<uint32_t>(stoi(argv[++i]));
        } else if (arg == "-r") {
            row_wise = true;
        } else if (arg == "-d") {
            setenv("DEBUG", "1", 1);
            PRINT_DEBUG << "Debug mode enabled." << ENDL;
        }
        else {
            cerr << "Usage: " << argv[0] << " -c <core_coord_x> <core_coord_y> -n <num_output_tiles> [-r]" << endl;
            return 1;
        }
    }

    // Check if required arguments are provided
    if (core_coord_x == 0 && core_coord_y == 0 && num_output_tiles == 0) {
        cerr << "Error: Missing required arguments." << endl;
        cerr << "Usage: " << argv[0] << " -c <core_coord_x> <core_coord_y> -n <num_output_tiles> [-r]" << endl;
        return 1;
    }

    // Create CoreCoord
    CoreCoord core_coord = {core_coord_x, core_coord_y};

    PRINT_HOST << "CoreCoord: " << core_coord.str() << ENDL;
    PRINT_HOST << "Number of output tiles: " << num_output_tiles << ENDL;
    PRINT_HOST << "Row-wise: " << row_wise << ENDL;
    PRINT_LINE; 

    // Call the function
    auto [num_cores, all_cores, core_group_1, core_group_2, num_output_tiles_per_core_group_1, num_output_tiles_per_core_group_2] = split_work_to_cores(
        core_coord,
        num_output_tiles,
        row_wise
    );

    // Print the results
    PRINT_HOST << "Number of cores: " << num_cores << ENDL;
    PRINT_HOST << "Number of output tiles per core group 1: " << num_output_tiles_per_core_group_1 << ENDL;
    PRINT_HOST << "Number of output tiles per core group 2: " << num_output_tiles_per_core_group_2 << ENDL;

    return 0;
}
