/*
 * Copyright (C) 2022-2023 Intel Corporation
 *
 * SPDX-License-Identifier: MIT
 *
 */

#include <cassert>
#include "aub_stream.h"
#include "command_streamer_helper.h"
#include "gpu.h"
#include "hardware_context_imp.h"

namespace aub_stream {

template <typename T>
inline T ptrOffset(T ptrBefore, size_t offset) {
    auto addrBefore = (uintptr_t)ptrBefore;
    auto addrAfter = addrBefore + offset;
    return (T)addrAfter;
}

bool CommandStreamerHelper::isMemorySupported(uint32_t memoryBank, uint32_t alignment) const {
    assert(gpu);
    return gpu->isMemorySupported(memoryBank, alignment);
}

void CommandStreamerHelper::setRingTail(void *pLRCIn, uint32_t ringTail) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetRingRegisters + offsetRingTail);
    *pLRCA++ = mmioEngine + 0x2030;
    *pLRCA++ = ringTail;
}

void CommandStreamerHelper::setRingHead(void *pLRCIn, uint32_t ringHead) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetRingRegisters + offsetRingHead);
    *pLRCA++ = mmioEngine + 0x2034;
    *pLRCA++ = ringHead;
}

void CommandStreamerHelper::setRingBase(void *pLRCIn, uint32_t ringBase) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetRingRegisters + offsetRingBase);
    *pLRCA++ = mmioEngine + 0x2038;
    *pLRCA++ = ringBase;
}

void CommandStreamerHelper::setRingCtrl(void *pLRCIn, uint32_t ringCtrl) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetRingRegisters + offsetRingCtrl);
    *pLRCA++ = mmioEngine + 0x203c;
    *pLRCA++ = ringCtrl;
}

void CommandStreamerHelper::setBbCurrentHeadReg(void *pLRCIn) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetRingRegisters + offsetBbCurrentHeadReg);
    *pLRCA++ = mmioEngine + 0x2168;
    *pLRCA++ = 0;

    *pLRCA++ = mmioEngine + 0x2140;
    *pLRCA++ = 0;
}

void CommandStreamerHelper::setPDP0(void *pLRCIn, uint64_t address) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetPageTableRegisters + offsetPDP0);

    *pLRCA++ = mmioEngine + 0x2274;
    *pLRCA++ = address >> 32;
    *pLRCA++ = mmioEngine + 0x2270;
    *pLRCA++ = address & 0xffffffff;
}

void CommandStreamerHelper::setPDP1(void *pLRCIn, uint64_t address) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetPageTableRegisters + offsetPDP1);

    *pLRCA++ = mmioEngine + 0x227c;
    *pLRCA++ = address >> 32;
    *pLRCA++ = mmioEngine + 0x2278;
    *pLRCA++ = address & 0xffffffff;
}

void CommandStreamerHelper::setPDP2(void *pLRCIn, uint64_t address) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetPageTableRegisters + offsetPDP2);

    *pLRCA++ = mmioEngine + 0x2284;
    *pLRCA++ = address >> 32;
    *pLRCA++ = mmioEngine + 0x2280;
    *pLRCA++ = address & 0xffffffff;
}

void CommandStreamerHelper::setPDP3(void *pLRCIn, uint64_t address) const {
    auto pLRCA = ptrOffset(reinterpret_cast<uint32_t *>(pLRCIn),
                           offsetContext + offsetPageTableRegisters + offsetPDP3);

    *pLRCA++ = mmioEngine + 0x228c;
    *pLRCA++ = address >> 32;
    *pLRCA++ = mmioEngine + 0x2288;
    *pLRCA++ = address & 0xffffffff;
}

void CommandStreamerHelper::setPML(void *pLRCIn, uint64_t address) const {
    setPDP0(pLRCIn, address);
}

void CommandStreamerHelper::initialize(void *pLRCIn, PageTable *ppgtt, uint32_t flags, bool isGroupContext) const {
    auto pLRCABase = reinterpret_cast<uint32_t *>(pLRCIn);

    for (size_t i = 0; i < sizeLRCA / sizeof(uint32_t); i++) {
        pLRCABase[i] = 0x1;
    }

    auto pLRCA = ptrOffset(pLRCABase, offsetContext);

    // Initialize the ring context of the LRCA
    auto pLRI = ptrOffset(pLRCA, offsetLRI0);
    auto numRegs = numRegsLRI0;
    *pLRI++ = 0x11001000 | (2 * numRegs - 1);
    uint32_t value = isGroupContext ? 0x00090001 : 0x00090009; // Inhibit context-restore (if not group context) and synchronous context switch

    value |= flags;
    while (numRegs-- > 0) {
        *pLRI++ = mmioEngine + 0x2244; // CTXT_SR_CTL
        *pLRI++ = value;
    }

    // Initialize the other LRI
    assert(offsetLRI1 == 0x21 * sizeof(uint32_t));
    pLRI = ptrOffset(pLRCA, offsetLRI1);
    numRegs = numRegsLRI1;
    *pLRI++ = 0x11001000 | (2 * numRegs - 1);
    while (numRegs-- > 0) {
        *pLRI++ = mmioEngine + 0x20d8; // DEBUG_MODE
        *pLRI++ = 0x00200020;
    }

    assert(offsetLRI2 == 0x41 * sizeof(uint32_t));
    pLRI = ptrOffset(pLRCA, offsetLRI2);
    numRegs = numRegsLRI2;
    *pLRI++ = 0x11000000 | (2 * numRegs - 1);
    while (numRegs-- > 0) {
        *pLRI++ = mmioEngine + 0x2094; // NOP ID
        *pLRI++ = 0x00000000;
    }

    setRingHead(pLRCIn, 0);
    setRingTail(pLRCIn, 0);
    setRingBase(pLRCIn, 0);
    setRingCtrl(pLRCIn, 0);
    setBbCurrentHeadReg(pLRCIn);

    if (!ppgtt) {
        setPDP0(pLRCIn, 0);
        setPDP1(pLRCIn, 0);
        setPDP2(pLRCIn, 0);
        setPDP3(pLRCIn, 0);
    } else if (ppgtt->getNumAddressBits() >= 48) {
        setPML(pLRCIn, ppgtt->getPhysicalAddress());
    } else {
        auto pChild = ppgtt->getChild(0);
        assert(pChild);
        setPDP0(pLRCIn, pChild->getPhysicalAddress());

        pChild = ppgtt->getChild(1);
        assert(pChild);
        setPDP1(pLRCIn, pChild->getPhysicalAddress());

        pChild = ppgtt->getChild(2);
        assert(pChild);
        setPDP2(pLRCIn, pChild->getPhysicalAddress());

        pChild = ppgtt->getChild(3);
        assert(pChild);
        setPDP3(pLRCIn, pChild->getPhysicalAddress());
    }
}

