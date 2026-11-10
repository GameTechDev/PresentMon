// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "LoggingConfigurationStore.h"
#include <Core/source/cli/CliOptions.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <CommonUtilities/Exception.h>
#include <CommonUtilities/file/TempFile.h>
#include <CommonUtilities/log/Log.h>
#include <CommonUtilities/log/Verbose.h>
#include <CommonUtilities/str/String.h>
#include <nlohmann/json.hpp>
#include <fstream>
#include <format>

namespace p2c
{
	using namespace ::pmon::util;
	using namespace ::pmon::util::log;
	namespace fs = std::filesystem;
	using json = nlohmann::json;

	namespace
	{
		constexpr const char* kStoreFileName = "logging.json";

		struct LoggingAuditState_
		{
			DiagnosticLoggingConfiguration persisted;
			DiagnosticLoggingConfiguration effective;
			std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count> fields;
		};

		const char* FieldName_(LoggingConfigField field)
		{
			switch (field) {
			case LoggingConfigField::LogLevel: return "logLevel";
			case LoggingConfigField::TraceLevel: return "traceLevel";
			case LoggingConfigField::VerboseModules: return "verboseModules";
			case LoggingConfigField::Folder: return "folder";
			case LoggingConfigField::ExceptionTrace: return "exceptionTrace";
			default: return "unknown";
			}
		}

		std::string FieldValue_(LoggingConfigField field, const DiagnosticLoggingConfiguration& config)
		{
			switch (field) {
			case LoggingConfigField::LogLevel:
				return GetLevelName(config.logLevel);
			case LoggingConfigField::TraceLevel:
				return GetLevelName(config.traceLevel);
			case LoggingConfigField::VerboseModules:
				return std::format("0x{:08X}", config.verboseModules);
			case LoggingConfigField::Folder:
				return config.folder.empty() ? "<default>" : str::ToNarrow(config.folder.wstring());
			case LoggingConfigField::ExceptionTrace:
				return config.exceptionTrace ? "true" : "false";
			default:
				return "<unknown>";
			}
		}

		bool FieldValuesEqual_(LoggingConfigField field,
			const DiagnosticLoggingConfiguration& left,
			const DiagnosticLoggingConfiguration& right)
		{
			return FieldValue_(field, left) == FieldValue_(field, right);
		}

		std::string SourceName_(const LoggingConfigFieldStatus& status)
		{
			switch (status.source) {
			case LoggingConfigSource::Default:
				return "default";
			case LoggingConfigSource::Persisted:
				return "persisted";
			case LoggingConfigSource::Cli:
				return "CLI override";
			case LoggingConfigSource::Ui:
				return status.cliOverride ? "UI; CLI override superseded" : "UI";
			default:
				return "unknown";
			}
		}

		bool StatusEqual_(const LoggingConfigFieldStatus& left,
			const LoggingConfigFieldStatus& right)
		{
			return left.source == right.source
				&& left.cliOverride == right.cliOverride;
		}

		std::string EffectiveField_(LoggingConfigField field,
			const DiagnosticLoggingConfiguration& config,
			const LoggingConfigFieldStatus& status)
		{
			return std::format("{} [{}]", FieldValue_(field, config), SourceName_(status));
		}

		std::string EffectiveState_(const LoggingAuditState_& state)
		{
			std::string text = "{";
			for (size_t i = 0; i < (size_t)LoggingConfigField::Count; ++i) {
				if (i != 0) {
					text += ", ";
				}
				const auto field = (LoggingConfigField)i;
				text += std::format("{}={}", FieldName_(field),
					EffectiveField_(field, state.effective, state.fields[i]));
			}
			text += "}";
			return text;
		}

