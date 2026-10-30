// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "LogSetup.h"
#include <array>
#include <filesystem>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace p2c::cli
{
	struct Options;
}

namespace p2c
{
	enum class LoggingConfigField : uint32_t
	{
		LogLevel = 0,
		TraceLevel,
		VerboseModules,
		Folder,
		ExceptionTrace,
		Count,
	};

	enum class LoggingConfigSource : uint32_t
	{
		Default = 0,
		Persisted,
		Cli,
		Ui,
	};

	struct LoggingConfigFieldStatus
	{
		LoggingConfigSource source = LoggingConfigSource::Default;
		bool cliOverride = false;
	};

	struct LoggingConfigurationSnapshot
	{
		DiagnosticLoggingConfiguration persisted;
		DiagnosticLoggingConfiguration effective;
		DiagnosticLoggingConfiguration cli;
		std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count> fields{};
		DiagnosticLoggingPaths activePaths;
		bool storeLoaded = false;
	};

	struct LoggingConfigFieldPatch
	{
		std::optional<::pmon::util::log::Level> logLevel;
		std::optional<::pmon::util::log::Level> traceLevel;
		std::optional<uint64_t> verboseModules;
		std::optional<std::string> folder;
		std::optional<bool> exceptionTrace;
	};

	class LoggingConfigurationStore
	{
	public:
		// Schema version. v1 fields: logLevel, traceLevel, verboseModules,
		// folder, exceptionTrace. Version mismatch or format errors
		// reset to defaults and rewrite the store file.
		static constexpr int StoreVersion = 1;

		static LoggingConfigurationStore& Get();
		static DiagnosticLoggingConfiguration MakeDefaults();
		static bool IsValidLevel(::pmon::util::log::Level level);
		static bool IsValidVerboseModules(uint64_t mask);
		static bool TryParseJson(const std::string& jsonText, DiagnosticLoggingConfiguration& out,
			std::string* error = nullptr);
		static std::string ToJson(const DiagnosticLoggingConfiguration& config);
		static DiagnosticLoggingConfiguration MergeLayers(
			const DiagnosticLoggingConfiguration& persisted,
			const DiagnosticLoggingConfiguration& cli,
			const std::array<bool, (size_t)LoggingConfigField::Count>& cliPresent,
			const DiagnosticLoggingConfiguration& ui,
			const std::array<bool, (size_t)LoggingConfigField::Count>& uiPresent,
			std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count>* status = nullptr);
		static std::vector<std::string> BuildCefLaunchArgs(const DiagnosticLoggingConfiguration& effective);
		// appends option followed by the names of modules set in mask (nothing for empty mask)
		static void AppendVerboseModuleArgs(std::vector<std::string>& args, const char* option, uint64_t mask);

		void ResetForTesting(std::filesystem::path storePath = {});
		void InitializeFromDiskAndCli(const cli::Options& opt);
		void LoadFromDiskForTesting();
		LoggingConfigurationSnapshot Snapshot() const;
		DiagnosticLoggingConfiguration GetEffective() const;
		DiagnosticLoggingConfiguration GetPersisted() const;
		DiagnosticLoggingPaths GetActivePaths() const;

		void ApplyPatch(const LoggingConfigFieldPatch& patch, bool persist);
		void RestoreDefaults();
		void SaveCurrentAsDefault(const LoggingConfigFieldPatch& fieldsToSave);
		void ApplyEffectiveToRuntime();

		std::filesystem::path GetStorePath() const;

	private:
		struct PendingLoadDiagnostic_
		{
			::pmon::util::log::Level level;
			std::string message;
		};

		LoggingConfigurationStore() = default;

		void LoadUnlocked_();
		void PersistUnlocked_();
		void RecomputeEffectiveUnlocked_();
		void CaptureCliOverridesUnlocked_(const cli::Options& opt);
		void ApplyRuntimeUnlocked_();
		static std::filesystem::path DefaultStorePath_();

		mutable std::mutex mtx_;
		std::filesystem::path storePath_;
		DiagnosticLoggingConfiguration persisted_ = MakeDefaults();
		DiagnosticLoggingConfiguration cli_ = MakeDefaults();
		DiagnosticLoggingConfiguration ui_ = MakeDefaults();
		DiagnosticLoggingConfiguration effective_ = MakeDefaults();
		std::array<bool, (size_t)LoggingConfigField::Count> cliPresent_{};
		std::array<bool, (size_t)LoggingConfigField::Count> uiPresent_{};
		std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count> fieldStatus_{};
		DiagnosticLoggingPaths activePaths_;
		std::vector<PendingLoadDiagnostic_> pendingLoadDiagnostics_;
		bool initialized_ = false;
	};
}
