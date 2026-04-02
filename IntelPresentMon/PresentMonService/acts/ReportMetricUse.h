#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include <format>
#include <cereal/cereal.hpp>

#define ACT_NAME ReportMetricUse
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_NS ::pmon::svc::acts
#define ACT_TYPE AsyncActionBase_

namespace pmon::svc::acts
{
	using namespace ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			std::unordered_set<MetricUse> metricUsage;
			template<class A> void serialize(A& ar) {
				ar(CEREAL_NVP(metricUsage));
			}
		};
		struct Response
		{
			template<class A> void serialize(A& ar) {}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			pmlog_verb(pmon::util::log::V::met_use)("ReportMetricUse action payload")
				.pmwatch(stx.remotePid)
				.serialize("reportMetricUse", in);

			stx.metricUsage = std::move(in.metricUsage);
			ctx.UpdateMetricUsage();
			return {};
		}
	};

#ifdef PM_ASYNC_ACTION_REGISTRATION_
	ACTION_REG();
#endif
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
