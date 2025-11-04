#include "MultiClient.h"
#include "CliOptions.h"
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include <PresentMonAPIWrapper/FixedQuery.h>
#include <chrono>
#include <iostream>
#include <span>
#include <PresentMonAPI2Tests/TestCommands.h>
#include <cereal/archives/json.hpp>
#include <vincentlaucsb-csv-parser/csv.hpp>
#include <CommonUtilities/IntervalWaiter.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/log/Log.h>

using namespace pmon;
using namespace std::literals;
using namespace pmon::test::client;
using Clock = std::chrono::steady_clock;
namespace rn = std::ranges;

std::vector<std::string> MakeHeader(
	std::span<const PM_QUERY_ELEMENT> qels,
	const pmapi::intro::Root& intro)
{
	std::vector<std::string> headerColumns{ "poll-time"s };
	for (auto& qel : qels) {
		headerColumns.push_back(std::format("{}({})",
			intro.FindMetric(qel.metric).Introspect().GetSymbol(),
			intro.FindEnum(PM_ENUM_STAT).FindKey((int)qel.stat).GetShortName()
		));
	}
	return headerColumns;
}

void WriteRunToCsv(
	const std::string& csvFilePath,
	const std::vector<std::string>& header,
	const std::vector<std::vector<double>>& runRows)
{
	std::ofstream csvStream{ csvFilePath };
	auto csvWriter = csv::make_csv_writer(csvStream);
	csvWriter << header;
	for (auto& row : runRows) {
		csvWriter << row;
	}
}

class BlobReader
{
	struct LookupInfo_
	{
		uint64_t offset;
		PM_DATA_TYPE type;
	};
public:
	BlobReader(std::span<const PM_QUERY_ELEMENT> qels, std::shared_ptr<pmapi::intro::Root> pIntro)
	{
		for (auto& q : qels) {
			qInfo_.push_back({ q.dataOffset, pIntro->FindMetric(q.metric).GetDataTypeInfo().GetPolledType() });
		}
	}
	void Target(const pmapi::BlobContainer& blobs, uint32_t iBlob = 0)
	{
		pFirstByteTarget_ = blobs[iBlob];
	}
	template<typename T>
	T At(size_t iElement)
	{
		const auto off = qInfo_[iElement].offset;
		switch (qInfo_[iElement].type) {
		case PM_DATA_TYPE_BOOL:   return (T)reinterpret_cast<const bool&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_DOUBLE: return    reinterpret_cast<const double&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_ENUM:   return (T)reinterpret_cast<const int&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_INT32:  return (T)reinterpret_cast<const int32_t&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_STRING: return (T)-1;
		case PM_DATA_TYPE_UINT32: return (T)reinterpret_cast<const uint32_t&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_UINT64: return (T)reinterpret_cast<const uint64_t&>(pFirstByteTarget_[off]);
		case PM_DATA_TYPE_VOID:   return (T)-1;
		}
		return (T)-1;
	}
private:
	const uint8_t* pFirstByteTarget_ = nullptr;
	std::vector<LookupInfo_> qInfo_;
};

std::vector<PM_QUERY_ELEMENT> BuildQueryElementSet(const pmapi::intro::Root& intro)
{
	std::vector<PM_QUERY_ELEMENT> qels;
	for (const auto& m : intro.GetMetrics()) {
		// there is no reliable way of distinguishing CPU telemetry metrics from PresentData-based metrics via introspection
		// adding CPU device type is an idea, however that would require changing device id of the cpu metrics from 0 to
		// whatever id is assigned to cpu (probably an upper range like 1024+) and this might break existing code that just
		// hardcodes device id for the CPU metrics; for the time being use a hardcoded blacklist here
		if (rn::contains(std::array{
			PM_METRIC_CPU_UTILIZATION,
			PM_METRIC_CPU_POWER_LIMIT,
			PM_METRIC_CPU_POWER,
			PM_METRIC_CPU_TEMPERATURE,
			PM_METRIC_CPU_FREQUENCY,
			PM_METRIC_CPU_CORE_UTILITY,
			}, m.GetId())) {
			continue;
		}
		// only allow dynamic metrics
		if (m.GetType() != PM_METRIC_TYPE_DYNAMIC && m.GetType() != PM_METRIC_TYPE_DYNAMIC_FRAME) {
			continue;
		}
		// should be exactly 1 device (universal one)
		auto dmi = m.GetDeviceMetricInfo();
		if (dmi.size() != 1) {
			continue;
		}
		// device must be available and 0 (universal)
		if (!dmi.front().IsAvailable() || dmi.front().GetDevice().GetId() != 0) {
			continue;
		}
		// don't work on string data metrics
		if (m.GetDataTypeInfo().GetPolledType() == PM_DATA_TYPE_STRING) {
			continue;
		}
		for (const auto& s : m.GetStatInfo()) {
			// skip displayed fps (max) as it is broken now
			if (m.GetId() == PM_METRIC_DISPLAYED_FPS && s.GetStat() == PM_STAT_MAX) {
				continue;
			}
			qels.push_back(PM_QUERY_ELEMENT{ m.GetId(), s.GetStat() });
		}
	}
	return qels;
}

