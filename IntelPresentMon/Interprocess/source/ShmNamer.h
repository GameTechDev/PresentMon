#pragma once
#include <string>
#include <optional>


namespace pmon::ipc
{
	// encodes the conventions used to name shared memory during creation and opening
	class ShmNamer
	{
	public:
		ShmNamer(std::string customPrefix, std::optional<std::string> salt = {});
		std::string MakeIntrospectionName() const;
		std::string MakeSystemName() const;
		std::string MakeGpuName(uint32_t deviceId) const;
		std::string MakeFrameName(uint32_t pid) const;
		const std::string& GetSalt() const;
		const std::string& GetPrefix() const;
	private:
		std::string prefix_;
		std::string salt_;
	};
}