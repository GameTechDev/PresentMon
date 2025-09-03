#pragma once
#include "IDriver.h"
#include "IChannel.h"
#include <IntelPresentMon/PresentMonAPI2/PresentMonDiagnostics.h>
#include "../win/Event.h"
#include <atomic>
#include <concurrentqueue/concurrentqueue.h>
#include "Entry.h"


namespace pmon::util::log
{
	// TODO: DiagnosticMessage
	struct DiagnosticMessage : public PM_DIAGNOSTIC_MESSAGE
	{
		DiagnosticMessage(const PM_DIAGNOSTIC_MESSAGE& msg);
		DiagnosticMessage();
		void SyncBuffers_();
		std::string messageBuffer_;
		std::string locationBuffer_;
		std::string traceBuffer_;
		std::string timestampBuffer_;
	};

	struct DiagnosticConfig : public PM_DIAGNOSTIC_CONFIGURATION
	{
		DiagnosticConfig(const PM_DIAGNOSTIC_CONFIGURATION& src);
		DiagnosticConfig();
		std::vector<PM_DIAGNOSTIC_SUBSYSTEM> subsystems;
	};

	class DiagnosticDriver : public IDriver
	{
	public:
		DiagnosticDriver(const PM_DIAGNOSTIC_CONFIGURATION* pConfig);
		~DiagnosticDriver();
		void Submit(const Entry&) override;
		void Flush() override;
		uint32_t GetQueuedMessageCount();
		uint32_t GetMaxQueuedMessages();
		void SetMaxQueuedMessages(uint32_t);
		uint32_t GetDiscardedMessageCount();
		std::unique_ptr<DiagnosticMessage> DequeueMessage();
		void EnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage);
		PM_DIAGNOSTIC_WAKE_REASON WaitForMessage(uint32_t timeoutMs);
		void UnblockWaitingThread();

	private:
		// functions
		void Enqueue_(std::unique_ptr<DiagnosticMessage> pMsg);
		void ProcessCommon_(std::unique_ptr<DiagnosticMessage> pMsg);
		// data
		DiagnosticConfig config_;
		std::atomic<uint32_t> maxQueuedMessages_ = 256;
		std::atomic<uint32_t> discardedCount_ = 0;
		win::Event messageWaitEvent_{ false };
		win::Event manualUnblockEvent_{ false };
		std::atomic<bool> dying_ = false;
		moodycamel::ConcurrentQueue<std::unique_ptr<DiagnosticMessage>> messageQueue_;
	};
}
