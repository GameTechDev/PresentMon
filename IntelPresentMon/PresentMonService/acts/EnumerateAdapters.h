#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "../ActionExecutionContext.h"
#include <format>
#include <ranges>

#define ACT_NAME EnumerateAdapters
#define ACT_EXEC_CTX ActionExecutionContext
#define ACT_NS ::pmon::svc::acts
#define ACT_TYPE AsyncActionBase_

namespace pmon::svc::acts
{
	using namespace ipc::act;
	namespace rn = std::ranges;
	namespace vi = rn::views;

	// TODO: remove this action, it is redundant in light of Introspection
	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params {};
		struct Response
		{
			struct Adapter
			{
				uint32_t id;
				PM_DEVICE_VENDOR vendor;
				std::string name;
				double gpuSustainedPowerLimit;
				uint64_t gpuMemorySize;
				uint64_t gpuMemoryMaxBandwidth;

				template<class A> void serialize(A& ar) {
					ar(id, vendor, name, gpuSustainedPowerLimit, gpuMemorySize, gpuMemoryMaxBandwidth);
				}
			};
			std::vector<Adapter> adapters;

			template<class A> void serialize(A& ar) {
				ar(adapters);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			Response out;
			for (auto&&[i, adapter] : ctx.pPmon->EnumerateAdapters() | vi::enumerate) {
				out.adapters.push_back(Response::Adapter{
					.id = (uint32_t)i,
					.vendor = adapter->GetVendor(),
					.name = adapter->GetName(),
					.gpuSustainedPowerLimit = adapter->GetSustainedPowerLimit(),
					.gpuMemorySize = adapter->GetDedicatedVideoMemory(),
					.gpuMemoryMaxBandwidth = adapter->GetVideoMemoryMaxBandwidth(),
				});
			}
			pmlog_dbg(std::format("{} adapters enumerated", out.adapters.size()));
			return out;
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
