#include "DiagnosticDriver.h"
#include "../Exception.h"
#include "../win/WinAPI.h"
#include "PanicLogger.h"
#include <iostream>
#include <span>
#include <algorithm>

using namespace std::chrono_literals;
namespace rn = std::ranges;

namespace pmon::util::log
{
	DiagnosticDriver::DiagnosticDriver(const PM_DIAGNOSTIC_CONFIGURATION* pConfig)
	{
		if (pConfig) {
			config_ = DiagnosticConfig{ *pConfig };
		}
	}
	DiagnosticDriver::~DiagnosticDriver()
	{
		dying_ = true;
		try { manualUnblockEvent_.Set(); }
		catch (...) { pmlog_panic_("Failed to set the manual unblock event"); }
	}
	void DiagnosticDriver::Submit(const Entry& e)
	{
		// ignore non-diagnostic entries and low-priority levels
		if ((!e.diagnosticLayer_ && !config_.disableDiagnosticFilter)
			|| int(e.level_) > int(config_.filterLevel)) {
			return;
		}
		// filtering by subsystem
		if (!config_.subsystems.empty() && !rn::contains(config_.subsystems,
			(PM_DIAGNOSTIC_SUBSYSTEM)e.subsystem_)) {
			return;
		}
		// prepare string payloads
		std::string timestamp;
		if (config_.enableTimestamp) {
			const auto zt = std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ };
			timestamp = std::format("{}", zt);
		}
		std::string trace;
		if (config_.enableTrace) {
			if (e.pTrace_ && e.pTrace_->Resolved()) {
				trace = e.pTrace_->ToString();
			}
		}
		std::string location;
		if (config_.enableLocation) {
			location = std::format("{}({})", e.GetSourceFileName(), e.sourceLine_);
		}
		// create diagnostic message based on logging entry
		auto pMessage = std::make_unique<DiagnosticMessage>();
		// set values
		pMessage->level = (PM_DIAGNOSTIC_LEVEL)e.level_;
		pMessage->system = (PM_DIAGNOSTIC_SUBSYSTEM)e.subsystem_;
		pMessage->pid = e.pid_;
		pMessage->tid = e.tid_;
		// handle buffers
		pMessage->messageBuffer_ = e.note_;
		pMessage->locationBuffer_ = std::move(location);
		pMessage->timestampBuffer_ = std::move(timestamp);
		pMessage->traceBuffer_ = std::move(trace);
		pMessage->SyncBuffers_();
		// process message
		ProcessCommon_(std::move(pMessage));
	}
	void DiagnosticDriver::Flush() {}
	uint32_t DiagnosticDriver::GetQueuedMessageCount()
	{
		return (uint32_t)messageQueue_.size_approx();
	}
	uint32_t DiagnosticDriver::GetMaxQueuedMessages()
	{
		return maxQueuedMessages_;
	}
	void DiagnosticDriver::SetMaxQueuedMessages(uint32_t max)
	{
		maxQueuedMessages_ = max;
	}
	uint32_t DiagnosticDriver::GetDiscardedMessageCount()
	{
		return discardedCount_;
	}
	std::unique_ptr<DiagnosticMessage> DiagnosticDriver::DequeueMessage()
	{
		messageWaitEvent_.Reset();
		std::unique_ptr<DiagnosticMessage> pMessage;
		messageQueue_.try_dequeue(pMessage);
		return pMessage;
	}
	void DiagnosticDriver::EnqueueMessage(const PM_DIAGNOSTIC_MESSAGE* pMessage)
	{
		if (!pMessage) {
			throw Except<Exception>("Diagnostic message injection with null message pointer");
		}
		ProcessCommon_(std::make_unique<DiagnosticMessage>(*pMessage));
	}
	PM_DIAGNOSTIC_WAKE_REASON DiagnosticDriver::WaitForMessage(uint32_t timeoutMs)
	{
		if (GetQueuedMessageCount() > 0) {
			return PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE;
		}
		const auto wakeSignal = timeoutMs ?
			win::WaitAnyEventFor(timeoutMs * 1ms, messageWaitEvent_, manualUnblockEvent_) :
			win::WaitAnyEvent(messageWaitEvent_, manualUnblockEvent_);
		if (!wakeSignal) {
			return PM_DIAGNOSTIC_WAKE_REASON_TIMEOUT;
		}
		if (*wakeSignal == 0) {
			return PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE;
		}
		if (*wakeSignal == 1) {
			return dying_ ?
				PM_DIAGNOSTIC_WAKE_REASON_SHUTDOWN :
				PM_DIAGNOSTIC_WAKE_REASON_MANUAL_UNBLOCK;
		}
		throw Except<Exception>("Bad event index waiting on message+unblock events");
	}
	void DiagnosticDriver::UnblockWaitingThread()
	{
		manualUnblockEvent_.Set();
	}
	void DiagnosticDriver::Enqueue_(std::unique_ptr<DiagnosticMessage> pMsg)
	{
		// ensure that # of queued does not exceed max
		while (GetQueuedMessageCount() >= GetMaxQueuedMessages()) {
			std::unique_ptr<DiagnosticMessage> p;
			if (messageQueue_.try_dequeue(p)) {
				discardedCount_++;
			}
		}
		// enqueue the message
		messageQueue_.enqueue(std::move(pMsg));
		// signal message availability
		messageWaitEvent_.Set();
	}
	void DiagnosticDriver::ProcessCommon_(std::unique_ptr<DiagnosticMessage> pMsg)
	{
		using namespace std::string_literals;
		// handle direct output
		std::string formattedMsg;
		auto format = [&] { if (formattedMsg.empty()) {
			auto level = GetLevelName((Level)pMsg->level);
			auto sys = GetSubsystemName((Subsystem)pMsg->system);
			if (pMsg->pTimestamp) {
				formattedMsg = std::format("[PMON:{} @{}] <{}:{}> {{{}}} {}\n", sys, level, pMsg->pid, pMsg->tid, pMsg->pTimestamp, pMsg->pText);
			}
			else {
				formattedMsg = std::format("[PMON:{} @{}] <{}:{}> {}\n", sys, level, pMsg->pid, pMsg->tid, pMsg->pText);
			}
		} };
		// output formatted string directly to active media
		if (config_.outputFlags & PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER) {
			format();
			OutputDebugStringA(formattedMsg.c_str());
		}
		if (config_.outputFlags & PM_DIAGNOSTIC_OUTPUT_FLAGS_STDERR) {
			format();
			std::cerr << formattedMsg;
		}
		if (config_.outputFlags & PM_DIAGNOSTIC_OUTPUT_FLAGS_STDOUT) {
			format();
			std::cout << formattedMsg;
		}
		// handle queue
		if (config_.outputFlags & PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE) {
			Enqueue_(std::move(pMsg));
		}
	}
	DiagnosticConfig::DiagnosticConfig(const PM_DIAGNOSTIC_CONFIGURATION& src)
		:
		PM_DIAGNOSTIC_CONFIGURATION{ src }
	{
		if (src.pSubsystems) {
			subsystems.append_range(std::span(src.pSubsystems, src.nSubsystems));
			nSubsystems = (uint32_t)subsystems.size();
		}
	}
	DiagnosticConfig::DiagnosticConfig()
		:
		PM_DIAGNOSTIC_CONFIGURATION{
			.filterLevel{ PM_DIAGNOSTIC_LEVEL_WARNING },
			.outputFlags{ PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER },
			.pSubsystems{ nullptr },
			.nSubsystems{ 0 },
			.enableTimestamp{ false },
			.enableTrace{ PM_DIAGNOSTIC_LEVEL_NONE },
			.enableLocation{ false },
			.disableDiagnosticFilter{ false },
			.verboseModuleBitset{ 0 },
			.enableSynchronousLogging{ false },
			.enableFlushOnCrash{ false }
		}
	{}
	DiagnosticMessage::DiagnosticMessage(const PM_DIAGNOSTIC_MESSAGE& msg)
		:
		// copies value-based fields, pointers will be set in subsequent SyncBuffers_() call
		PM_DIAGNOSTIC_MESSAGE{ msg }
	{
		if (!msg.pText) {
			throw Except<Exception>("Diagnostic message text pointer not set");
		}
		messageBuffer_ = msg.pText;
		if (msg.pTimestamp) {
			timestampBuffer_ = msg.pTimestamp;
		}
		if (msg.pTrace) {
			traceBuffer_ = msg.pTrace;
		}
		if (msg.pLocation) {
			locationBuffer_ = msg.pLocation;
		}
		SyncBuffers_();
	}
	DiagnosticMessage::DiagnosticMessage() : PM_DIAGNOSTIC_MESSAGE{}
	{}
	void DiagnosticMessage::SyncBuffers_()
	{
		if (!messageBuffer_.empty()) {
			pText = messageBuffer_.c_str();
		}
		if (!timestampBuffer_.empty()) {
			pTimestamp = timestampBuffer_.c_str();
		}
		if (!traceBuffer_.empty()) {
			pTrace = traceBuffer_.c_str();
		}
		if (!locationBuffer_.empty()) {
			pLocation = locationBuffer_.c_str();
		}
	}
}
