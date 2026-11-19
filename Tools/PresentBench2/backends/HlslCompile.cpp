// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "HlslCompile.h"
#include "../../../IntelPresentMon/CommonUtilities/log/Log.h"
#include "../../../IntelPresentMon/CommonUtilities/log/HrLogger.h"
#include <d3dcompiler.h>
#include <format>
#include <stdexcept>

namespace pb2
{
	Microsoft::WRL::ComPtr<ID3DBlob> CompileHlsl(std::string_view source,
		const char* entry, const char* target, const D3D_SHADER_MACRO* macros,
		const char* sourceName)
	{
		Microsoft::WRL::ComPtr<ID3DBlob> blob;
		Microsoft::WRL::ComPtr<ID3DBlob> errBlob;
		const HRESULT hr = D3DCompile(source.data(), source.size(), sourceName, macros,
			nullptr, entry, target, 0, 0, &blob, &errBlob);
		if (FAILED(hr)) {
			std::string detail = std::format("HLSL compile failed ({}/{}): 0x{:08X}",
				entry, target, (unsigned)hr);
			if (errBlob && errBlob->GetBufferSize() > 0) {
				detail.append(": ");
				detail.append((const char*)errBlob->GetBufferPointer(), errBlob->GetBufferSize());
			}
			pmlog_error(detail);
			throw std::runtime_error(detail);
		}
		return blob;
	}
}
