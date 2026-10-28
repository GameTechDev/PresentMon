#include "ShmNamer.h"
#include <format>
#include <functional>
#include <mutex>
#include <random>

namespace pmon::ipc
{
	namespace
	{
		std::mutex frameGenerationMutex_;
		std::function<std::string()> frameGenerationOverride_;
	}

	ShmNamer::ShmNamer(std::string customPrefix, std::optional<std::string> salt)
		:
		prefix_{ customPrefix },
		salt_{ salt.value_or(std::format("{:08x}", std::random_device{}())) }
	{}
	std::string ShmNamer::MakeIntrospectionName() const
	{
		return std::format("{}_{}_int", prefix_, salt_);
	}
	std::string ShmNamer::MakeIntrospectionReadyName() const
	{
		return std::format("{}_{}_int_ready_evt", prefix_, salt_);
	}
	std::string ShmNamer::MakeSystemName() const
	{
		return std::format("{}_{}_sys", prefix_, salt_);
	}
	std::string ShmNamer::MakeGpuName(uint32_t deviceId) const
	{
		return std::format("{}_{}_gpu_{}", prefix_, salt_, deviceId);
	}
	std::string ShmNamer::MakeFrameName(uint32_t pid, std::string_view generation) const
	{
		return std::format("{}_{}_tgt_{}_{}", prefix_, salt_, pid, generation);
	}
	void ShmNamer::SetFrameGenerationOverride(std::function<std::string()> fn)
	{
		std::scoped_lock lk{ frameGenerationMutex_ };
		frameGenerationOverride_ = std::move(fn);
	}
	void ShmNamer::ClearFrameGenerationOverride()
	{
		std::scoped_lock lk{ frameGenerationMutex_ };
		frameGenerationOverride_ = {};
	}
	std::string ShmNamer::MakeFrameGeneration()
	{
		{
			std::scoped_lock lk{ frameGenerationMutex_ };
			if (frameGenerationOverride_) {
				return frameGenerationOverride_();
			}
		}
		std::random_device rd;
		const uint64_t bits = ((uint64_t)rd() << 32) | (uint64_t)rd();
		return std::format("{:016x}", bits);
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
