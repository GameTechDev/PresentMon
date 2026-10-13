#include "KernelLink.h"
#include <CommonUtilities/Exception.h>

namespace p2c::client::util
{
	KernelLink::KernelLink(uint64_t pipeHandleValue, kernel_relay::SignalRelay::BrowserGetter getBrowser)
		:
		signals_{ getBrowser },
		pClient_{ OpenSession_(pipeHandleValue, signals_) },
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

	std::unique_ptr<CefClient> KernelLink::OpenSession_(uint64_t pipeHandleValue, const kernel_relay::SignalRelay& signals)
	{
		if (!pipeHandleValue) {
			pmlog_error("No kernel pipe handle was passed; the UI has no kernel link");
			return {};
		}
		const auto handle = reinterpret_cast<HANDLE>((uintptr_t)pipeHandleValue);
		// an inherited handle stays inheritable; keep it out of the processes CEF launches
		if (!SetHandleInformation(handle, HANDLE_FLAG_INHERIT, 0)) {
			pmlog_error("Failed to clear inheritance on the kernel pipe handle").hr();
		}
		try {
			return std::make_unique<CefClient>(::pmon::util::win::Handle{ handle },
				cact::CefExecutionContext{ .pSignalRelay = &signals });
		}
		catch (...) {
			pmlog_error(::pmon::util::ReportException("Failed opening the kernel session; the UI has no kernel link"));
			return {};
		}
	}
}
