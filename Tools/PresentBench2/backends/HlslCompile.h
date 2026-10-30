// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <d3dcommon.h>
#include <wrl/client.h>
#include <string>
#include <string_view>

namespace pb2
{
	Microsoft::WRL::ComPtr<ID3DBlob> CompileHlsl(std::string_view source,
		const char* entry, const char* target, const D3D_SHADER_MACRO* macros = nullptr,
		const char* sourceName = "PresentBench2");
}
