// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

//
// Contains utility functions for partitioning work between multiple cores.
//

#pragma once

#include "core_struct.h"
#include "ttsim.h"
#include <set>
#include <tuple>


// Given a number of tiles and number of cores available
// Set the largest number of cores less than the number of tiles
// Returns the number of cores as well as the number of tiles per core

inline std::tuple<uint32_t, uint32_t> get_max_cores_divisible_by_tiles_per_core_tiles(
    const uint32_t &num_tiles, const uint32_t &num_cores_max, bool request_even = false) {
    uint32_t num_cores = 1;
    for (int i = 2; i <= num_cores_max; i++) {
        if ((num_tiles % i) == 0) {
            num_cores = i;
        }
    }
    if (request_even) {
        num_cores = num_cores - num_cores % 2;
    }
    uint32_t per_core_tiles_dim = num_tiles / num_cores;
    if (num_tiles % num_cores != 0)
        per_core_tiles_dim++;
    return {num_cores, per_core_tiles_dim};
}

// Finds the maximum even divisor of val starting at start_max_div and below
inline int find_max_divisor(uint32_t val, uint32_t start_max_div) {
    int result = 1;
    for (int find_divisor = start_max_div; find_divisor >= 1; find_divisor--) {
        if (find_divisor == 7 || find_divisor == 5)
            continue;
        if (val % find_divisor == 0) {
            result = find_divisor;
            break;
        }
    }
    return result;
}

inline int find_max_block_size(uint32_t val, uint32_t max_block_size=8) {
    int result = 1;
    for (int find_divisor = max_block_size; find_divisor >= 1; find_divisor--) {
        if (val % find_divisor == 0) {
            result = find_divisor;
            break;
        }
    }
    return result;
}

inline void print_set_corerange(std::set<CoreRange> &set) {
    auto itor = set.begin();
    for (int i = 0; i < set.size(); i++) {
        PRINT_LEVEL(1);
        CoreRange it = *itor++;
        PRINT_HOST << "set[" << i << "] = " << it.str() << ENDL;
    }
}

inline std::set<CoreRange> num_cores_to_corerange_set(uint32_t target_num_cores, CoreCoord grid_size, bool row_wise = false) {
	uint32_t num_cores_x = grid_size.x;
    uint32_t num_cores_y = grid_size.y;

    PRINT_HOST << "num_cores_to_corerange_set(target_num_cores=" << target_num_cores << ",grid_size=" << grid_size.str() << ",row_wise=" << row_wise << ")" << ENDL;

	// TT_ASSERT(target_num_cores <= num_cores_x * num_cores_y, "Target number of cores is greater than total number of cores");
    if (target_num_cores > num_cores_x * num_cores_y) {
        PRINT_ERROR << "Target number of cores is greater than total number of cores\n";
        EXIT_FAILURE;
    }
	std::set<CoreRange> all_cores_set;
    if (row_wise) {
        if (target_num_cores > num_cores_x) {
            CoreRange start_block({0, 0}, {num_cores_x - 1, target_num_cores / num_cores_x - 1});
            PRINT_LEVEL(1); PRINT_DEBUG << "insert start_block: " << start_block.str() << ENDL;
            all_cores_set.insert(start_block);
            auto leftover_stick_size = target_num_cores % num_cores_x;
            if (leftover_stick_size > 0) {
                auto leftover_start_y = target_num_cores / num_cores_x;
                PRINT_LEVEL(1); PRINT_DEBUG << "leftover_stick_size=" << leftover_stick_size << ENDL;
                PRINT_LEVEL(1); PRINT_DEBUG << "leftover_start_y=" << leftover_start_y << ENDL;
                CoreRange leftover_block({0, leftover_start_y}, {leftover_stick_size - 1, leftover_start_y});
                PRINT_LEVEL(1); PRINT_DEBUG << "insert leftover_block: " << leftover_block.str() << ENDL;    
                all_cores_set.insert(leftover_block);
            }
        } else {
            CoreRange start_block({0, 0}, {target_num_cores - 1, 0});
            all_cores_set.insert(start_block);
            PRINT_LEVEL(1); PRINT_DEBUG << "insert start_block: " << start_block.str() << ENDL;
        }
    } else {
        if (target_num_cores > num_cores_y) {
            CoreRange start_block({0, 0}, {target_num_cores / num_cores_y - 1, num_cores_y - 1});
            all_cores_set.insert(start_block);
            PRINT_LEVEL(1); PRINT_DEBUG << "insert start_block: " << start_block.str() << ENDL;
            auto leftover_stick_size = target_num_cores % num_cores_y;

            if (leftover_stick_size > 0) {
                auto leftover_start_x = target_num_cores / num_cores_y;
                PRINT_LEVEL(1); PRINT_DEBUG << "leftover_stick_size=" << leftover_stick_size << ENDL;
                PRINT_LEVEL(1); PRINT_DEBUG << "leftover_start_x=" << leftover_start_x << ENDL;
                CoreRange leftover_block({leftover_start_x, 0}, {leftover_start_x, leftover_stick_size - 1});
                PRINT_LEVEL(1); PRINT_DEBUG << "insert leftover_block: " << leftover_block.str() << ENDL;    
                all_cores_set.insert(leftover_block);
            }
        } else {
            CoreRange start_block({0, 0}, {0, target_num_cores - 1});
            all_cores_set.insert(start_block);
            PRINT_LEVEL(1); PRINT_DEBUG << "insert start_block: " << start_block.str() << ENDL;
        }
    }

    PRINT_HOST << "Returning all_cores_set" << ENDL;
    print_set_corerange(all_cores_set);
	return all_cores_set;
}

