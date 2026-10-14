#include "KernelLink.h"
#include <CommonUtilities/Exception.h>

namespace p2c::client::util
{
	::pmon::util::win::Handle TakeKernelPipeHandle(uint64_t pipeHandleValue)
	{
		if (!pipeHandleValue) {
			pmlog_error("No kernel pipe handle was passed; the UI has no kernel link");
			return {};
		}
		::pmon::util::win::Handle pipe{ reinterpret_cast<HANDLE>((uintptr_t)pipeHandleValue) };
		// an inherited handle stays inheritable; it must never reach a process this one launches
		if (!SetHandleInformation(pipe, HANDLE_FLAG_INHERIT, 0)) {
			pmlog_error("Failed to clear inheritance on the kernel pipe handle; the UI has no kernel link").hr();
			return {};
		}
		return pipe;
	}

	KernelLink::KernelLink(::pmon::util::win::Handle pipe, kernel_relay::SignalRelay::BrowserGetter getBrowser)
		:
		signals_{ getBrowser },
		pClient_{ OpenSession_(std::move(pipe), signals_) },
		calls_{ pClient_.get() }
	{}

	kernel_relay::CallRelay& KernelLink::GetCallRelay()
	{
		return calls_;
	}

	CefRefPtr<CefDictionaryValue> KernelLink::MakeBrowserExtraInfo() const
	{
		const std::string unknown = "unknown";
		auto info = CefDictionaryValue::Create();
		info->SetString(kernel_relay::kServiceBuildIdKey, pClient_ ? pClient_->GetServiceBuildId() : unknown);
		info->SetString(kernel_relay::kServiceBuildTimeKey, pClient_ ? pClient_->GetServiceBuildTime() : unknown);
		info->SetString(kernel_relay::kServiceVersionKey, pClient_ ? pClient_->GetServiceVersion() : unknown);
		info->SetString(kernel_relay::kMiddlewareApiVersionKey, pClient_ ? pClient_->GetMiddlewareApiVersion() : unknown);
		return info;
	}

	std::unique_ptr<CefClient> KernelLink::OpenSession_(::pmon::util::win::Handle pipe, const kernel_relay::SignalRelay& signals)
	{
		if (!pipe) {
			// TakeKernelPipeHandle has logged why
			return {};
		}
		try {
			return std::make_unique<CefClient>(std::move(pipe),
				cact::CefExecutionContext{ .pSignalRelay = &signals });
		}
		catch (...) {
			pmlog_error(::pmon::util::ReportException("Failed opening the kernel session; the UI has no kernel link"));
			return {};
		}
	}
}