		std::string UiChangeMessage_(const char* operation,
			const LoggingAuditState_& before,
			const LoggingAuditState_& after)
		{
			std::string changes;
			for (size_t i = 0; i < (size_t)LoggingConfigField::Count; ++i) {
				const auto field = (LoggingConfigField)i;
				const bool effectiveChanged =
					!FieldValuesEqual_(field, before.effective, after.effective)
					|| !StatusEqual_(before.fields[i], after.fields[i]);
				const bool persistedChanged =
					!FieldValuesEqual_(field, before.persisted, after.persisted);
				if (!effectiveChanged && !persistedChanged) {
					continue;
				}
				if (!changes.empty()) {
					changes += "; ";
				}
				changes += FieldName_(field);
				if (effectiveChanged) {
					changes += std::format(": effective {} -> {}",
						EffectiveField_(field, before.effective, before.fields[i]),
						EffectiveField_(field, after.effective, after.fields[i]));
				}
				if (persistedChanged) {
					changes += std::format("{}persisted {} -> {}",
						effectiveChanged ? ", " : ": ",
						FieldValue_(field, before.persisted),
						FieldValue_(field, after.persisted));
				}
			}
			if (changes.empty()) {
				changes = "<none>";
			}
			return std::format(
				"Diagnostic logging changed by UI: operation={}, changes={{ {} }}, state={}",
				operation, changes, EffectiveState_(after));
		}

		bool TryParseLevel_(const json& node, Level& out)
		{
			if (node.is_number_integer()) {
				out = (Level)node.get<int>();
				return LoggingConfigurationStore::IsValidLevel(out);
			}
			if (node.is_string()) {
				auto map = GetLevelMapNarrow();
				const auto key = str::ToLower(node.get<std::string>());
				const auto it = map.find(key);
				if (it == map.end()) {
					return false;
				}
				out = it->second;
				return true;
			}
			return false;
		}

		bool TryReadConfigObject_(const json& obj, DiagnosticLoggingConfiguration& out, std::string* error)
		{
			auto fail = [&](const std::string& msg) {
				if (error) {
					*error = msg;
				}
				return false;
			};
			if (!obj.is_object()) {
				return fail("logging configuration root must be an object");
			}

			auto cfg = LoggingConfigurationStore::MakeDefaults();
			if (obj.contains("logLevel")) {
				if (!TryParseLevel_(obj.at("logLevel"), cfg.logLevel)) {
					return fail("invalid logLevel");
				}
			}
			if (obj.contains("traceLevel")) {
				if (!TryParseLevel_(obj.at("traceLevel"), cfg.traceLevel)) {
					return fail("invalid traceLevel");
				}
			}
			if (obj.contains("verboseModules")) {
				if (!obj.at("verboseModules").is_number_unsigned() && !obj.at("verboseModules").is_number_integer()) {
					return fail("invalid verboseModules");
				}
				cfg.verboseModules = obj.at("verboseModules").get<uint64_t>();
				if (!LoggingConfigurationStore::IsValidVerboseModules(cfg.verboseModules)) {
					return fail("verboseModules mask out of range");
				}
			}
			if (obj.contains("folder")) {
				if (!obj.at("folder").is_string()) {
					return fail("invalid folder");
				}
				cfg.folder = str::ToWide(obj.at("folder").get<std::string>());
			}
			if (obj.contains("exceptionTrace")) {
				if (!obj.at("exceptionTrace").is_boolean()) {
					return fail("invalid exceptionTrace");
				}
				cfg.exceptionTrace = obj.at("exceptionTrace").get<bool>();
			}
			out = std::move(cfg);
			return true;
		}

		json ConfigToJsonObject_(const DiagnosticLoggingConfiguration& config)
		{
			return json{
				{ "version", LoggingConfigurationStore::StoreVersion },
				{ "logLevel", str::ToLower(GetLevelName(config.logLevel)) },
				{ "traceLevel", str::ToLower(GetLevelName(config.traceLevel)) },
				{ "verboseModules", config.verboseModules },
				{ "folder", str::ToNarrow(config.folder.wstring()) },
				{ "exceptionTrace", config.exceptionTrace },
			};
		}

