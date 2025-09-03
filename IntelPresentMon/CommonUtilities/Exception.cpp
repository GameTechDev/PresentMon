#include "Exception.h"
#include "win/Utilities.h"
#include <sstream>
#include "log/GlobalPolicy.h"
#include "log/StackTrace.h"
#include <format>


#ifndef PM_THROW_SKIP
#ifndef NDEBUG
#define PM_THROW_SKIP 3
#else
#define PM_THROW_SKIP 2
#endif
#endif

namespace pmon::util
{
	Exception::Exception(std::string msg) noexcept : note_{ std::move(msg) } {}
	void Exception::CaptureStackTrace()
	{
		pTrace_ = log::StackTrace::Here(PM_THROW_SKIP);
	}
	const char* Exception::what() const noexcept
	{
		if (buffer_.empty()) {
			buffer_ = ComposeWhatString_();
		}
		return buffer_.c_str();
	}
	std::string Exception::ComposeWhatString_() const noexcept
	{
		try {
			std::ostringstream oss;
			oss << GetNote();
			if (pTrace_) {
				oss << "\n" << GetTraceString();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}
	const std::string& Exception::GetNote() const
	{
		return note_;
	}
	std::string Exception::GetTraceString() const
	{
		if (HasTrace()) {
			pTrace_->Resolve();
			std::ostringstream oss;
			oss << " ====== STACK TRACE (newest on top) ======\n";
			oss << pTrace_->ToString();
			oss << " =========================================\n";
			return oss.str();
		}
		return {};
	}
	bool Exception::HasTrace() const noexcept
	{
		return bool(pTrace_);
	}

	PM_STATUS Exception::GeneratePmStatus() const noexcept
	{
		return PM_STATUS_FAILURE;
	}

	bool Exception::HasPmStatus() const noexcept
	{
		return false;
	}

	void DoCapture_(Exception& e)
	{
		if (log::GlobalPolicy::Get().GetExceptionTrace()) {
			e.CaptureStackTrace();
		}
	}

	std::pair<std::string, std::optional<PM_STATUS>> ReportException(std::string note, std::exception_ptr pEx) noexcept
	{
		if (!pEx) {
			pEx = std::current_exception();
		}
		const auto concat = [&](std::string what) {
			if (!note.empty()) {
				return note + "\n" + what;
			}
			else {
				return what;
			}
		};
		if (pEx) {
			try {
				std::rethrow_exception(pEx);
			}
			catch (const pmon::util::Exception& e) {
				try {
					if (e.HasPmStatus()) {
						return { concat(std::format("[{}] {}", typeid(e).name(), e.what())), e.GeneratePmStatus() };
					}
					else {
						return { concat(std::format("[{}] {}", typeid(e).name(), e.what())), std::nullopt };
					}
				}
				catch (...) { return {}; }
			}
			catch (const std::exception& e) {
				try { return { concat(std::format("[{}] {}", typeid(e).name(), e.what())), std::nullopt }; }
				catch (...) { return {}; }
			}
			catch (...) {
				return { concat("Unrecognized exception"), std::nullopt };
			}
		}
		return { concat("No exception in flight"), std::nullopt };
	}

	PM_STATUS GeneratePmStatus(std::exception_ptr pEx) noexcept
	{
		if (!pEx) {
			pEx = std::current_exception();
		}
		if (pEx) {
			try {
				std::rethrow_exception(pEx);
			}
			catch (const Exception& e) {
				return e.GeneratePmStatus();
			}
			catch (...) {
				return PM_STATUS_FAILURE;
			}
		}
		return PM_STATUS_SUCCESS;
	}

	namespace {
		void seh_trans_func(unsigned int u, EXCEPTION_POINTERS*)
		{
			SehException ex{ u };
			if (log::GlobalPolicy::Get().GetSehTracing()) {
				ex.CaptureStackTrace();
			}
			throw ex;
		}
	}

	void InstallSehTranslator() noexcept
	{
		_set_se_translator(seh_trans_func);
	}

	SehException::SehException(unsigned int code) noexcept : sehCode{ code } {}

	unsigned int SehException::GetSehCode() const noexcept { return sehCode; }

	std::string SehException::ComposeWhatString_() const noexcept
	{
		try {
			std::ostringstream oss;
			oss << std::format("Error Code [0x{:08X}]: {}\n", GetSehCode(), win::GetSEHSymbol(GetSehCode()));
			if (HasTrace()) {
				oss << "\n" << GetTraceString();
			}
			return oss.str();
		}
		catch (...) {}
		return {};
	}
}