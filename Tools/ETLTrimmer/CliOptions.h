#pragma once
#include "../../IntelPresentMon/CommonUtilities/cli/CliFramework.h"
#include <ranges>

namespace clio
{
	using namespace pmon::util::cli;
	struct Options : public OptionsBase<Options>
	{
	private: Group gf_{ this, "Files", "Input and output file paths" }; public:
		Option<std::string> inputFile{ this, "--input-file", "", "Path to input file to use", [this](CLI::Option* pOpt) {
			pOpt->required();
		} };
		Option<std::string> outputFile{ this, "--output-file", "", "Path for the trimmed output file. Omitting this enables analysis-only mode" };

	private: Group gt_{ this, "Trimming", "Options for trimming and pruning events" }; public:
		Flag provider{ this, "--provider,-p", "Enable pruning by provider (uses same provider set as PresentMon)" };
		Flag event{ this, "--event,-e", "Enable pruning by event (uses same event set as PresentMon)" };
		Flag trimState{ this, "--trim-state,-s", "Override default behavior that avoids discarding stateful events like process and DC start/stop when trimming by timestamp range" };
		Flag listProcesses{ this, "--list-processes", "Generate a report of all processes ranked by event count" };
		Option<std::pair<uint64_t, uint64_t>> trimRangeQpc{ this, "--trim-range-qpc", {}, "Range of QPC timestamps outside of which to trim", RemoveCommas };
		Option<std::pair<uint64_t, uint64_t>> trimRangeNs{ this, "--trim-range-ns", {}, "Range of nanosecond times outside of which to trim", RemoveCommas };
		Option<std::pair<double, double>> trimRangeMs{ this, "--trim-range-ms", {}, "Range of millisecond times outside of which to trim" };

		static constexpr const char* description = "Postprocessing tool for trimming and pruning ETL files";
		static constexpr const char* name = "ETLTrimmer.exe";
	private:
		Dependency eventDep_{ event, provider };
		MutualExclusion trimRangeExcl_{ trimRangeQpc, trimRangeNs, trimRangeMs };
	};
}