		uint64_t VerboseMaskFromModules_(const std::vector<V>& modules)
		{
			uint64_t mask = 0;
			for (auto mod : modules) {
				const auto bit = (uint32_t)mod;
				if (bit >= (uint32_t)V::Count) {
					continue;
				}
				mask |= (1ull << bit);
			}
			return mask;
		}
	}

	LoggingConfigurationStore& LoggingConfigurationStore::Get()
	{
		static LoggingConfigurationStore store;
		return store;
	}

	DiagnosticLoggingConfiguration LoggingConfigurationStore::MakeDefaults()
	{
		return {};
	}

	bool LoggingConfigurationStore::IsValidLevel(Level level)
	{
		switch (level) {
		case Level::None:
		case Level::Fatal:
		case Level::Error:
		case Level::Warning:
		case Level::Info:
		case Level::Performance:
		case Level::Debug:
		case Level::Verbose:
		case Level::Verbose2:
			return true;
		default:
			return false;
		}
	}

	bool LoggingConfigurationStore::IsValidVerboseModules(uint64_t mask)
	{
		constexpr auto moduleCount = (uint32_t)V::Count;
		static_assert(moduleCount <= 64, "verbose module mask is limited to 64 modules");
		constexpr uint64_t validModuleMask = moduleCount == 64 ? ~0ull : (1ull << moduleCount) - 1ull;
		return (mask & ~validModuleMask) == 0;
	}

	bool LoggingConfigurationStore::TryParseJson(const std::string& jsonText,
		DiagnosticLoggingConfiguration& out, std::string* error)
	{
		auto fail = [&](const std::string& msg) {
			if (error) {
				*error = msg;
			}
			return false;
		};
		json doc = json::parse(jsonText, nullptr, false);
		if (doc.is_discarded() || !doc.is_object()) {
			return fail("malformed logging.json");
		}
		if (!doc.contains("version") || !doc.at("version").is_number_integer()) {
			return fail("missing logging.json version");
		}
		const auto version = doc.at("version").get<int>();
		if (version != StoreVersion) {
			return fail(std::format("unsupported logging.json version {}", version));
		}
		return TryReadConfigObject_(doc, out, error);
	}

	std::string LoggingConfigurationStore::ToJson(const DiagnosticLoggingConfiguration& config)
	{
		return ConfigToJsonObject_(config).dump(2);
	}

	DiagnosticLoggingConfiguration LoggingConfigurationStore::MergeLayers(
		const DiagnosticLoggingConfiguration& persisted,
		const DiagnosticLoggingConfiguration& cli,
		const std::array<bool, (size_t)LoggingConfigField::Count>& cliPresent,
		const DiagnosticLoggingConfiguration& ui,
		const std::array<bool, (size_t)LoggingConfigField::Count>& uiPresent,
		std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count>* status)
	{
		DiagnosticLoggingConfiguration effective = persisted;
		auto setStatus = [&](LoggingConfigField field, LoggingConfigSource source, bool hasCli) {
			if (!status) {
				return;
			}
			auto& s = (*status)[(size_t)field];
			s.source = source;
			s.cliOverride = hasCli;
		};

		auto applyField = [&](LoggingConfigField field, auto persistedValue, auto cliValue, auto uiValue, auto setter) {
			const auto idx = (size_t)field;
			const bool hasCli = cliPresent[idx];
			const bool hasUi = uiPresent[idx];
			if (hasUi) {
				setter(effective, uiValue);
				setStatus(field, LoggingConfigSource::Ui, hasCli);
			}
			else if (hasCli) {
				setter(effective, cliValue);
				setStatus(field, LoggingConfigSource::Cli, true);
			}
			else {
				setter(effective, persistedValue);
				setStatus(field, FieldValuesEqual_(field, persisted, MakeDefaults())
					? LoggingConfigSource::Default : LoggingConfigSource::Persisted, false);
			}
		};

		applyField(LoggingConfigField::LogLevel, persisted.logLevel, cli.logLevel, ui.logLevel,
			[](DiagnosticLoggingConfiguration& cfg, Level v) { cfg.logLevel = v; });
		applyField(LoggingConfigField::TraceLevel, persisted.traceLevel, cli.traceLevel, ui.traceLevel,
			[](DiagnosticLoggingConfiguration& cfg, Level v) { cfg.traceLevel = v; });
		applyField(LoggingConfigField::VerboseModules, persisted.verboseModules, cli.verboseModules, ui.verboseModules,
			[](DiagnosticLoggingConfiguration& cfg, uint64_t v) { cfg.verboseModules = v; });
		applyField(LoggingConfigField::Folder, persisted.folder, cli.folder, ui.folder,
			[](DiagnosticLoggingConfiguration& cfg, const fs::path& v) { cfg.folder = v; });
		applyField(LoggingConfigField::ExceptionTrace, persisted.exceptionTrace, cli.exceptionTrace, ui.exceptionTrace,
			[](DiagnosticLoggingConfiguration& cfg, bool v) { cfg.exceptionTrace = v; });
		return effective;
	}

