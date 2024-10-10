#pragma once
#include "../CommonUtilities/reg/Registry.h"
#include "../CommonUtilities/log/Level.h"

using namespace pmon::util;
struct Reg : public reg::RegistryBase<Reg>
{
	Value<log::Level> logLevel{ this, "logLevel" };
	Value<std::string> logDir{ this, "logDir" };

	static constexpr const wchar_t* keyPath_ = LR"(SOFTWARE\INTEL\PresentMon\Service)";
};