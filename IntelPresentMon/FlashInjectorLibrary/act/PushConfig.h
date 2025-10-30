#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "InjectionPointExecutionContext.h"
#include "../Extension/OverlayConfigPack.h"
#include <format>
#include <cereal/types/array.hpp>

#define ACT_NAME PushConfig
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
		struct Params : GfxLayer::Extension::OverlayConfig
		{
			template<class A> void serialize(A& ar) {
				ar(BarSize, BarRightShift, BarColor, RenderBackground, BackgroundColor,
					FlashDuration, UseRainbow, BackgroundSize);
			}
		};
		struct Response {
			template<class A> void serialize(A& ar) {
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			GfxLayer::Extension::OverlayConfigPack::Get().Update(in);
			return {};
		}
	};

	ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE