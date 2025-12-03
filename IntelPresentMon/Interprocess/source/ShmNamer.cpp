#pragma once
#include "ShmNamer.h"
#include <format>
#include <random>

namespace pmon::ipc
{
	ShmNamer::ShmNamer(std::optional<std::string> salt, std::optional<std::string> customPrefix)
		:
		salt_{ salt.value_or(std::format("{:08x}", std::random_device{}())) },
		prefix_{ customPrefix.value_or(R"(Global\pm2sh)") }
	{}
	std::string ShmNamer::MakeIntrospectionName() const
	{
		return std::format("{}_{}_int", prefix_, salt_);
	}
	std::string ShmNamer::MakeSystemName() const
	{
		return std::format("{}_{}_sys", prefix_, salt_);
	}
	std::string ShmNamer::MakeGpuName(uint32_t deviceId) const
	{
		return std::format("{}_{}_gpu_{}", prefix_, salt_, deviceId);
	}
	std::string ShmNamer::MakeFrameName(uint32_t pid) const
	{
		return std::format("{}_{}_tgt_{}", prefix_, salt_, pid);
	}
	const std::string& ShmNamer::GetSalt() const
	{
		return salt_;
	}
	const std::string& ShmNamer::GetPrefix() const
	{
		return prefix_;
	}
}