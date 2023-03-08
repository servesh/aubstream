/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include "test_defaults.h"
#include "aub_mem_dump/family_mapper.h"
#include "aub_mem_dump/gpu.h"
#include "aub_mem_dump/memory_banks.h"
#include "aub_mem_dump/aub_file_stream.h"
#include "aub_mem_dump/tbx_stream.h"
#include "tests/test_traits/test_traits.h"
#include "gtest/gtest.h"
#include <cstdint>
#include <cstddef>
#include <string>

#include "test.h"

namespace aub_stream {

const Gpu *gpu = nullptr;

uint32_t defaultDevice = 0;
uint32_t defaultStepping = 0;
uint32_t defaultDeviceCount = 1;
size_t defaultHBMSizePerDevice = 0x80000000;
size_t defaultPageSize = 65536;
uint32_t defaultMemoryBank = MEMORY_BANK_0;
uint32_t systemMemoryBank = MEMORY_BANK_SYSTEM;
EngineType defaultEngine = EngineType::NUM_ENGINES;

const bool localMemorySupportedInTests = defaultMemoryBank != MEMORY_BANK_SYSTEM;

std::string folderAUB = ".";
std::string fileSeparator = "/";

std::string getAubFileName(const GpuDescriptor &desc) {

    const ::testing::TestInfo *const testInfo = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = testInfo->test_case_name();
    testName += std::string("_");
    testName += testInfo->name();

    std::stringstream strfilename;
    strfilename << desc.productAbbreviation
                << "_";

    if (desc.deviceCount > 1) {
        strfilename << desc.deviceCount
                    << "tx";
    }

    auto traits = testTraits[static_cast<uint32_t>(desc.productFamily)];
    assert(traits);

    strfilename << traits->deviceSliceCount
                << "x"
                << traits->deviceSubSliceCount
                << "x"
                << traits->deviceEuPerSubSlice
                << "_"
                << testName
                << ".aub";

    // clean-up any fileName issues because of the file system incompatibilities
    auto fileName = strfilename.str();
    for (char &i : fileName) {
        i = i == '/' ? '_' : i;
    }

    std::string filePath(folderAUB);
    filePath.append(fileSeparator);
    filePath.append(fileName);

    return filePath;
}

bool initializeAubStream(AubFileStream &stream,
                         const GpuDescriptor &desc) {

    auto filePath = getAubFileName(desc);

    stream.fileHandle.open(filePath, std::ofstream::binary);
    return stream.init(SteppingValues::A, desc);
}

bool initializeAubStream(AubFileStream &stream) {
    GpuDescriptor desc = *gpu;
    desc.deviceCount = defaultDeviceCount;

    return initializeAubStream(
        stream,
        desc);
}

} // namespace aub_stream

bool MatchMemory::hasBank0(const aub_stream::Gpu *gpu) {
    return gpu->isMemorySupported(aub_stream::MemoryBank::MEMORY_BANK_0, 64 * aub_stream::KB);
}
