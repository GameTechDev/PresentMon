#pragma once
#include <thread>
#include <string>

class PresentMon;
class Service;

namespace pmon::svc
{
	class ActionServer;
}

namespace pmon::svc::testing
{
	class TestControlModule
	{
	public:
		TestControlModule(const PresentMon* pPresentMon, Service* pService, const ActionServer* pActionServer);
	private:
		void Run_();
		static void WriteResponse_(const std::string& payload);

		const PresentMon* pPresentMon_;
		Service* pService_;
		const ActionServer* pActionServer_;
		std::jthread worker_;
	};
}