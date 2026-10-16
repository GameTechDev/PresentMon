#include "KernelRelay.h"
#include <include/cef_task.h>
#include <include/base/cef_callback.h>
#include <include/wrapper/cef_closure_task.h>
#include <format>

namespace p2c::client::util::kernel_relay
{
	bool CallRelay::HandleMessage(CefProcessMessage& message, CefRefPtr<CefFrame> frame)
	{
		if (message.GetName() != kCallMessage) {
			return false;
		}
		const auto args = message.GetArgumentList();
		const auto action = args->GetString(0).ToString();
		const auto callId = args->GetInt(1);
		if (!pClient_) {
			pmlog_error("Kernel call refused, the UI has no kernel link").pmwatch(action);
			PostReply_(frame, callId, false, nullptr);
			return true;
		}
		const auto i = Forwarders_().find(action);
		if (i == Forwarders_().end()) {
			pmlog_error("Kernel call refused, unknown action").pmwatch(action);
			PostReply_(frame, callId, false, nullptr);
			return true;
		}
		// no binary value means Params serialized to no bytes
		const auto pParams = args->GetType(2) == VTYPE_BINARY ? args->GetBinary(2) : nullptr;
		try {
			i->second(*pClient_, callId, pParams, frame);
		}
		catch (...) {
			pmlog_error(::pmon::util::ReportException(std::format("Failed relaying kernel call [{}]", action)));
			PostReply_(frame, callId, false, nullptr);
		}
		return true;
	}

	std::unordered_map<std::string, CallRelay::Forwarder>& CallRelay::Forwarders_()
	{
		static std::unordered_map<std::string, Forwarder> forwarders;
		return forwarders;
	}

	void CallRelay::PostReply_(CefRefPtr<CefFrame> frame, int callId, bool succeeded, CefRefPtr<CefBinaryValue> pResponse)
	{
		CefPostTask(TID_UI, base::BindOnce(&CallRelay::SendReply_, std::move(frame), callId, succeeded, std::move(pResponse)));
	}

	void CallRelay::SendReply_(CefRefPtr<CefFrame> frame, int callId, bool succeeded, CefRefPtr<CefBinaryValue> pResponse)
	{
		// the page that made the call may have reloaded or closed since
		if (!frame || !frame->IsValid()) {
			pmlog_dbg("Dropping kernel call reply, its frame is gone").pmwatch(callId);
			return;
		}
		auto message = CefProcessMessage::Create(kReplyMessage);
		const auto args = message->GetArgumentList();
		args->SetInt(0, callId);
		args->SetBool(1, succeeded);
		if (pResponse) {
			args->SetBinary(2, std::move(pResponse));
		}
		frame->SendProcessMessage(PID_RENDERER, std::move(message));
	}

	void SignalRelay::Forward(const std::string& key, std::optional<uint32_t> arg) const
	{
		CefPostTask(TID_UI, base::BindOnce(&SignalRelay::Send_, getBrowser_, key, arg.has_value(), arg.value_or(0)));
	}

	void SignalRelay::Send_(BrowserGetter getBrowser, std::string key, bool hasArg, uint32_t arg)
	{
		const auto browser = getBrowser();
		if (!browser) {
			pmlog_warn("Dropping kernel event, no browser yet").pmwatch(key);
			return;
		}
		auto message = CefProcessMessage::Create(kSignalMessage);
		const auto args = message->GetArgumentList();
		args->SetString(0, key);
		if (hasArg) {
			args->SetInt(1, (int)arg);
		}
		browser->GetMainFrame()->SendProcessMessage(PID_RENDERER, std::move(message));
	}
}
