#pragma once
#include "DataFetchPack.h"
#include "../pmon/MetricFetcherFactory.h"
#include "../pmon/DynamicQuery.h"
#include "../pmon/Timekeeper.h"
#include "../pmon/metric/MetricFetcher.h"
#include "../infra/Logging.h"
#include "OverlaySpec.h"
#include <CommonUtilities\Hash.h>
#include <unordered_map>
#include <ranges>

namespace std
{
	template<>
	struct hash<p2c::kern::QualifiedMetric>
	{
		size_t operator()(const p2c::kern::QualifiedMetric& q) const
		{
			using namespace pmon::util::hash;
			return HashCombine(
				DualHash(q.metricId, q.arrayIndex),
				DualHash(q.deviceId, q.statId)
			);
		}
	};
}

namespace p2c::kern
{
	namespace rn = std::ranges;
	namespace vi = std::views;
	using v = ::pmon::util::log::V;
	// maps QualifiedMetrics to packs (data sinks + sources)
	// interacts with MetricFetcherFactory (outputs range of QualifiedMetrics, inputs Fetchers and poll obj)
	// manages carry-over of graph data buffers between pushes while refreshing query / fetchers
	// exposes interface that polls sources and fetches data from them into buffers
	class MetricPackMapper
	{
	public:
		void CommitChanges(uint32_t pid, double winSizeMs, double metricOffsetMs, pmon::MetricFetcherFactory& factory)
		{
			// if there was an empty  widget payload, just clear everyting out and bail
			if (usageMap_.empty()) {
				metricPackMap_.clear();
				pQuery_.reset();
				return;
			}
			// eliminate data / packs which are not necessary anymore
			for (auto&& [qmet, pPack] : metricPackMap_) {
				if (auto&& i = usageMap_.find(qmet); i != usageMap_.end()) {
					if (!i->second.graph) {
						pPack.graphData.reset();
					}
					if (!i->second.text) {
						pPack.textData.reset();
					}
				}
				else {
					pPack.graphData.reset();
					pPack.textData.reset();
				}
			}
			std::erase_if(metricPackMap_, [](const auto& e) { return !(e.second.graphData || e.second.textData); });
			for (auto& [qmet, pPack] : metricPackMap_) {
				pPack.dataUnavailable = false;
			}
			for (auto& [qmet, usage] : usageMap_) {
				if (usage.dataUnavailable) {
					metricPackMap_[qmet].dataUnavailable = true;
				}
			}
			usageMap_.clear();
			std::vector<QualifiedMetric> qualifiedMetrics;
			for (auto& [qmet, pPack] : metricPackMap_) {
				if (!pPack.dataUnavailable) {
					qualifiedMetrics.push_back(qmet);
				}
			}
			pmlog_verb(v::core_metric)("Metrics for query build:\n" + [&] { return qualifiedMetrics |
				vi::transform([](auto& q) {return "    " + q.Dump(); }) |
				vi::join_with('\n') | rn::to<std::basic_string>(); }());
			if (!qualifiedMetrics.empty()) {
				auto buildResult = factory.Build(pid, winSizeMs, metricOffsetMs, qualifiedMetrics);
				for (auto& [qmet, pFetcher] : buildResult.fetchers) {
#pragma warning(push)
#pragma warning(disable : 26800)
					metricPackMap_[qmet].pFetcher = std::move(pFetcher);
#pragma warning(pop)
				}
				pQuery_ = std::move(buildResult.pQuery);
			}
			else {
				pQuery_.reset();
			}
			for (auto& [qmet, pPack] : metricPackMap_) {
				if (pPack.dataUnavailable) {
					pPack.pFetcher = std::make_shared<pmon::met::UnavailableMetricFetcher>();
				}
			}
		}
		void AddGraph(const QualifiedMetric& qmet, double timeWindow, bool dataUnavailable = false)
		{
			auto& pPack = metricPackMap_[qmet];
			if (!pPack.graphData) {
				pPack.graphData = std::make_shared<gfx::lay::GraphData>(timeWindow);
				pmlog_verb(v::core_metric)(std::format("AddGraph[new]> {}", qmet.Dump()));
			}
			else if (pPack.graphData->GetWindowSize() != timeWindow) {
				pPack.graphData->Resize(timeWindow);
				pmlog_verb(v::core_metric)(std::format("AddGraph[resize]> {}", qmet.Dump()));
			}
			usageMap_[qmet].graph = true;
			if (dataUnavailable) {
				usageMap_[qmet].dataUnavailable = true;
			}
		}
		void AddReadout(const QualifiedMetric& qmet, bool dataUnavailable = false)
		{
			auto& pPack = metricPackMap_[qmet];
			if (!pPack.textData) {
				pPack.textData = std::make_shared<std::wstring>();
				pmlog_verb(v::core_metric)(std::format("AddReadout[new]> {}", qmet.Dump()));
			}
			usageMap_[qmet].text = true;
			if (dataUnavailable) {
				usageMap_[qmet].dataUnavailable = true;
			}
		}
		void Populate(const pmapi::ProcessTracker& tracker, uint64_t timestamp)
		{
			if (pQuery_) {
				pQuery_->PollWithTimestamp(tracker, timestamp);
			}
			const auto time = pmon::Timekeeper::RelativeToEpoch((int64_t)timestamp);
			for (auto&& [qmet, pPack] : metricPackMap_) {
				(void)qmet;
				pPack.Populate(time);
			}
		}
		DataFetchPack& operator[](const QualifiedMetric& qmet)
		{
			return metricPackMap_.at(qmet);
		}
	private:
		// types
		struct MetricUsage_
		{
			bool graph = false;
			bool text = false;
			bool dataUnavailable = false;
		};
		// data
		std::unordered_map<QualifiedMetric, DataFetchPack> metricPackMap_;
		// we might need a class that encapsulates all pollable sources, including DynamicQuery
		std::shared_ptr<pmon::DynamicQuery> pQuery_;
		// map used to determine which metrics are no longer needed after a push
		// i.e. which ones can carry over, differentiates between graph and readout
		std::unordered_map<QualifiedMetric, MetricUsage_> usageMap_;
	};
}
