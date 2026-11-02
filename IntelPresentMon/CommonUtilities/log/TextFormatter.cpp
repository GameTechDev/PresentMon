#include "TextFormatter.h"
#include <chrono>
#include <format>
#include <sstream>
#include "Entry.h"
#include "../win/Utilities.h"
#include "../str/String.h"
#include <ranges>
#include "PanicLogger.h"
#include "StackTrace.h"
#include "IdentificationTable.h"
#include "IErrorCodeResolver.h"

namespace pmon::util::log
{
	std::string MakeProc_(const Entry& e)
	{
		std::string text;
		if (auto proc = IdentificationTable::LookupProcess(e.pid_)) {
			text = std::format("{}({})", proc->name, proc->pid);
		}
		else {
			text = std::to_string(e.pid_);
		}
		return text;
	}
	std::string MakeThread_(const Entry& e)
	{
		std::string text;
		if (auto thread = IdentificationTable::LookupThread(e.tid_)) {
			text = std::format("{}({})", thread->name, thread->tid);
		}
		else {
			text = std::to_string(e.tid_);
		}
		return text;
	}


	std::string TextFormatter::Format(const Entry& e) const
	{
		try {
			const auto timestamp = [&] {
				try {
					return std::format("{}", std::chrono::zoned_time{ std::chrono::current_zone(), e.timestamp_ });
				}
				catch (...) {
					return std::format("{:%F %T} UTC", e.timestamp_);
				}
			}();
			std::ostringstream oss;
			oss << std::format("[@{}] <{}:{}> {{{}}}",
				GetLevelName(e.level_),
				MakeProc_(e),
				MakeThread_(e),
				timestamp
			);
			if (!e.note_.empty()) {
				oss << "\n  " << e.note_;
			}
			if (e.errorCode_) {
				auto& ec = e.errorCode_;
				if (ec.IsResolvedNontrivial()) {
					auto pStrings = ec.GetStrings();
					oss << std::format("\n  !{} [{}] ({}): {} => {}", pStrings->type, ec.AsHex(), pStrings->symbol, pStrings->name, pStrings->description);
				}
				else {
					oss << std::format("\n  !UNKNOWN [{}]", ec.AsHex());
				}
			}
			// display of source line info could be controlled here
			// if so, hitcount would need to be separately handled
			if (true) {
				std::visit([&](auto& strings) {
					oss << std::format("\n  >> at {} {}\n     {}({})\n",
						strings.functionName_,
						[&] { return e.hitCount_ == -1 ? std::string{} : std::format("[Hits: {}]", e.hitCount_); }(),
						strings.file_,
						e.sourceLine_
					);
				}, e.sourceStrings_);
			}
			if (e.pTrace_) {
				try {
					oss << " ====== STACK TRACE (newest on top) ======\n";
					oss << e.pTrace_->ToString();
					oss << " =========================================\n";
				}
				catch (...) {
					pmlog_panic_("Failed printing stack trace in TextFormatter::Format");
				}
			}
			return oss.str();
		}
		catch (...) {
			pmlog_panic_("Exception in TextFormatter::Format");
			return {};
		}
	}
}