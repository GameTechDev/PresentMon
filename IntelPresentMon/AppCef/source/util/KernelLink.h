#pragma once
#include "KernelRelay.h"
#include <cstdint>
#include <memory>

namespace p2c::client::util
{
	// Browser process: the UI's one connection to the kernel. It holds the client end of the pipe
	// pair the kernel created for this UI, relays the page's calls to the kernel, and forwards the
	// kernel's events to the page. Lives from just after CefInitialize until just before CefShutdown.
	class KernelLink
	{
	public:
		// pipeHandleValue is the inherited handle passed on the command line. If it is missing or
		// the session cannot be opened, the UI still runs with no kernel link and calls are refused.
		KernelLink(uint64_t pipeHandleValue, kernel_relay::SignalRelay::BrowserGetter getBrowser);
		kernel_relay::CallRelay& GetCallRelay();
		// service and middleware versions for the renderer, passed with browser creation
		CefRefPtr<CefDictionaryValue> MakeBrowserExtraInfo() const;
	private:
		static std::unique_ptr<CefClient> OpenSession_(uint64_t pipeHandleValue, const kernel_relay::SignalRelay& signals);
		kernel_relay::SignalRelay signals_;
		std::unique_ptr<CefClient> pClient_;
		kernel_relay::CallRelay calls_;
	};
}
