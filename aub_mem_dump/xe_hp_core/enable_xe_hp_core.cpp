/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "aub_mem_dump/family_mapper.h"
#include "aub_mem_dump/gpu.h"
#include "aubstream/product_family.h"

namespace aub_stream {
#ifdef SUPPORT_XE_HP_SDV
static RegisterFamily<ProductFamily::XeHpSdv> registerFamilyXeHp;
#endif
} // namespace aub_stream
