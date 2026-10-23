#include "IpcInvocationManager.h"
#include "KernelRelay.h"
#include "../../../KernelProcess/kact/AllActions.h"
namespace p2c::client::util::kact {
	// registers both halves of a kernel action: the renderer binding called from the page,
	// and the browser-side relay that forwards it to the kernel
	template<class A>
	struct IpcActionRegistrator {
		IpcActionRegistrator()
		{
			::p2c::client::util::IpcInvocationManager::RegisterDispatchBinding<A>();
			::p2c::client::util::kernel_relay::CallRelay::RegisterAction<A>();
		}
	};

	IpcActionRegistrator<kproc::kact::Introspect> reg_ibind_Introspect_;
	IpcActionRegistrator<kproc::kact::PushSpecification> reg_ibind_PushSpecification_;
	IpcActionRegistrator<kproc::kact::SetCapture> reg_ibind_SetCapture_;
	IpcActionRegistrator<kproc::kact::BindHotkey> reg_ibind_BindHotkey_;
	IpcActionRegistrator<kproc::kact::ClearHotkey> reg_ibind_ClearHotkey_;
	IpcActionRegistrator<kproc::kact::SetEtlLogging> reg_ibind_SetEtlLogging_;
}

