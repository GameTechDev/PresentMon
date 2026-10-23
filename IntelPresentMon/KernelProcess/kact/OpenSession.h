#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include <format>

#ifdef PM_KERNEL_PROCESS_BUILD
#include <PresentMonAPI2/PresentMonAPI.h>
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
		return std::format("{}.{}.{}", version.major, version.minor, version.patch);
	}

	inline std::string GetMiddlewareApiVersion_()
	{
		try {
			PM_VERSION version{};
			if (const auto status = pmGetApiVersion(&version); status != PM_STATUS_SUCCESS) {
				pmlog_warn("Unable to retrieve middleware API version").pmwatch((int)status);
				return "N/A";
			}
			return FormatMiddlewareApiVersion_(version);
		}
		catch (...) {
			pmlog_error("Exception retrieving middleware API version").no_trace();
			return "N/A";
		}
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
			std::string serviceBuildConfig;
			std::string middlewareBuildId;
			std::string middlewareBuildTime;
			std::string middlewareVersion;
			std::string middlewareBuildConfig;
			std::string middlewareApiVersion;

			template<class A> void serialize(A& ar) {
				ar(kernelPid, serviceBuildId, serviceBuildTime, serviceVersion, serviceBuildConfig,
					middlewareBuildId, middlewareBuildTime, middlewareVersion,
					middlewareBuildConfig, middlewareApiVersion);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;
		static Response Execute_(const ACT_EXEC_CTX& ctx, SessionContext& stx, Params&& in)
		{
			stx.remotePid = in.cefRenderPid;
			Response res{
				.kernelPid = GetCurrentProcessId(),
				.serviceBuildId = "N/A",
				.serviceBuildTime = "N/A",
				.serviceVersion = "N/A",
				.serviceBuildConfig = "N/A",
				.middlewareBuildId = "N/A",
				.middlewareBuildTime = "N/A",
				.middlewareVersion = "N/A",
				.middlewareBuildConfig = "N/A",
				.middlewareApiVersion = "N/A",
			};
#ifdef PM_KERNEL_PROCESS_BUILD
			const auto serviceInfo = (*ctx.ppKernel)->GetServiceBuildInfo();
			res.serviceBuildId = serviceInfo.buildId;
			res.serviceBuildTime = serviceInfo.buildTime;
			res.serviceVersion = serviceInfo.productVersion;
			res.serviceBuildConfig = serviceInfo.buildConfig;

			const auto middlewareInfo = (*ctx.ppKernel)->GetMiddlewareBuildInfo();
			res.middlewareBuildId = middlewareInfo.buildId;
			res.middlewareBuildTime = middlewareInfo.buildTime;
			res.middlewareVersion = middlewareInfo.productVersion;
			res.middlewareBuildConfig = middlewareInfo.buildConfig;

			res.middlewareApiVersion = GetMiddlewareApiVersion_();
#endif
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
