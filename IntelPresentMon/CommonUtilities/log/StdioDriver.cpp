#include "StdioDriver.h"
#include "Entry.h"
#include "ITextFormatter.h"
#include "PanicLogger.h"
#include "../win/WinAPI.h"
#include <iostream>

namespace pmon::util::log
{
	namespace
	{
		constexpr const char* kAnsiReset_ = "\x1b[0m";

		const char* GetLevelColor_(Level level) noexcept
		{
			switch (level) {
			case Level::Fatal:
				return "\x1b[1;38;5;196m";
			case Level::Error:
				return "\x1b[38;5;196m";
			case Level::Warning:
				return "\x1b[38;5;208m";
			case Level::Info:
				return "\x1b[38;5;33m";
			case Level::Performance:
				return "\x1b[38;5;45m";
			case Level::Debug:
				return "\x1b[38;5;94m";
			case Level::Verbose:
			case Level::Verbose2:
				return "\x1b[38;5;55m";
			case Level::None:
			case Level::EndOfEnumKeys:
			default:
				return kAnsiReset_;
			}
		}

		bool TryEnableVirtualTerminal_(bool useStderr) noexcept
		{
			const auto hOut = GetStdHandle(useStderr ? STD_ERROR_HANDLE : STD_OUTPUT_HANDLE);
			if (hOut == INVALID_HANDLE_VALUE || hOut == nullptr) {
				return false;
			}
			DWORD mode = 0;
			if (!GetConsoleMode(hOut, &mode)) {
				return false;
			}
			if (mode & ENABLE_VIRTUAL_TERMINAL_PROCESSING) {
				return true;
			}
			return !!SetConsoleMode(hOut, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
		}
	}

	StdioDriver::StdioDriver(std::shared_ptr<ITextFormatter> pFormatter, bool useStderr, bool enableColorizedOutput)
		:
		pFormatter_{ std::move(pFormatter) },
		useStderr_{ useStderr },
		colorizedOutput_{ enableColorizedOutput }
	{
		if (colorizedOutput_) {
			colorizedOutput_ = TryEnableVirtualTerminal_(useStderr_);
		}
	}
	void StdioDriver::Submit(const Entry& e)
	{
		if (pFormatter_) {
			const auto formatted = pFormatter_->Format(e);
			if (useStderr_) {
				if (colorizedOutput_) {
					std::cerr << GetLevelColor_(e.level_) << formatted << kAnsiReset_;
				}
				else {
					std::cerr << formatted;
				}
			}
			else {
				if (colorizedOutput_) {
					std::cout << GetLevelColor_(e.level_) << formatted << kAnsiReset_;
				}
				else {
					std::cout << formatted;
				}
			}
		}
		else {
			pmlog_panic_("BasicFileDriver submitted to without a formatter set");
		}
	}
	void StdioDriver::SetFormatter(std::shared_ptr<ITextFormatter> pFormatter)
	{
		pFormatter_ = std::move(pFormatter);
	}
	std::shared_ptr<ITextFormatter> StdioDriver::GetFormatter() const
	{
		return pFormatter_;
	}
	void StdioDriver::Flush()
	{
		if (useStderr_) {
			std::cerr << std::flush;
		}
		else {
			std::cout << std::flush;
		}
	}
}
