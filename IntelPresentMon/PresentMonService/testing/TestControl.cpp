#include "TestControl.h"
#include "../PresentMon.h"
#include "../Service.h"
#include "../ActionServer.h"
#include <iostream>
#include <sstream>
#include <cereal/archives/json.hpp>

namespace pmon::svc::testing
{
	TestControlModule::TestControlModule(const PresentMon* pPmon, Service* pService, const ActionServer* pActionServer)
		:
		pPresentMon_{ pPmon },
		pService_{ pService },
		pActionServer_{ pActionServer },
		worker_{ &TestControlModule::Run_, this }
	{}
	void TestControlModule::Run_()
	{
		std::string line;
		// wait for ping before entering main execution mode
		while (std::getline(std::cin, line)) {
			if (line == "%ping") {
				WriteResponse_("ping-ok");
				break;
			}
			else {
				WriteResponse_("err-expect-ping");
			}
		}
		// command execution loop
		while (std::getline(std::cin, line)) {
			if (line == "%quit") {
				SetEvent(pService_->GetServiceStopHandle());
				WriteResponse_("quit-ok");
				break;
			}
			else if (line == "%status") {
				auto status = pPresentMon_->GetTestingStatus();
				if (pActionServer_) {
					status.actionSessionCount = pActionServer_->GetSessionCount();
					status.actionAcceptorCount = pActionServer_->GetAcceptorCount();
				}
				std::ostringstream oss;
				cereal::JSONOutputArchive{ oss }(status);
				WriteResponse_(oss.str());
			}
			else {
				WriteResponse_("err-bad-command");
			}
		}
	}
	void TestControlModule::WriteResponse_(const std::string& payload)
	{
		std::cout << std::format("%%{{{}}}%%\n", payload) << std::flush;
	}
}