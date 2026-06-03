#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include <Versioning/BuildId.h>
#include <format>

#ifdef PM_KERNEL_PROCESS_BUILD
#include <PresentMonAPI2/PresentMonAPI.h>
#endif

#ifndef PM_VER_PRODUCT_STR
#define PM_VER_PRODUCT_STR "unknown"
#endif

#define ACT_NAME OpenSession
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS kproc::kact

namespace ACT_NS
{
	using namespace ::pmon::ipc::act;

#ifdef PM_KERNEL_PROCESS_BUILD
	inline std::string FormatMiddlewareApiVersion_(const PM_VERSION& version)
	{
		auto formatted = std::format("{}.{}.{}", version.major, version.minor, version.patch);
		if (version.tag[0] != '\0') {
			formatted += std::format(" {}", version.tag);
		}
		return formatted + std::format(" {} {}", version.hash, version.config);
	}

	inline std::string GetMiddlewareApiVersion_()
	{
		PM_VERSION version{};
		if (pmGetApiVersion(&version) != PM_STATUS_SUCCESS) {
			return "unknown";
		}
		return FormatMiddlewareApiVersion_(version);
	}
#endif

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);
		struct Params
		{
			uint32_t cefRenderPid;

			template<class A> void serialize(A& ar) {
				ar(cefRenderPid);
			}
		};
		struct Response {
			uint32_t kernelPid;
			std::string serviceBuildId;
			std::string serviceBuildTime;
			std::string serviceVersion;
			std::string middlewareApiVersion;

			template<class A> void serialize(A& ar) {
				ar(kernelPid, serviceBuildId, serviceBuildTime, serviceVersion, middlewareApiVersion);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.cefRenderPid;
			const Response res{
				.kernelPid = GetCurrentProcessId(),
				.serviceBuildId = ::pmon::bid::BuildIdLongHash(),
				.serviceBuildTime = ::pmon::bid::BuildIdTimestamp(),
				.serviceVersion = PM_VER_PRODUCT_STR,
#ifdef PM_KERNEL_PROCESS_BUILD
				.middlewareApiVersion = GetMiddlewareApiVersion_(),
#else
				.middlewareApiVersion = "unknown",
#endif
			};
			pmlog_info(std::format("Kernel open action for cef={} krn={}", in.cefRenderPid, res.kernelPid));
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