class TestClientModule
{
public:
	TestClientModule(std::unique_ptr<pmapi::Session> pSession, double windowMs,
		double offsetMs)
		:
		pSession_{ std::move(pSession) },
		pIntro_{ pSession_->GetIntrospectionRoot() },
		qels_{ BuildQueryElementSet(*pIntro_) },
		query_{ pSession_->RegisterDynamicQuery(qels_, windowMs, offsetMs) },
		blobs_{ query_.MakeBlobContainer(1) }
	{}
	const pmapi::intro::Root& GetIntrospection() const
	{
		return *pIntro_;
	}
	std::span<const PM_QUERY_ELEMENT> GetQueryElements() const
	{
		return qels_;
	}
	void SetETWFlushPeriod(uint32_t ms)
	{
		pSession_->SetEtwFlushPeriod(ms);
	}
	void SetTelemetryPeriod(uint32_t ms)
	{
		pSession_->SetTelemetryPollingPeriod(0, ms);
	}
	std::vector<std::vector<double>> RecordPolling(uint32_t targetPid, double recordingStartSec,
		double recordingStopSec, double pollInterval)
	{
		// start tracking target
		auto tracker = pSession_->TrackProcess(targetPid);
		// get the waiter and the timer clocks ready
		using Clock = std::chrono::high_resolution_clock;
		const auto startTime = Clock::now();
		util::IntervalWaiter waiter{ pollInterval, 0.001 };
		// run polling loop and poll into vector
		std::vector<std::vector<double>> rows;
		std::vector<double> cells;
		BlobReader br{ qels_, pIntro_ };
		br.Target(blobs_);
		const auto recordingStart = recordingStartSec * 1s;
		const auto recordingStop = recordingStopSec * 1s;
		for (auto now = Clock::now(), start = Clock::now();
			now - start <= recordingStop; now = Clock::now()) {
			// skip recording while time has not reached start time
			if (now - start >= recordingStart) {
				cells.reserve(qels_.size() + 1);
				query_.Poll(tracker, blobs_);
				// first column is the time as measured in polling loop
				cells.push_back(std::chrono::duration<double>(now - start).count());
				// remaining columns are from the query
				for (size_t i = 0; i < qels_.size(); i++) {
					cells.push_back(br.At<double>(i));
				}
				rows.push_back(std::move(cells));
			}
			waiter.Wait();
		}
		return rows;
	}
private:
	std::unique_ptr<pmapi::Session> pSession_;
	std::shared_ptr<pmapi::intro::Root> pIntro_;
	std::vector<PM_QUERY_ELEMENT> qels_;
	pmapi::DynamicQuery query_;
	pmapi::BlobContainer blobs_;
};

int PacedPlaybackTest(std::unique_ptr<pmapi::Session> pSession)
{
	auto& opt = clio::Options::Get();

	std::optional<PM_STATUS> errorStatus;

	try {
		if (!opt.processId) {
			pmlog_error("need pid");
		}

		// connect to service and register query
		TestClientModule client{ std::move(pSession), *opt.windowSize, *opt.metricOffset };
		if (opt.etwFlushPeriodMs) {
			client.SetETWFlushPeriod(*opt.etwFlushPeriodMs);
		}
		if (opt.telemetryPeriodMs) {
			client.SetTelemetryPeriod(*opt.telemetryPeriodMs);
		}

		// ping gate to sync on init finished
		std::string line;
		std::getline(std::cin, line);
		if (line != "%ping") {
			std::cout << "%%{ping-error}%%" << std::endl;
			return -1;
		}
		std::cout << "%%{ping-ok}%%" << std::endl;

		// poll for designated period and parse run results
		const auto runTimeEnd = *opt.runStart + *opt.runTime;
		auto run = client.RecordPolling(*opt.processId, *opt.runStart, runTimeEnd, *opt.pollPeriod);
		WriteRunToCsv(*opt.outputPath, MakeHeader(client.GetQueryElements(), client.GetIntrospection()), run);
	}
	catch (const pmapi::ApiErrorException& e) {
		if (!opt.testExpectError) {
			throw;
		}
		errorStatus = e.GetCode();
	}

	std::string line;

	// if we captured an error, wait here for error ack
	if (errorStatus) {
		std::getline(std::cin, line);
		if (line != "%err-check") {
			std::cout << "%%{err-check-error}%%" << std::endl;
			return -1;
		}
		auto&& err = pmapi::EnumMap::GetKeyMap(PM_ENUM_STATUS)->at(*errorStatus).narrowSymbol;
		std::cout << "%%{err-check-ok:" << err << "}%%" << std::endl;
	}

	// wait for command
	while (std::getline(std::cin, line)) {
		if (line == "%quit") {
			std::cout << "%%{quit-ok}%%" << std::endl;
			std::this_thread::sleep_for(25ms);
			return 0;
		}
		//else if (line == "%get-frames") {
		//	FrameResponse resp;
		//	if (!opt.runTime) {
		//		resp.status = "get-frames-err:not-recorded";
		//	}
		//	else {
		//		resp.status = "get-frames-ok";
		//		resp.frames = frames;
		//	}
		//	std::ostringstream oss;
		//	cereal::JSONOutputArchive{ oss }(resp);
		//	std::cout << "%%{" << oss.str() << "}%%" << std::endl;
		//}
		else {
			std::cout << "%%{err-bad-command}%%" << std::endl;
		}
	}

	return -1;
}