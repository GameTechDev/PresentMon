// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/win/Handle.h"
#include <chrono>
#include <string>
#include <thread>
#include <vector>

// raw control-pipe client for transport abuse cases, which need to write bytes the
// real client would never produce (and sometimes to write nothing at all)
class RawPipeClient
{
public:
	// A server posts a bounded number of pipe instances, so a connect can legitimately find
	// them all taken (ERROR_PIPE_BUSY) or find the name momentarily gone while acceptors are
	// replenished (ERROR_FILE_NOT_FOUND). Real clients poll through this via
	// WaitForAvailability, and so must we. Pass a zero timeout to probe without waiting.
	explicit RawPipeClient(const std::string& pipeName, std::chrono::milliseconds connectTimeout = std::chrono::seconds{ 2 })
	{
		const auto deadline = std::chrono::steady_clock::now() + connectTimeout;
		do {
			handle_ = pmon::util::win::Handle{ CreateFileA(pipeName.c_str(),
				GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr) };
			connectError_ = GetLastError();
			if (handle_ || (connectError_ != ERROR_PIPE_BUSY && connectError_ != ERROR_FILE_NOT_FOUND)) {
				return;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds{ 2 });
		} while (std::chrono::steady_clock::now() < deadline);
	}
	RawPipeClient(const RawPipeClient&) = delete;
	RawPipeClient& operator=(const RawPipeClient&) = delete;
	RawPipeClient(RawPipeClient&&) = default;
	RawPipeClient& operator=(RawPipeClient&&) = default;
	~RawPipeClient() = default;
	bool IsConnected() const
	{
		return (bool)handle_;
	}
	DWORD GetConnectError() const
	{
		return connectError_;
	}
	void Write(const void* pData, size_t size)
	{
		DWORD written = 0;
		WriteFile(handle_, pData, (DWORD)size, &written, nullptr);
	}
	void WriteDeclaredBodySize(uint32_t size)
	{
		Write(&size, sizeof(size));
	}
	// writes a length prefix followed by fewer bytes than promised, then stops
	void WriteTruncatedBody(uint32_t declaredSize, uint32_t actualSize)
	{
		WriteDeclaredBodySize(declaredSize);
		const std::vector<char> filler(actualSize, '\0');
		Write(filler.data(), filler.size());
	}
	// true once the server has closed its end, which is how a rejected session is observed
	bool WaitForServerClose(std::chrono::milliseconds timeout)
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (std::chrono::steady_clock::now() < deadline) {
			if (IsClosedByServer()) {
				return true;
			}
			std::this_thread::sleep_for(std::chrono::milliseconds{ 5 });
		}
		return false;
	}
	bool IsClosedByServer() const
	{
		DWORD available = 0;
		return !PeekNamedPipe(handle_, nullptr, 0, nullptr, &available, nullptr);
	}
	void Close()
	{
		handle_.Clear();
	}
private:
	pmon::util::win::Handle handle_;
	DWORD connectError_ = 0;
};
