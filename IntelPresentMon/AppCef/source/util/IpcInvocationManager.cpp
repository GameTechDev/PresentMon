#include "IpcInvocationManager.h"


namespace p2c::client::util
{
	std::unordered_map<std::string, std::function<void(IpcInvocationManager&, IpcInvocationManager::CallbackContext, CefRefPtr<CefV8Value>)>>
		IpcInvocationManager::dispatchBindings_;
	int IpcInvocationManager::nextCallId_ = 0;

	bool IpcInvocationManager::HandleReply(CefProcessMessage& message)
	{
		CEF_REQUIRE_RENDERER_THREAD();
		if (message.GetName() != kernel_relay::kReplyMessage) {
			return false;
		}
		const auto args = message.GetArgumentList();
		const auto callId = args->GetInt(0);
		const auto i = pendingCalls_.find(callId);
		if (i == pendingCalls_.end()) {
			pmlog_dbg("Dropping kernel call reply for an unknown call").pmwatch(callId);
			return true;
		}
		auto onReply = std::move(i->second);
		pendingCalls_.erase(i);
		// no binary value means the Response serialized to no bytes
		onReply(args->GetBool(1), args->GetType(2) == VTYPE_BINARY ? args->GetBinary(2) : nullptr);
		return true;
	}

	void IpcInvocationManager::SendCall_(const std::string& action, CefRefPtr<CefBinaryValue> pParams,
		CefRefPtr<CefFrame> frame, ReplyHandler onReply)
	{
		CEF_REQUIRE_RENDERER_THREAD();
		const auto callId = nextCallId_++;
		auto message = CefProcessMessage::Create(kernel_relay::kCallMessage);
		const auto args = message->GetArgumentList();
		args->SetString(0, action);
		args->SetInt(1, callId);
		// null when Params serialized to no bytes, see kernel_relay::ToBinary
		if (pParams) {
			args->SetBinary(2, std::move(pParams));
		}
		pendingCalls_.emplace(callId, std::move(onReply));
		frame->SendProcessMessage(PID_BROWSER, std::move(message));
	}
}