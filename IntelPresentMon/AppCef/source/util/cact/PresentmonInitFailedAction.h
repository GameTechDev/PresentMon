#pragma once
#include <Interprocess/source/act/ActionHelper.h>
#include "CefExecutionContext.h"
#include "../../Action.h"
#include <format>

#define ACT_NAME PresentmonInitFailedAction
#define ACT_EXEC_CTX CefExecutionContext
#define ACT_TYPE AsyncEventActionBase_
#define ACT_NS ::p2c::client::util::cact

namespace p2c::client::util::cact
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			template<class A> void serialize(A& ar) {}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static void Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in);
	};
}

#ifdef PM_ASYNC_ACTION_REGISTRATION_
#include "../KernelRelay.h"
#include "../SignalManager.h"
namespace p2c::client::util::cact
{
	ACTION_REG();
	// need to put this function definition only in the cef-server-side since it contains
	// references to CEF types we do not want to pollute other projects with
	// TODO: consider a better approach to maintaining single-file actions like this
	// while also enabling dependency decoupling more easily
	void ACT_NAME::Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
	{
		ctx.pSignalRelay->Forward(SignalManager::Keys::presentmonInitFailed());
	}
}
#endif

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE