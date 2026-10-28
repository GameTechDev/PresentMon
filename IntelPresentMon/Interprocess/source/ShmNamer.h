#pragma once
#include <functional>
#include <string>
#include <optional>
#include <string_view>


namespace pmon::ipc
{
	// encodes the conventions used to name shared memory during creation and opening
	class ShmNamer
	{
	public:
		ShmNamer(std::string customPrefix, std::optional<std::string> salt = {});
		std::string MakeIntrospectionName() const;
		std::string MakeIntrospectionReadyName() const;
		std::string MakeSystemName() const;
		std::string MakeGpuName(uint32_t deviceId) const;
		std::string MakeFrameName(uint32_t pid, std::string_view generation) const;
		static std::string MakeFrameGeneration();
		// Test hook: when set, MakeFrameGeneration() calls this instead of the default RNG.
		static void SetFrameGenerationOverride(std::function<std::string()> fn);
		static void ClearFrameGenerationOverride();
		const std::string& GetSalt() const;
		const std::string& GetPrefix() const;
	private:
		std::string prefix_;
		std::string salt_;
	};
}
