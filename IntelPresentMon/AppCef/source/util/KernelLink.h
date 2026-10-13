#pragma once
#include "KernelRelay.h"
#include <cstdint>
#include <memory>

namespace p2c::client::util
{
	// Browser process: takes ownership of the kernel pipe handle passed on the command line and
	// clears its inheritance. Call before CefInitialize, so no process CEF launches can inherit it.
	// Returns an empty handle, and logs why, when the value is 0 or inheritance cannot be cleared;
	// the UI then runs with no kernel link.
	::pmon::util::win::Handle TakeKernelPipeHandle(uint64_t pipeHandleValue);

	// Browser process: the UI's one connection to the kernel. It holds the client end of the pipe
	// pair the kernel created for this UI, relays the page's calls to the kernel, and forwards the
	// kernel's events to the page. Lives from just after CefInitialize until just before CefShutdown.
	class KernelLink
	{
	public:
		// pipe comes from TakeKernelPipeHandle. If it is empty or the session cannot be opened, the
		// UI still runs with no kernel link and calls are refused.
		KernelLink(::pmon::util::win::Handle pipe, kernel_relay::SignalRelay::BrowserGetter getBrowser);
		kernel_relay::CallRelay& GetCallRelay();
		// service and middleware versions for the renderer, passed with browser creation
		CefRefPtr<CefDictionaryValue> MakeBrowserExtraInfo() const;
	private:
		static std::unique_ptr<CefClient> OpenSession_(::pmon::util::win::Handle pipe, const kernel_relay::SignalRelay& signals);
		kernel_relay::SignalRelay signals_;
		std::unique_ptr<CefClient> pClient_;
		kernel_relay::CallRelay calls_;
	};
}
