#pragma once
#include "../../CommonUtilities/Exception.h"
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <sstream>

// TODO: move this component to common utilities

namespace pmon::ipc
{
	class PmStatusError : public util::Exception {
	public:
		PmStatusError(PM_STATUS code);
		PmStatusError(PM_STATUS code, std::string msg);
		std::string ComposeWhatString_() const noexcept override;
		PM_STATUS GetCode() const noexcept;
		PM_STATUS GeneratePmStatus() const noexcept override;
		bool HasPmStatus() const noexcept override;
	private:
		PM_STATUS code_;
	};

	inline PmStatusError::PmStatusError(PM_STATUS code) :
		Exception{ "PresentMon error in service or middleware" },
		code_{ code } {}

	inline PmStatusError::PmStatusError(PM_STATUS code, std::string msg) :
		Exception{ std::move(msg) },
		code_{ code } {}

	inline std::string PmStatusError::ComposeWhatString_() const noexcept
	{
		try {
			std::ostringstream oss;
			oss << GetNote() << " CODE:[" << code_ << "]";
			if (HasTrace()) {
				oss << "\n" << GetTraceString();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}

	inline PM_STATUS PmStatusError::GetCode() const noexcept { return code_; }

	inline PM_STATUS PmStatusError::GeneratePmStatus() const noexcept
	{
		return GetCode();
	}

	inline bool PmStatusError::HasPmStatus() const noexcept
	{
		return true;
	}
}