	std::vector<std::string> LoggingConfigurationStore::BuildCefLaunchArgs(
		const DiagnosticLoggingConfiguration& effective)
	{
		std::vector<std::string> args{
			"--p2c-log-level", str::ToLower(GetLevelName(effective.logLevel)),
			"--p2c-log-trace-level", str::ToLower(GetLevelName(effective.traceLevel)),
		};
		if (effective.exceptionTrace) {
			args.push_back("--p2c-trace-exceptions");
		}
		std::error_code folderError;
		if (!effective.folder.empty() &&
			fs::is_directory(effective.folder, folderError)) {
			args.push_back("--p2c-log-folder");
			args.push_back(str::ToNarrow(effective.folder.wstring()));
		}
		AppendVerboseModuleArgs(args, "--p2c-log-verbose-modules", effective.verboseModules);
		return args;
	}

	void LoggingConfigurationStore::AppendVerboseModuleArgs(std::vector<std::string>& args,
		const char* option, uint64_t mask)
	{
		if (mask != 0) {
			args.push_back(option);
			for (uint32_t bit = 0; bit < (uint32_t)V::Count; ++bit) {
				if ((mask & (1ull << bit)) != 0) {
					args.push_back(GetVerboseModuleName((V)bit));
				}
			}
		}
	}

	void LoggingConfigurationStore::ResetForTesting(fs::path storePath)
	{
		std::lock_guard lk{ mtx_ };
		storePath_ = std::move(storePath);
		persisted_ = MakeDefaults();
		cli_ = MakeDefaults();
		ui_ = MakeDefaults();
		effective_ = MakeDefaults();
		cliPresent_ = {};
		uiPresent_ = {};
		fieldStatus_ = {};
		activePaths_ = {};
		pendingLoadDiagnostics_.clear();
		initialized_ = false;
	}

	void LoggingConfigurationStore::InitializeFromDiskAndCli(const cli::Options& opt)
	{
		std::lock_guard lk{ mtx_ };
		if (storePath_.empty()) {
			storePath_ = DefaultStorePath_();
		}
		LoadUnlocked_();
		CaptureCliOverridesUnlocked_(opt);
		RecomputeEffectiveUnlocked_();
		initialized_ = true;
	}

	void LoggingConfigurationStore::LoadFromDiskForTesting()
	{
		std::lock_guard lk{ mtx_ };
		LoadUnlocked_();
		RecomputeEffectiveUnlocked_();
		initialized_ = true;
	}

	LoggingConfigurationSnapshot LoggingConfigurationStore::Snapshot() const
	{
		std::lock_guard lk{ mtx_ };
		return {
			.persisted = persisted_,
			.effective = effective_,
			.cli = cli_,
			.fields = fieldStatus_,
			.activePaths = activePaths_,
			.storeLoaded = initialized_,
		};
	}

	DiagnosticLoggingConfiguration LoggingConfigurationStore::GetEffective() const
	{
		std::lock_guard lk{ mtx_ };
		return effective_;
	}

