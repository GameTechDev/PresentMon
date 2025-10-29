// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../AsyncEndpoint.h"
#include "../CefValues.h"
#include "../PathSanitaryCheck.h"
#include <fstream>


namespace p2c::client::util::async
{
    class LoadFile : public AsyncEndpoint
    {
    public:
        static constexpr std::string GetKey() { return "loadFile"; }
        LoadFile() : AsyncEndpoint{ AsyncEndpoint::Environment::RenderProcess } {}
        // {location:int, path:string} => {payload:string}
        Result ExecuteOnRenderer(uint64_t uid, CefRefPtr<CefValue> pArgObj, cef::DataBindAccessor&) const override
        {
            const auto filePath = ResolveSanitizedPath(
                Traverse(pArgObj)["location"],
                Traverse(pArgObj)["path"].AsWString());
            if (std::wifstream file{ filePath }) {
                std::wstring payload(
                    std::istreambuf_iterator<wchar_t>{ file },
                    std::istreambuf_iterator<wchar_t>{}
                );
                return Result{ true, MakeCefObject(CefProp{ "payload", std::move(payload) }) };
            }
            else {
                auto s = std::format("Unable to open file path: {}", filePath.string());
                pmlog_warn(s);
                throw std::runtime_error{ s };
            }
        }
    };
}