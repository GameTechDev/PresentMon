#pragma once
#include <memory>

namespace pmapi
{
	class Session;
}

int ServiceCrashClientTest(std::unique_ptr<pmapi::Session> pSession);