	DiagnosticLoggingConfiguration LoggingConfigurationStore::GetPersisted() const
	{
		std::lock_guard lk{ mtx_ };
		return persisted_;
	}

	DiagnosticLoggingPaths LoggingConfigurationStore::GetActivePaths() const
	{
		std::lock_guard lk{ mtx_ };
		return activePaths_;
	}

	void LoggingConfigurationStore::ApplyPatch(const LoggingConfigFieldPatch& patch, bool persist)
	{
		std::lock_guard lk{ mtx_ };
		const LoggingAuditState_ before{ persisted_, effective_, fieldStatus_ };
		// validate everything before changing any state so a rejected patch applies nothing
		if ((patch.logLevel && !IsValidLevel(*patch.logLevel)) ||
			(patch.traceLevel && !IsValidLevel(*patch.traceLevel))) {
			throw Except<Exception>("Invalid diagnostic logging level");
		}
		if (patch.verboseModules && !IsValidVerboseModules(*patch.verboseModules)) {
			throw Except<Exception>("Invalid verbose logging module mask");
		}
		auto touch = [&](LoggingConfigField field) {
			uiPresent_[(size_t)field] = true;
		};
		if (patch.logLevel) {
			ui_.logLevel = *patch.logLevel;
			touch(LoggingConfigField::LogLevel);
			if (persist) {
				persisted_.logLevel = *patch.logLevel;
			}
		}
		if (patch.traceLevel) {
			ui_.traceLevel = *patch.traceLevel;
			touch(LoggingConfigField::TraceLevel);
			if (persist) {
				persisted_.traceLevel = *patch.traceLevel;
			}
		}
		if (patch.verboseModules) {
			ui_.verboseModules = *patch.verboseModules;
			touch(LoggingConfigField::VerboseModules);
			if (persist) {
				persisted_.verboseModules = *patch.verboseModules;
			}
		}
		if (patch.folder) {
			ui_.folder = str::ToWide(*patch.folder);
			touch(LoggingConfigField::Folder);
			if (persist) {
				persisted_.folder = ui_.folder;
			}
		}
		if (patch.exceptionTrace) {
			ui_.exceptionTrace = *patch.exceptionTrace;
			touch(LoggingConfigField::ExceptionTrace);
			if (persist) {
				persisted_.exceptionTrace = *patch.exceptionTrace;
			}
		}
		if (persist) {
			PersistUnlocked_();
		}
		RecomputeEffectiveUnlocked_();
		const LoggingAuditState_ after{ persisted_, effective_, fieldStatus_ };
		const auto message = UiChangeMessage_("patch", before, after);
		const bool logBeforeApply = before.effective.logLevel >= Level::Info
			&& after.effective.logLevel < Level::Info;
		if (logBeforeApply) {
			pmlog_info(message);
		}
		ApplyRuntimeUnlocked_();
		if (!logBeforeApply) {
			pmlog_info(message);
		}
	}

	void LoggingConfigurationStore::RestoreDefaults()
	{
		std::lock_guard lk{ mtx_ };
		const LoggingAuditState_ before{ persisted_, effective_, fieldStatus_ };
		persisted_ = MakeDefaults();
		ui_ = MakeDefaults();
		uiPresent_ = {};
		PersistUnlocked_();
		RecomputeEffectiveUnlocked_();
		const LoggingAuditState_ after{ persisted_, effective_, fieldStatus_ };
		const auto message = UiChangeMessage_("restore-defaults", before, after);
		const bool logBeforeApply = before.effective.logLevel >= Level::Info
			&& after.effective.logLevel < Level::Info;
		if (logBeforeApply) {
			pmlog_info(message);
		}
		ApplyRuntimeUnlocked_();
		if (!logBeforeApply) {
			pmlog_info(message);
		}
	}

