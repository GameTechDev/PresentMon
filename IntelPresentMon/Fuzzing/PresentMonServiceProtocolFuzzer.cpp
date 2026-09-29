// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <IntelPresentMon/Interprocess/source/act/Packet.h>
#include <cereal/archives/binary.hpp>
#include <cereal/types/string.hpp>
#include <cstddef>
#include <cstdint>
#include <sstream>
#include <string>
#include <utility>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, std::size_t size)
{
    std::string input;
    if (size != 0) {
        input.assign(reinterpret_cast<const char*>(data), size);
    }

    std::istringstream stream{ std::move(input), std::ios::binary };
    try {
        pmon::ipc::act::PacketHeader header;
        cereal::BinaryInputArchive archive{ stream };
        archive(header);
    }
    catch (const cereal::Exception&) {
    }

    return 0;
}
