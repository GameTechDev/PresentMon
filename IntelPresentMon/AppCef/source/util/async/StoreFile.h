// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include "../FileLocation.h"
#include "../PathSanitaryCheck.h"
#include <Core/source/kernel/Kernel.h>
#include <Core/source/infra/svc/Services.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <fstream>

namespace p2c::client::util::async
{
    class StoreFile : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "storeFile"; }
        StoreFile() : AsyncEndpoint{ AsyncEndpoint::Environment::KernelTask } {}
        // {payload: string, location: int, path: string} => null
        Result ExecuteOnKernelTask(uint64_t uid, CefRefPtr<CefValue> pArgObj, kern::Kernel& kernel) const override
        {
            using namespace p2c::infra;
            using namespace infra::util;

            // try to resolve configs folder, fallback to cwd
            std::filesystem::path base;
            if (auto fr = svc::Services::ResolveOrNull<FolderResolver>()) {
                const FileLocation loc = Traverse(pArgObj)["location"];
                if (loc == FileLocation::Install) {
                    base = fr->Resolve(FolderResolver::Folder::Install, L"");
                }
                else if (loc == FileLocation::Data) {
                    base = fr->Resolve(FolderResolver::Folder::App, L"");
                }
                else if (loc == FileLocation::Documents) {
                    base = fr->Resolve(FolderResolver::Folder::Documents, L"");
                }
                else {
                    throw std::runtime_error{ std::format("Bad file location: {}", uint32_t(loc)) };
                }
            }
            else {
                base = std::filesystem::current_path();
            }

            // compose path and make sure nobody is trying to escape sandbox
            const auto filePath = base / Traverse(pArgObj)["path"].AsWString();
            if (!PathSanitaryCheck(filePath, base)) {
                throw std::runtime_error{ std::format("Unsanitary path: {}", filePath.string()) };
            }

            // open file (over)write with payload
            if (std::wofstream file{ filePath, std::ios::trunc }) {
                file << Traverse(pArgObj)["payload"].AsWString();
                return Result{ true, CefValueNull() };
            }

            return Result{ false, CefValueNull() };
        }
    };
}