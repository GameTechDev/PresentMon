#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../../Core/source/infra/LoggingConfigurationStore.h"
#include "../../CommonUtilities/Exception.h"
#include <cstdint>
#include <optional>
#include <string>

#define ACT_NAME SetDiagnosticLogging
#define ACT_EXEC_CTX KernelExecutionContext
#define ACT_TYPE AsyncActionBase_
#define ACT_NS kproc::kact

namespace ACT_NS
{
	using namespace ::pmon::ipc::act;

	class ACT_NAME : public ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>
	{
	public:
		static constexpr const char* Identifier = STRINGIFY(ACT_NAME);

		enum class Mode : uint32_t
		{
			Patch = 0,
			RestoreDefaults = 1,
			SaveAsDefault = 2,
		};

		struct Params
		{
			Mode mode = Mode::Patch;
			std::optional<::pmon::util::log::Level> logLevel;
			std::optional<::pmon::util::log::Level> traceLevel;
			std::optional<uint64_t> verboseModules;
			std::optional<std::string> folder;
			std::optional<bool> exceptionTrace;
			bool persist = true;

			template<class A> void serialize(A& ar) {
				ar(mode, logLevel, traceLevel, verboseModules, folder, exceptionTrace, persist);
			}
		};
		struct Response
		{
			template<class A> void serialize(A&) {}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;

		static ::p2c::LoggingConfigFieldPatch ToPatch_(const Params& in)
		{
			return {
				.logLevel = in.logLevel,
				.traceLevel = in.traceLevel,
				.verboseModules = in.verboseModules,
				.folder = in.folder,
				.exceptionTrace = in.exceptionTrace,
			};
		}

		static Response Execute_(const ACT_EXEC_CTX&, SessionContext&, Params&& in)
		{
			auto& store = ::p2c::LoggingConfigurationStore::Get();
			switch (in.mode) {
			case Mode::RestoreDefaults:
				store.RestoreDefaults();
				return {};
			case Mode::SaveAsDefault:
				store.SaveCurrentAsDefault(ToPatch_(in));
				return {};
			case Mode::Patch:
				store.ApplyPatch(ToPatch_(in), in.persist);
				return {};
			default:
				throw ::pmon::util::Except<::pmon::util::Exception>("Invalid diagnostic logging mode");
			}
		}
	};

	ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
