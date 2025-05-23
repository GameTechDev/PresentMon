#pragma once
#include <Interprocess/source/act/ActionHelper.h>
#include "CefExecutionContext.h"
#include "../../Action.h"
#include <format>

#define ACT_NAME HotkeyFiredAction
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
			int32_t actionId;

			template<class A> void serialize(A& ar) {
				ar(actionId);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static void Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in);
	};
}

#ifdef PM_ASYNC_ACTION_REGISTRATION_
#include <include/cef_task.h> 
#include <include/base/cef_callback.h> 
#include <include/wrapper/cef_closure_task.h> 
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
		CefPostTask(TID_RENDERER, base::BindOnce(&SignalManager::SignalHotkeyFired,
			base::Unretained(ctx.pSignalManager), in.actionId));
	}
}
#endif

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE