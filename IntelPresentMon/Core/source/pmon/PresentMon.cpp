// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMon.h"
#include <Core/source/infra/Logging.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <PresentMonAPI2/PresentMonAPI.h>
#include <PresentMonAPIWrapper/PresentMonAPIWrapper.h>
#include <PresentMonAPIWrapperCommon/EnumMap.h>
#include "RawFrameDataWriter.h"

namespace p2c::pmon
{
	using namespace ::pmapi;

	class FrameEventFlusher
	{
	public:
		FrameEventFlusher(Session& sesh)
		{
			// register minimal query used for flushing frame events
			std::array queryElements{ PM_QUERY_ELEMENT{ PM_METRIC_PRESENT_MODE, PM_STAT_MID_POINT } };
			query_ = sesh.RegisterFrameQuery(queryElements);
			blobs_ = query_.MakeBlobContainer(50);
		}
		void Flush(ProcessTracker& tracker)
		{
			query_.ForEachConsume(tracker, blobs_, [](auto) {});
		}
	private:
		FrameQuery query_;
		BlobContainer blobs_;
	};

	PresentMon::PresentMon(std::optional<std::string> namedPipeName, double window_in, double offset_in, uint32_t telemetrySamplePeriodMs_in)
	{
		const auto RemoveDoubleQuotes = [](std::string s) {
			if (s.front() == '"' && s.back() == '"' && s.size() >= 2) {
				s = s.substr(1, s.size() - 2);
			}
			return s;
		};
		if (namedPipeName) {
			auto pipeName = RemoveDoubleQuotes(*namedPipeName);
			pmlog_info(std::format("Connecting to service with custom pipe [{}]", pipeName));
			pSession = std::make_unique<pmapi::Session>(std::move(pipeName));
		}
		else {
			pmlog_info("Connecting to service with default pipe name");
			pSession = std::make_unique<pmapi::Session>();
		}

		// acquire introspection data
		pIntrospectionRoot = pSession->GetIntrospectionRoot();

		// establish initial sampling period and flush period
		SetGpuTelemetryPeriod(telemetrySamplePeriodMs_in);
		SetEtwFlushPeriod(std::nullopt);

		// create flusher used to clear out piled-up old frame events before capture
		pFlusher = std::make_unique<FrameEventFlusher>(*pSession);
	}
	PresentMon::~PresentMon() = default;
	void PresentMon::StartTracking(uint32_t pid_)
	{
		if (processTracker) {
			if (processTracker.GetPid() == pid_) {
				return;
			}
			pmlog_warn(std::format("Starting stream [{}] while previous stream [{}] still active",
				pid_, processTracker.GetPid()));
		}
		processTracker = pSession->TrackProcess(pid_);
	}
	void PresentMon::StopTracking()
	{
		if (!processTracker) {
			pmlog_warn("Cannot stop stream: no stream active");
			return;
		}
		const auto pid = processTracker.GetPid();
		processTracker.Reset();
		// TODO: caches cleared here maybe
		pmlog_info(std::format("stopped pmon stream for pid {}", pid));
	}
	void PresentMon::SetGpuTelemetryPeriod(uint32_t period)
	{
		pSession->SetTelemetryPollingPeriod(1, period);
		telemetrySamplePeriod = period;
	}
	uint32_t PresentMon::GetGpuTelemetryPeriod()
	{
		return telemetrySamplePeriod;
	}
	std::vector<AdapterInfo> PresentMon::EnumerateAdapters() const
	{
		std::vector<AdapterInfo> infos;
		for (const auto& info : pIntrospectionRoot->GetDevices()) {
			if (info.GetType() != PM_DEVICE_TYPE_GRAPHICS_ADAPTER) {
				continue;
			}
			infos.push_back(AdapterInfo{
				.id = info.GetId(),
				.vendor = info.IntrospectVendor().GetName(),
				.name = info.GetName(),
			});
		}
		return infos;
	}
	void PresentMon::SetAdapter(uint32_t id)
	{
		pmlog_info(std::format("Set active adapter to [{}]", id));
		if (id == 0) {
			pmlog_warn("Adapter was set to id 0; resetting");
			selectedAdapter.reset();
		}
		else {
			selectedAdapter = id;
		}
	}
	void PresentMon::SetEtlLogging(bool active)
	{
		pmlog_info("Setting etl logging").pmwatch(active);
		if (active) {
			if (etlLogger) {
				pmlog_error("Etl logging desync: session already active").pmwatch(etlLogger.GetHandle());
			}
			else {
				etlLogger = pSession->StartEtlLogging();
				pmlog_dbg("Got etl logging handle").pmwatch(etlLogger.GetHandle());
			}
		}
		else {
			if (!etlLogger) {
				pmlog_error("Etl logging desync: no active session");
			}
			else {
				using FR = infra::util::FolderResolver;
				const auto folderPath = FR::Get().Resolve(FR::Folder::Documents, FR::etlSubdirectory);
				const std::chrono::zoned_time now{ std::chrono::current_zone(), std::chrono::system_clock::now() };
				const auto fullPath = std::format(L"{0}\\ipm-{1:%y}{1:%m}{1:%d}-{1:%H}{1:%M}{1:%OS}.etl", folderPath, now);
				etlLogger.Finish(fullPath);
			}
		}
	}
	std::optional<uint32_t> PresentMon::GetPid() const {
		return bool(processTracker) ? processTracker.GetPid() : std::optional<uint32_t>{};
	}
	const pmapi::ProcessTracker& PresentMon::GetTracker() const
	{
		return processTracker;
	}
	std::shared_ptr<RawFrameDataWriter> PresentMon::MakeRawFrameDataWriter(std::wstring path,
		std::optional<std::wstring> statsPath, uint32_t pid, std::wstring procName)
	{
		// flush any buffered present events before starting capture
		pFlusher->Flush(processTracker);

		// make the frame data writer
		return std::make_shared<RawFrameDataWriter>(std::move(path), processTracker, std::move(procName),
			selectedAdapter.value_or(1), *pSession, std::move(statsPath), *pIntrospectionRoot);
	}
	std::optional<uint32_t> PresentMon::GetSelectedAdapter() const
	{
		return selectedAdapter;
	}
	const pmapi::intro::Root& PresentMon::GetIntrospectionRoot() const
	{
		return *pIntrospectionRoot;
	}
	pmapi::Session& PresentMon::GetSession()
	{
		return *pSession;
	}
	void PresentMon::SetEtwFlushPeriod(std::optional<uint32_t> periodMs)
	{
		assert(pSession);
		etwFlushPeriodMs = periodMs;
		pSession->SetEtwFlushPeriod(periodMs.value_or(0));
	}
	std::optional<uint32_t> PresentMon::GetEtwFlushPeriod()
	{
		return etwFlushPeriodMs;
	}
}