#include "TestControl.h"
#include "../PresentMon.h"
#include "../Service.h"
#include <iostream>
#include <sstream>
#include <cereal/archives/json.hpp>

namespace pmon::svc::testing
{
	TestControlModule::TestControlModule(const PresentMon* pPmon, Service* pService)
		:
		pPresentMon_{ pPmon },
		pService_{ pService },
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
				std::ostringstream oss;
				cereal::JSONOutputArchive{ oss }(pPresentMon_->GetTestingStatus());
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