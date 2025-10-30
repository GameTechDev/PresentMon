#pragma once
#include "../CommonUtilities/reg/Registry.h"
#include "../CommonUtilities/log/Level.h"
#include "GlobalIdentifiers.h"

using namespace pmon::util;
struct Reg : public reg::RegistryBase<Reg, HKEY_LOCAL_MACHINE>
{
	Value<log::Level> logLevel{ this, "logLevel" };
	Value<std::string> logDir{ this, "logDir" };
	Value<std::string> middlewarePath{ this, pmon::gid::middlewarePathKey };
	Value<uint64_t> logVerboseModules{ this, "logVerboseModules" };

	static constexpr const wchar_t* keyPath_ = pmon::gid::registryPath;
};