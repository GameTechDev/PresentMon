#include "DiagnosticHandler.h"
#include <IntelPresentMon/PresentMonAPIWrapperCommon/Exception.h>
#include <thread>
#include <atomic>
#include <semaphore>
#include <chrono>

namespace pmapi
{
	namespace
	{
		class DiagnosticHandlerImpl
		{
		public:
			DiagnosticHandlerImpl(std::function<void(const PM_DIAGNOSTIC_MESSAGE&)> callback)
			{
				if (callback) {
					worker_ = std::jthread{ [this, callback = std::move(callback)] {
						try {
							constructionSema_.release();
							while (auto res = pmDiagnosticWaitForMessage(0)) {
								if (res == PM_DIAGNOSTIC_WAKE_REASON_MESSAGE_AVAILABLE) {
									PM_DIAGNOSTIC_MESSAGE* pMsg = nullptr;
									pmDiagnosticDequeueMessage(&pMsg);
									if (pMsg) {
										callback(*pMsg);
										pmDiagnosticFreeMessage(pMsg);
									}
								}
								else {
									break;
								}
							}
						}
						catch (...) {}
					} };
					// block caller until the worker thread is up and running
					constructionSema_.acquire();
				}
			}
			~DiagnosticHandlerImpl()
			{
				pmDiagnosticUnblockWaitingThread();
			}
		private:
			std::binary_semaphore constructionSema_{ 0 };
			std::jthread worker_;
		};
	}
	DiagnosticHandler::DiagnosticHandler(
		PM_DIAGNOSTIC_LEVEL filterLevel,
		int outputFlags,
		std::function<void(const PM_DIAGNOSTIC_MESSAGE&)> callback,
		int gracePeriodMs,
		std::span<PM_DIAGNOSTIC_SUBSYSTEM> allowList,
		bool enableTimestamp,
		bool enableTrace,
		bool enableLocation
	) : gracePeriodMs_{ gracePeriodMs } {
		const PM_DIAGNOSTIC_CONFIGURATION config{
			.filterLevel = filterLevel,
			.outputFlags = outputFlags,
			.pSubsystems = allowList.data(),
			.nSubsystems = (uint32_t)allowList.size(),
			.enableTimestamp = enableTimestamp,
			.enableTrace = enableTrace,
			.enableLocation = enableLocation,
		};
		if (pmDiagnosticSetup(&config) != PM_STATUS_SUCCESS) {
			throw Exception{ "Failure setting up diagnostic layer" };
		}
		pImpl = std::make_shared<DiagnosticHandlerImpl>(std::move(callback));
	}
	DiagnosticHandler::~DiagnosticHandler()
	{
		using namespace std::chrono_literals;
		std::this_thread::sleep_for(gracePeriodMs_ * 1ms);
	}
}