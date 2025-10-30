#pragma once
#include <IntelPresentMon/PresentMonAPI2/PresentMonDiagnostics.h>
#include <span>
#include <functional>
#include <memory>

namespace pmapi
{
	// DiagnosticHandler simplifies the process of setting up the debug diagnostic layer
	// when you supply a callback, this class will spawn a thread to process messages from
	// the diagnostic queue (sleeping while none are available)
	// make sure that any resources accessed by this callback are done so in a thread-safe manner
	// and refrain from retaining references to the message as it is destroyed after handling
	class DiagnosticHandler
	{
	public:
		DiagnosticHandler(
			// ignore all messages greater than filterLevel
			PM_DIAGNOSTIC_LEVEL filterLevel = PM_DIAGNOSTIC_LEVEL_WARNING,
			// bitmask of destinations to transmit diagnostics to
			int outputFlags = PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER,
			// callback that will process messages received into the queue
			std::function<void(const PM_DIAGNOSTIC_MESSAGE&)> callback = {},
			// add a grace period to wait for final messages before destroying diagnostics (ms)
			int gracePeriodMs = 10,
			// span of subsystems to allow, ignore all not in this span (empty span to accept all)
			std::span<PM_DIAGNOSTIC_SUBSYSTEM> allowList = {},
			// capture timestamps as a string and add to all non-queue outputs
			bool enableTimestamp = true,
			// capture stack traces as a string when available (typically for error-level messages)
			bool enableTrace = false,
			// capture source file and line number as a string
			bool enableLocation = false
		);
		DiagnosticHandler(const DiagnosticHandler& other) = delete;
		DiagnosticHandler(DiagnosticHandler&& other) = delete;
		DiagnosticHandler& operator=(const DiagnosticHandler& other) = delete;
		~DiagnosticHandler();
	private:
		std::shared_ptr<void> pImpl;
		int gracePeriodMs_;
	};
}