	void LoggingConfigurationStore::SaveCurrentAsDefault(
		const LoggingConfigFieldPatch& fieldsToSave)
	{
		std::lock_guard lk{ mtx_ };
		const LoggingAuditState_ before{ persisted_, effective_, fieldStatus_ };
		bool any = false;
		if (fieldsToSave.logLevel) {
			persisted_.logLevel = effective_.logLevel;
			any = true;
		}
		if (fieldsToSave.traceLevel) {
			persisted_.traceLevel = effective_.traceLevel;
			any = true;
		}
		if (fieldsToSave.verboseModules) {
			persisted_.verboseModules = effective_.verboseModules;
			any = true;
		}
		if (fieldsToSave.folder) {
			persisted_.folder = effective_.folder;
			any = true;
		}
		if (fieldsToSave.exceptionTrace) {
			persisted_.exceptionTrace = effective_.exceptionTrace;
			any = true;
		}
		if (!any) {
			throw Except<Exception>("No logging fields selected to save as default");
		}
		PersistUnlocked_();
		RecomputeEffectiveUnlocked_();
		const LoggingAuditState_ after{ persisted_, effective_, fieldStatus_ };
		pmlog_info(UiChangeMessage_("save-as-default", before, after));
	}

	void LoggingConfigurationStore::ApplyEffectiveToRuntime()
	{
		std::lock_guard lk{ mtx_ };
		ApplyRuntimeUnlocked_();
		for (const auto& diagnostic : pendingLoadDiagnostics_) {
			switch (diagnostic.level) {
			case Level::Error:
				pmlog_error(diagnostic.message).no_trace();
				break;
			case Level::Warning:
				pmlog_warn(diagnostic.message).no_trace();
				break;
			case Level::Debug:
				pmlog_dbg(diagnostic.message).no_trace();
				break;
			default:
				break;
			}
		}
		pendingLoadDiagnostics_.clear();
		const LoggingAuditState_ state{ persisted_, effective_, fieldStatus_ };
		pmlog_dbg("Initial diagnostic logging settings resolved: " + EffectiveState_(state));
	}

	fs::path LoggingConfigurationStore::GetStorePath() const
	{
		std::lock_guard lk{ mtx_ };
		return storePath_.empty() ? DefaultStorePath_() : storePath_;
	}

	void LoggingConfigurationStore::LoadUnlocked_()
	{
		persisted_ = MakeDefaults();
		std::error_code existsError;
		const bool storeExists = !storePath_.empty() && fs::exists(storePath_, existsError);
		if (existsError) {
			pendingLoadDiagnostics_.push_back({
				Level::Error,
				std::format("Failed to check logging configuration [{}]: {}; using defaults",
					str::ToNarrow(storePath_.wstring()), existsError.message()),
			});
			return;
		}
		if (storeExists) {
			try {
				std::string text;
				{
					// Close before any rewrite; Windows cannot replace a file with an open stream.
					std::ifstream in{ storePath_ };
					if (!in) {
						throw Except<Exception>("Failed to open logging configuration file");
					}
					text.assign(std::istreambuf_iterator<char>{ in }, {});
					if (in.bad()) {
						throw Except<Exception>("Failed while reading logging configuration file");
					}
				}
				std::string error;
				if (TryParseJson(text, persisted_, &error)) {
					return;
				}
				if (error.starts_with("unsupported logging.json version")) {
					pendingLoadDiagnostics_.push_back({
						Level::Warning,
						std::format(
							"Logging configuration version mismatch [{}]: {}; resetting to defaults",
							str::ToNarrow(storePath_.wstring()), error),
					});
				}
				else {
					pendingLoadDiagnostics_.push_back({
						Level::Error,
						std::format(
							"Failed to parse logging configuration [{}]: {}; resetting to defaults",
							str::ToNarrow(storePath_.wstring()), error),
					});
				}
			}
			catch (...) {
				pendingLoadDiagnostics_.push_back({
					Level::Error,
					std::format(
						"Failed to load logging configuration [{}]: {}; resetting to defaults",
						str::ToNarrow(storePath_.wstring()), ReportException().first),
				});
			}
			// Version mismatch or format error: discard persisted content and rewrite defaults.
			persisted_ = MakeDefaults();
			try {
				PersistUnlocked_();
			}
			catch (...) {
				pendingLoadDiagnostics_.push_back({
					Level::Error,
					std::format("Failed to rewrite logging configuration defaults [{}]: {}",
						str::ToNarrow(storePath_.wstring()), ReportException().first),
				});
			}
			return;
		}

		pendingLoadDiagnostics_.push_back({
			Level::Debug,
			std::format("Logging configuration file is missing [{}]; using defaults",
				str::ToNarrow(storePath_.wstring())),
		});
	}

