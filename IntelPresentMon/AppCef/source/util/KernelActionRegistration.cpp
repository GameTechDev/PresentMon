#include "IpcInvocationManager.h"
#include "../../../KernelProcess/kact/AllActions.h"
namespace p2c::client::util::kact {
	template<class A>
	struct IpcActionRegistrator {
		IpcActionRegistrator() { ::p2c::client::util::IpcInvocationManager::RegisterDispatchBinding<A>(); }
	};

	IpcActionRegistrator<kproc::kact::EnumerateAdapters> reg_ibind_EnumerateAdapters_;
	IpcActionRegistrator<kproc::kact::Introspect> reg_ibind_Introspect_;
	IpcActionRegistrator<kproc::kact::PushSpecification> reg_ibind_PushSpecification_;
	IpcActionRegistrator<kproc::kact::SetAdapter> reg_ibind_SetAdapter_;
	IpcActionRegistrator<kproc::kact::SetCapture> reg_ibind_SetCapture_;
	IpcActionRegistrator<kproc::kact::BindHotkey> reg_ibind_BindHotkey_;
	IpcActionRegistrator<kproc::kact::ClearHotkey> reg_ibind_ClearHotkey_;
}

