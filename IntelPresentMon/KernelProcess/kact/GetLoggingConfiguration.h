#pragma once
#include "../../Interprocess/source/act/ActionHelper.h"
#include "KernelExecutionContext.h"
#include "../../Core/source/infra/LoggingConfigurationStore.h"
#include "../../CommonUtilities/str/String.h"
#include <cereal/types/array.hpp>
#include <array>
#include <cstdint>
#include <string>

#define ACT_NAME GetLoggingConfiguration
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
		struct Params
		{
			template<class A> void serialize(A&) {}
		};
		struct FieldStatus
		{
			uint32_t source = 0;
			bool cliOverride = false;

			template<class A> void serialize(A& ar) {
				ar(source, cliOverride);
			}
		};
		struct ConfigValues
		{
			::pmon::util::log::Level logLevel = ::pmon::util::log::Level::Error;
			::pmon::util::log::Level traceLevel = ::pmon::util::log::Level::Error;
			uint64_t verboseModules = 0;
			std::string folder;
			bool exceptionTrace = false;

			template<class A> void serialize(A& ar) {
				ar(logLevel, traceLevel, verboseModules, folder, exceptionTrace);
			}
		};
		struct Response
		{
			ConfigValues persisted;
			ConfigValues effective;
			ConfigValues cli;
			std::array<FieldStatus, (size_t)::p2c::LoggingConfigField::Count> fields{};
			std::string clientLogPath;
			std::string middlewareLogPath;
			bool storeLoaded = false;

			template<class A> void serialize(A& ar) {
				ar(persisted, effective, cli, fields, clientLogPath, middlewareLogPath,
					storeLoaded);
			}
		};
	private:
		friend class ACT_TYPE<ACT_NAME, ACT_EXEC_CTX>;

		static ConfigValues ToValues_(const ::p2c::DiagnosticLoggingConfiguration& cfg)
		{
			return {
				.logLevel = cfg.logLevel,
				.traceLevel = cfg.traceLevel,
				.verboseModules = cfg.verboseModules,
				.folder = pmon::util::str::ToNarrow(cfg.folder.wstring()),
				.exceptionTrace = cfg.exceptionTrace,
			};
		}

		static Response Execute_(const ACT_EXEC_CTX&, SessionContext&, Params&&)
		{
			const auto snap = ::p2c::LoggingConfigurationStore::Get().Snapshot();
			Response out{
				.persisted = ToValues_(snap.persisted),
				.effective = ToValues_(snap.effective),
				.cli = ToValues_(snap.cli),
				.clientLogPath = pmon::util::str::ToNarrow(snap.activePaths.client.wstring()),
				.middlewareLogPath = pmon::util::str::ToNarrow(snap.activePaths.middleware.wstring()),
				.storeLoaded = snap.storeLoaded,
			};
			for (size_t i = 0; i < out.fields.size(); ++i) {
				out.fields[i] = {
					.source = (uint32_t)snap.fields[i].source,
					.cliOverride = snap.fields[i].cliOverride,
				};
			}
			return out;
		}
	};

	ACTION_REG();
}

ACTION_TRAITS_DEF();

#undef ACT_NAME
#undef ACT_EXEC_CTX
#undef ACT_NS
#undef ACT_TYPE