	void LoggingConfigurationStore::PersistUnlocked_()
	{
		if (storePath_.empty()) {
			storePath_ = DefaultStorePath_();
		}
		fs::create_directories(storePath_.parent_path());
		auto temp = file::TempFile::CreateAt(storePath_.parent_path(),
			std::format("logging.{}.tmp", file::TempFile::MakeRandomName()));
		{
			std::ofstream out{ temp.GetPath(), std::ios::trunc | std::ios::binary };
			if (!out) {
				throw Except<Exception>("Failed to open temporary logging.json for write");
			}
			out << ToJson(persisted_);
			out.flush();
			if (!out) {
				throw Except<Exception>("Failed while writing temporary logging.json");
			}
		}
		const auto tempPath = temp.GetPath();
		std::error_code ec;
		fs::rename(tempPath, storePath_, ec);
		if (ec) {
			fs::remove(storePath_, ec);
			ec.clear();
			fs::rename(tempPath, storePath_, ec);
			if (ec) {
				throw Except<Exception>(std::format("Failed to replace logging.json: {}", ec.message()));
			}
		}
		temp.Release();
	}

	void LoggingConfigurationStore::RecomputeEffectiveUnlocked_()
	{
		effective_ = MergeLayers(persisted_, cli_, cliPresent_, ui_, uiPresent_, &fieldStatus_);
		// only override when the folder exists so ResolveLogPath() yields the real default
		// as the fallback for a missing configured folder (e.g. a disconnected drive)
		if (std::error_code ec; !effective_.folder.empty() && fs::is_directory(effective_.folder, ec)) {
			infra::util::FolderResolver::SetLogPathOverride(effective_.folder);
		}
		else {
			infra::util::FolderResolver::SetLogPathOverride({});
		}
	}

	void LoggingConfigurationStore::CaptureCliOverridesUnlocked_(const cli::Options& opt)
	{
		cli_ = MakeDefaults();
		cliPresent_ = {};
		if (opt.logLevel) {
			cli_.logLevel = *opt.logLevel;
			cliPresent_[(size_t)LoggingConfigField::LogLevel] = true;
		}
		if (opt.logTraceLevel) {
			cli_.traceLevel = *opt.logTraceLevel;
			cliPresent_[(size_t)LoggingConfigField::TraceLevel] = true;
		}
		if (opt.logVerboseModules) {
			cli_.verboseModules = VerboseMaskFromModules_(*opt.logVerboseModules);
			cliPresent_[(size_t)LoggingConfigField::VerboseModules] = true;
		}
		if (opt.logFolder) {
			cli_.folder = str::ToWide(*opt.logFolder);
			cliPresent_[(size_t)LoggingConfigField::Folder] = true;
		}
		if (opt.traceExceptions.IsPresent()) {
			cli_.exceptionTrace = *opt.traceExceptions;
			cliPresent_[(size_t)LoggingConfigField::ExceptionTrace] = true;
		}
	}

	void LoggingConfigurationStore::ApplyRuntimeUnlocked_()
	{
		activePaths_ = ConfigureDiagnosticLogging(effective_);
	}

	fs::path LoggingConfigurationStore::DefaultStorePath_()
	{
		return infra::util::FolderResolver::Get().ResolvePath(
			infra::util::FolderResolver::Folder::App, kStoreFileName);
	}
}
