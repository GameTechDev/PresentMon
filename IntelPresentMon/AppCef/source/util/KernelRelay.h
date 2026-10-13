#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_browser.h>
#include <include/cef_frame.h>
#include <include/cef_process_message.h>
#include <include/cef_values.h>
#include <cereal/archives/binary.hpp>
#include <cstdint>
#include <functional>
#include <optional>
#include <sstream>
#include <string>
#include <unordered_map>
#include "ActionClientServer.h"
#include "Logging.h"

// The browser process owns the pipe to the kernel; the page runs in a renderer process.
// Kernel calls and kernel events cross between the two as these CEF process messages.
namespace p2c::client::util::kernel_relay
{
	// Payloads are cereal bytes in a binary value. CEF has no empty binary value, so a payload
	// that serializes to nothing (an action whose Params or Response has no fields) is sent as
	// no value at that index; see ToBinary and FromBinary.
	// renderer -> browser. 0: action identifier (string), 1: call id (int), 2: Params (binary)
	inline constexpr const char* kCallMessage = "kern-call";
	// browser -> renderer. 0: call id (int), 1: succeeded (bool), 2: Response (binary, on success)
	inline constexpr const char* kReplyMessage = "kern-reply";
	// browser -> renderer. 0: signal key (string), 1: argument (int, only for signals that take one)
	inline constexpr const char* kSignalMessage = "kern-signal";
	// keys of the browser-creation extra_info dictionary, which carries the versions the kernel
	// reported when the browser process opened its session
	inline constexpr const char* kServiceBuildIdKey = "serviceBuildId";
	inline constexpr const char* kServiceBuildTimeKey = "serviceBuildTime";
	inline constexpr const char* kServiceVersionKey = "serviceVersion";
	inline constexpr const char* kMiddlewareApiVersionKey = "middlewareApiVersion";

	// Params and Response types already serialize with cereal for the pipe; the relay reuses that.
	// Null when value serializes to no bytes: CEF cannot hold an empty binary value.
	template<class T>
	CefRefPtr<CefBinaryValue> ToBinary(const T& value)
	{
		std::ostringstream stream;
		{
			cereal::BinaryOutputArchive archive{ stream };
			archive(value);
		}
		const auto bytes = std::move(stream).str();
		if (bytes.empty()) {
			return nullptr;
		}
		return CefBinaryValue::Create(bytes.data(), bytes.size());
	}
	// pBinary may be null, meaning the value serialized to no bytes
	template<class T>
	T FromBinary(CefBinaryValue* pBinary)
	{
		std::string bytes;
		if (pBinary) {
			bytes.resize(pBinary->GetSize());
			pBinary->GetData(bytes.data(), bytes.size(), 0);
		}
		std::istringstream stream{ std::move(bytes) };
		cereal::BinaryInputArchive archive{ stream };
		T value;
		archive(value);
		return value;
	}

	// Browser process: carries kernel calls from the renderer to the kernel, and the replies back
	// to the frame that made the call.
	class CallRelay
	{
	public:
		// pClient is null when the UI has no kernel link; every call is then refused.
		explicit CallRelay(CefClient* pClient) : pClient_{ pClient } {}
		// Handles a kCallMessage from frame. False for any other message.
		bool HandleMessage(CefProcessMessage& message, CefRefPtr<CefFrame> frame);
		// Registered once per kernel action, beside its renderer-side binding.
		template<class A>
		static void RegisterAction()
		{
			Forwarders_()[A::Identifier] = [](CefClient& client, int callId,
				CefRefPtr<CefBinaryValue> pParams, CefRefPtr<CefFrame> frame) {
				client.DispatchWithContinuation(FromBinary<typename A::Params>(pParams.get()),
					[callId, frame](typename A::Response&& res, std::exception_ptr pex) {
						if (pex) {
							PostReply_(frame, callId, false, nullptr);
						}
						else {
							PostReply_(frame, callId, true, ToBinary(res));
						}
					});
			};
		}
	private:
		using Forwarder = std::function<void(CefClient&, int, CefRefPtr<CefBinaryValue>, CefRefPtr<CefFrame>)>;
		static std::unordered_map<std::string, Forwarder>& Forwarders_();
		// any thread; the reply is sent on the CEF UI thread. pResponse may be null (see ToBinary).
		static void PostReply_(CefRefPtr<CefFrame> frame, int callId, bool succeeded, CefRefPtr<CefBinaryValue> pResponse);
		static void SendReply_(CefRefPtr<CefFrame> frame, int callId, bool succeeded, CefRefPtr<CefBinaryValue> pResponse);
		CefClient* pClient_;
	};

	// Browser process: forwards kernel events to the page. getBrowser runs on the CEF UI thread and
	// returns the current browser, or null before it exists; events with no browser are dropped.
	class SignalRelay
	{
	public:
		using BrowserGetter = CefRefPtr<CefBrowser>(*)();
		explicit SignalRelay(BrowserGetter getBrowser) : getBrowser_{ getBrowser } {}
		// any thread
		void Forward(const std::string& key, std::optional<uint32_t> arg = {}) const;
	private:
		static void Send_(BrowserGetter getBrowser, std::string key, bool hasArg, uint32_t arg);
		BrowserGetter getBrowser_;
	};
}