// This function takes in the core grid size, as well as the number of units of work to divide between the cores
// This function returns the number of cores, the CoreRangeSet of all cores, and then the CoreRangeSet that does
// the greater amount of work, and the CoreRangeSet that does less work if work cannot be evenly divided
// If it can be evenly divided, the second CoreRangeSet is the same as the first, and the last is empty
// The last 2 args are the units of work for the two core grids
inline std::tuple<uint32_t, CoreRangeSet, CoreRangeSet, CoreRangeSet, uint32_t, uint32_t> split_work_to_cores(CoreCoord grid_size, uint32_t units_to_divide, bool row_wise = false) {
    PRINT_DEBUG << "split_work_to_cores(grid_size=" << grid_size.str() << ",units_to_divide=" << units_to_divide << ",row_wise=" << row_wise << ")" << ENDL;
	uint32_t num_cores_x = grid_size.x, num_cores_y = grid_size.y;
	auto target_num_cores = std::min(units_to_divide, num_cores_x * num_cores_y);
	CoreRangeSet all_cores(num_cores_to_corerange_set(target_num_cores, grid_size, row_wise));

	std::set<CoreRange> core_group_1_set;
	std::set<CoreRange> core_group_2_set;
	uint32_t units_per_core_group_1 = units_to_divide / target_num_cores;
	uint32_t units_per_core_group_2 = 0;
    // Evenly divided units to all target cores
	if (units_to_divide % target_num_cores == 0) {
		core_group_1_set = all_cores.ranges();
    // Uneven division of units across cores
    // This case should only be hit when there are more units of work than a full grid of cores
    // which is implicitly assumed in the following logic
	} else {
        // Group of cores that do more work
        core_group_1_set = num_cores_to_corerange_set(units_to_divide % target_num_cores, grid_size, row_wise);
        auto last_block_group_1 = (*core_group_1_set.rbegin());
        auto last_block_all_cores = (*all_cores.ranges().rbegin());
        if (row_wise) {
            // Case where only the last row is divided between core group 1 and 2
            if (last_block_group_1.end_coord.y == last_block_all_cores.end_coord.y && last_block_group_1.end_coord.x != last_block_all_cores.end_coord.x) {
                CoreRange leftover_block(
                    {last_block_group_1.end_coord.x + 1, last_block_group_1.end_coord.y}, last_block_all_cores.end_coord);
                core_group_2_set.insert(leftover_block);
            } else {
                // Case where a middle row is divided between core group 1 and 2
                if (last_block_group_1.end_coord.x != num_cores_x - 1) {
                    CoreRange leftover_stick(
                        {last_block_group_1.end_coord.x + 1, last_block_group_1.end_coord.y},
                        {num_cores_x - 1, last_block_group_1.end_coord.y});
                    core_group_2_set.insert(leftover_stick);
                }
                // Remaining rows of cores that does less work
                CoreRange leftover_block({0, last_block_group_1.end_coord.y + 1}, last_block_all_cores.end_coord);
                core_group_2_set.insert(leftover_block);
            }
        } else {
            // Case where only the last column is divided between core group 1 and 2
            if (last_block_group_1.end_coord.x == last_block_all_cores.end_coord.x && last_block_group_1.end_coord.y != last_block_all_cores.end_coord.y) {
                CoreRange leftover_block(
                    {last_block_group_1.end_coord.x, last_block_group_1.end_coord.y + 1}, last_block_all_cores.end_coord);
                core_group_2_set.insert(leftover_block);
            } else {
                // Case where a middle column is divided between core group 1 and 2
                if (last_block_group_1.end_coord.y != num_cores_y - 1) {
                    CoreRange leftover_stick(
                        {last_block_group_1.end_coord.x, last_block_group_1.end_coord.y + 1},
                        {last_block_group_1.end_coord.x, num_cores_y - 1});
                    core_group_2_set.insert(leftover_stick);
                }
                // Remaining columns of cores that does less work
                CoreRange leftover_block({last_block_group_1.end_coord.x + 1, 0}, last_block_all_cores.end_coord);
                core_group_2_set.insert(leftover_block);
            }
        }
		units_per_core_group_2 = units_per_core_group_1;
        units_per_core_group_1++;
	}
	CoreRangeSet core_group_1(core_group_1_set);
	CoreRangeSet core_group_2(core_group_2_set);
    
    PRINT_HOST << "Returning: " << ENDL;
    PRINT_HOST << "core_group_1 is: " << core_group_1.str() << ENDL;
    if (core_group_2_set.size() > 0) {
        PRINT_HOST << "core_group_2 is: " << core_group_2.str() << ENDL;
    } else {
        PRINT_HOST << "core_group_2 is empty" << ENDL;
    }

	return std::make_tuple(target_num_cores, all_cores, core_group_1, core_group_2, units_per_core_group_1, units_per_core_group_2);
}