void CommandStreamerHelper::submit(AubStream &stream, uint32_t ggttLRCA, bool is48Bits, uint32_t contextId) const {
    std::array<MiContextDescriptorReg, 8> contextDescriptor = {};

    contextDescriptor[0].sData.Valid = true;
    contextDescriptor[0].sData.ForcePageDirRestore = false;
    contextDescriptor[0].sData.ForceRestore = false;
    contextDescriptor[0].sData.Legacy = true;
    contextDescriptor[0].sData.FaultSupport = 0;
    contextDescriptor[0].sData.PrivilegeAccessOrPPGTT = true;
    contextDescriptor[0].sData.ADor64bitSupport = is48Bits;

    contextDescriptor[0].sData.LogicalRingCtxAddress = ggttLRCA / 4096;
    contextDescriptor[0].sData.Reserved = 0;
    contextDescriptor[0].sData.ContextID = contextId;
    contextDescriptor[0].sData.Reserved2 = 0;

    submitContext(stream, contextDescriptor);
}

void CommandStreamerHelper::submit(AubStream &stream, const std::array<HardwareContextImp *, 8> &hwContexts, bool is48Bits) const {
    std::array<MiContextDescriptorReg, 8> contextDescriptor = {};

    for (uint32_t i = 0; i < hwContexts.size(); i++) {
        if (!hwContexts[i]) {
            contextDescriptor[i].sData.Valid = false;
            continue;
        }

        contextDescriptor[i].sData.Valid = true;
        contextDescriptor[i].sData.ForcePageDirRestore = false;
        contextDescriptor[i].sData.ForceRestore = false;
        contextDescriptor[i].sData.Legacy = true;
        contextDescriptor[i].sData.FaultSupport = 0;
        contextDescriptor[i].sData.PrivilegeAccessOrPPGTT = true;
        contextDescriptor[i].sData.ADor64bitSupport = is48Bits;

        contextDescriptor[i].sData.LogicalRingCtxAddress = hwContexts[i]->ggttLRCA / 4096;
        contextDescriptor[i].sData.Reserved = 0;
        contextDescriptor[i].sData.ContextID = hwContexts[i]->contextId;
        contextDescriptor[i].sData.Reserved2 = 0;
    }

    submitContext(stream, contextDescriptor);
}

void CommandStreamerHelper::initializeEngineMMIO(AubStream &stream) const {
    for (const auto &mmioPair : getEngineMMIO()) {
        stream.writeMMIO(mmioPair.first, mmioPair.second);
    }
}

void CommandStreamerHelper::pollForCompletion(AubStream &stream) const {
    bool pollNotEqual = false;
    stream.registerPoll(
        mmioEngine + 0x2234, // EXECLIST_STATUS
        getPollForCompletionMask(),
        getPollForCompletionMask(),
        pollNotEqual,
        CmdServicesMemTraceRegisterPoll::TimeoutActionValues::Abort);
}

void CommandStreamerHelper::addBatchBufferJump(std::vector<uint32_t> &ringBuffer, uint64_t gfxAddress, bool isGroupContext) const {
    ringBuffer.push_back(0x11000001);
    ringBuffer.push_back(mmioEngine + 0x2244);

    uint32_t value = isGroupContext ? 0x00090000 : 0x00090008; // Inhibit synchronous context switch (if not group context)

    ringBuffer.push_back(value);
    // Batch Buffer start
    ringBuffer.push_back(0x18800101);
    ringBuffer.push_back(uint32_t(gfxAddress));
    ringBuffer.push_back(uint32_t(uint64_t(gfxAddress) >> 32));
}

void CommandStreamerHelper::storeFenceValue(std::vector<uint32_t> &ringBuffer, uint64_t gfxAddress, uint32_t fenceValue) const {
    // Store DWORD Immediate
    ringBuffer.push_back(0x10400002);
    ringBuffer.push_back(uint32_t(gfxAddress));
    ringBuffer.push_back(0);
    ringBuffer.push_back(fenceValue);
}
} // namespace aub_stream
