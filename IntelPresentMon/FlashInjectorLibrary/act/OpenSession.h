#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "InjectionPointExecutionContext.h"
#include <format>

#define ACT_NAME OpenSession
#define ACT_EXEC_CTX InjectionPointExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS inj::act

namespace ACT_NS
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t kernelPid;

			template<class A> void serialize(A& ar) {
				ar(kernelPid);
			}
		};
		struct Response {
			uint32_t injectedLibPid;

			template<class A> void serialize(A& ar) {
				ar(injectedLibPid);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.kernelPid;
			const Response res{ .injectedLibPid = GetCurrentProcessId() };
			pmlog_info(std::format("Injector open action for cli={} inj={}", in.kernelPid, res.injectedLibPid));
			return res;
		}
	};

	ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE