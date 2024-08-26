// SPDX-FileCopyrightText: © 2023 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "tlb_config.hpp"

#include "third_party/umd/device/blackhole_implementation.h"
#include "third_party/umd/device/grayskull_implementation.h"
#include "third_party/umd/device/wormhole_implementation.h"

namespace ll_api {

namespace grayskull {

static constexpr unsigned int DYNAMIC_TLB_BASE_INDEX = tt::umd::grayskull::MEM_LARGE_READ_TLB + 1;

int32_t get_static_tlb_index(CoreCoord target) {
    int flat_index = target.y * tt::umd::grayskull::GRID_SIZE_X + target.x;
    if (flat_index == 0) {
        return -1;
    }
    return flat_index;
}

}  // namespace grayskull

namespace wormhole {

int32_t get_static_tlb_index(CoreCoord target) {
    bool is_eth_location =
        std::find(std::cbegin(tt::umd::wormhole::ETH_LOCATIONS), std::cend(tt::umd::wormhole::ETH_LOCATIONS), target) !=
        std::cend(tt::umd::wormhole::ETH_LOCATIONS);
    bool is_tensix_location =
        std::find(std::cbegin(tt::umd::wormhole::T6_X_LOCATIONS), std::cend(tt::umd::wormhole::T6_X_LOCATIONS), target.x) !=
            std::cend(tt::umd::wormhole::T6_X_LOCATIONS) &&
        std::find(std::cbegin(tt::umd::wormhole::T6_Y_LOCATIONS), std::cend(tt::umd::wormhole::T6_Y_LOCATIONS), target.y) !=
            std::cend(tt::umd::wormhole::T6_Y_LOCATIONS);
    // implementation migrated from wormhole.py in `src/t6ifc/t6py/packages/tenstorrent/chip/wormhole.py` from tensix
    // repo (t6py-wormhole-bringup branch)

    if (is_eth_location) {
        if (target.y == 6) {
            target.y = 1;
        }

        if (target.x >= 5) {
            target.x -= 1;
        }
        target.x -= 1;

        int flat_index = target.y * 8 + target.x;
        int tlb_index = flat_index;
        return tlb_index;

    } else if (is_tensix_location) {
        if (target.x >= 5) {
            target.x -= 1;
        }
        target.x -= 1;

        if (target.y >= 6) {
            target.y -= 1;
        }
        target.y -= 1;

        int flat_index = target.y * 8 + target.x;

        // All 80 get single 1MB TLB.
        int tlb_index = tt::umd::wormhole::ETH_LOCATIONS.size() + flat_index;

        return tlb_index;
    } else {
        return -1;
    }
}

}  // namespace wormhole

namespace blackhole {

int32_t get_static_tlb_index(CoreCoord target) {
    bool is_eth_location =
        std::find(
            std::cbegin(tt::umd::blackhole::ETH_LOCATIONS), std::cend(tt::umd::blackhole::ETH_LOCATIONS), target) !=
        std::cend(tt::umd::blackhole::ETH_LOCATIONS);
    bool is_tensix_location =
        std::find(
            std::cbegin(tt::umd::blackhole::T6_X_LOCATIONS), std::cend(tt::umd::blackhole::T6_X_LOCATIONS), target.x) !=
            std::cend(tt::umd::blackhole::T6_X_LOCATIONS) &&
        std::find(
            std::cbegin(tt::umd::blackhole::T6_Y_LOCATIONS), std::cend(tt::umd::blackhole::T6_Y_LOCATIONS), target.y) !=
            std::cend(tt::umd::blackhole::T6_Y_LOCATIONS);
    // implementation migrated from blackhole.py in `src/t6ifc/t6py/packages/tenstorrent/chip/blackhole.py` from tensix
    // repo (t6py-blackhole-bringup branch)

    // BH worker cores are starting from y=2
    target.y--;
    if (is_eth_location) {
        // TODO: Uplift this
        return -1;
    } else if (is_tensix_location) {
        if (target.x >= 8) {
            target.x -= 2;
        }

        int flat_index = target.y * 14 + target.x;
        int tlb_index = tt::umd::blackhole::ETH_LOCATIONS.size() + flat_index;
        return tlb_index;
    } else {
        return -1;
    }
}

}  // namespace blackhole

void configure_static_tlbs(tt::ARCH arch, chip_id_t mmio_device_id, const metal_SocDescriptor &sdesc, tt_device &device_driver) {
    using get_static_tlb_index_ptr = std::int32_t (*)(tt_xy_pair);
    get_static_tlb_index_ptr get_static_tlb_index;

    const uint32_t dynamic_tlb_count = 16;
    uint32_t dynamic_tlb_base_index, dynamic_tlb_16m_size, dram_channel_0_peer2peer_region_start, dram_channel_0_x, dram_channel_0_y;

    // Need to set these values based on arch because UMD does not expose architecture_implementation
    switch (arch) {
        case tt::ARCH::GRAYSKULL:
            get_static_tlb_index = grayskull::get_static_tlb_index;
            dynamic_tlb_base_index = grayskull::DYNAMIC_TLB_BASE_INDEX; // not defined in grayskull_implementation.h
            dynamic_tlb_16m_size = tt::umd::grayskull::DYNAMIC_TLB_16M_SIZE;
            dram_channel_0_peer2peer_region_start = tt::umd::grayskull::DRAM_CHANNEL_0_PEER2PEER_REGION_START;
            dram_channel_0_x = tt::umd::grayskull::DRAM_CHANNEL_0_X;
            dram_channel_0_y = tt::umd::grayskull::DRAM_CHANNEL_0_Y;
            break;
        case tt::ARCH::WORMHOLE:
        case tt::ARCH::WORMHOLE_B0:
            get_static_tlb_index = wormhole::get_static_tlb_index;
            dynamic_tlb_base_index = tt::umd::wormhole::DYNAMIC_TLB_BASE_INDEX;
            dynamic_tlb_16m_size = tt::umd::wormhole::DYNAMIC_TLB_16M_SIZE;
            dram_channel_0_peer2peer_region_start = tt::umd::wormhole::DRAM_CHANNEL_0_PEER2PEER_REGION_START;
            dram_channel_0_x = tt::umd::wormhole::DRAM_CHANNEL_0_X;
            dram_channel_0_y = tt::umd::wormhole::DRAM_CHANNEL_0_Y;
            break;
        case tt::ARCH::BLACKHOLE:
            get_static_tlb_index = blackhole::get_static_tlb_index;
            dynamic_tlb_base_index = tt::umd::blackhole::DYNAMIC_TLB_BASE_INDEX;
            dynamic_tlb_16m_size = 0;
            dram_channel_0_peer2peer_region_start = tt::umd::blackhole::DRAM_CHANNEL_0_PEER2PEER_REGION_START;
            dram_channel_0_x = tt::umd::blackhole::DRAM_CHANNEL_0_X;
            dram_channel_0_y = tt::umd::blackhole::DRAM_CHANNEL_0_Y;
            break;
        default: TT_THROW("Configuring static TLBs is not supported for {}", tt::get_string(arch));
    }

    auto statically_mapped_cores = sdesc.workers;
    // TODO (#9933): Remove workaround for BH
    if (arch != tt::ARCH::BLACKHOLE) {
        statically_mapped_cores.insert(
            statically_mapped_cores.end(), sdesc.ethernet_cores.begin(), sdesc.ethernet_cores.end());
    }
    std::int32_t address = 0;

    // Setup static TLBs for all worker cores
    for (auto &core : statically_mapped_cores) {
        auto tlb_index = get_static_tlb_index(core);
        // TODO
        // Note: see issue #10107
        // Strict is less performant than Posted, however, metal doesn't presently
        // use this on a perf path and the launch_msg "kernel config" needs to
        // arrive prior to the "go" message during device init and slow dispatch
        // Revisit this when we have a more flexible UMD api
        device_driver.configure_tlb(mmio_device_id, core, tlb_index, address, TLB_DATA::Strict);
    }

    // TODO (#9932): Remove workaround for BH
    if (arch != tt::ARCH::BLACKHOLE) {
        // Setup static TLBs for MMIO mapped data space
        uint64_t peer_dram_offset = dram_channel_0_peer2peer_region_start;
        for (uint32_t tlb_id = dynamic_tlb_base_index; tlb_id < dynamic_tlb_base_index + dynamic_tlb_count; tlb_id++) {
            device_driver.configure_tlb(
                mmio_device_id, CoreCoord(dram_channel_0_x, dram_channel_0_y), tlb_id, peer_dram_offset);
            // Align address space of 16MB TLB to 16MB boundary
            peer_dram_offset += dynamic_tlb_16m_size;
        }
    }
    device_driver.setup_core_to_tlb_map([get_static_tlb_index](CoreCoord core) { return get_static_tlb_index(core); });
}

std::unordered_map<std::string, std::int32_t> get_dynamic_tlb_config(tt::ARCH arch) {
    std::unordered_map<std::string, std::int32_t> dynamic_tlb_config;
    switch (arch) {
        case tt::ARCH::GRAYSKULL:
            dynamic_tlb_config["REG_TLB"] = tt::umd::grayskull::REG_TLB;
            break;
        case tt::ARCH::WORMHOLE_B0:
            dynamic_tlb_config["REG_TLB"] = tt::umd::wormhole::REG_TLB;
            break;
        case tt::ARCH::BLACKHOLE:
            dynamic_tlb_config["REG_TLB"] = tt::umd::blackhole::REG_TLB;
            break;
        default: TT_THROW("Configuring dynamic TLBs is not supported for {}", tt::get_string(arch));
    }
    return dynamic_tlb_config;
}

}  // namespace ll_api
