#pragma once
#include "../ActionHelper.h"
#include <format>
#include <ranges>

#define ACTNAME SetTelemetryPeriod

namespace pmon::svc::acts
{
	using namespace ipc::act;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	class ACTNAME : public AsyncActionBase_<ACTNAME, ServiceExecutionContext>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACTNAME);
		struct Params
		{
			uint32_t telemetrySamplePeriodMs;

			template<class A> void serialize(A& ar) {
				ar(telemetrySamplePeriodMs);
			}
		};
		struct Response { template<class A> void serialize(A& ar) {} };
	private:
		friend class AsyncActionBase_<ACTNAME, ServiceExecutionContext>;
		static Response Execute_(const ServiceExecutionContext& ctx, Params&& in)
		{
			ctx.pPmon->SetGpuTelemetryPeriod(in.telemetrySamplePeriodMs);
			pmlog_dbg(std::format("Setting telemetry sample period to {}ms", in.telemetrySamplePeriodMs));
			return {};
		}
	};
}

#ifdef PM_SERVICE_ASYNC_ACTION_REGISTRATION_
ACTION_REG(ACTNAME);
#endif

ACTION_TRAITS_DEF(ACTNAME);

#undef ACTNAME