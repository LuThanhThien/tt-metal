// SPDX-FileCopyrightText: © 2024 Tenstorrent Inc.
//
// SPDX-License-Identifier: Apache-2.0

#include "llrt/hal.hpp"
#include "tt_metal/third_party/umd/device/tt_soc_descriptor.h"

#if defined (ARCH_GRAYSKULL)

#include "hw/inc/grayskull/dev_mem_map.h"
#include "hw/inc/grayskull/eth_l1_address_map.h" // TODO remove when commonruntimeaddressmap is gone
#include "hostdevcommon/common_runtime_address_map.h"
#include "hw/inc/dev_msgs.h"

#endif

#define GET_MAILBOX_ADDRESS_HOST(x) ((uint64_t) & (((mailboxes_t *)MEM_MAILBOX_BASE)->x))

namespace tt {

namespace tt_metal {

static inline int hv (enum HalMemAddrType v) {
    return static_cast<int>(v);
}

void Hal::initialize_gs() {
#if defined (ARCH_GRAYSKULL)
    constexpr uint32_t num_proc_per_tensix_core = 5;

    std::vector<DeviceAddr> mem_map;
    mem_map.resize(hv(HalMemAddrType::COUNT));
    mem_map[hv(HalMemAddrType::BARRIER)] = MEM_L1_BARRIER;
    mem_map[hv(HalMemAddrType::LAUNCH)] = GET_MAILBOX_ADDRESS_HOST(launch);
    mem_map[hv(HalMemAddrType::WATCHER)] = GET_MAILBOX_ADDRESS_HOST(watcher);
    mem_map[hv(HalMemAddrType::DPRINT)] = GET_MAILBOX_ADDRESS_HOST(dprint_buf);
    mem_map[hv(HalMemAddrType::KERNEL_CONFIG_BASE)] = L1_KERNEL_CONFIG_BASE;
    mem_map[hv(HalMemAddrType::UNRESERVED_BASE)] = L1_UNRESERVED_BASE;

    this->core_info_.push_back({HalProgrammableCoreType::TENSIX, CoreType::WORKER, num_proc_per_tensix_core, mem_map});
#endif
}

}  // namespace tt_metal
}  // namespace tt
