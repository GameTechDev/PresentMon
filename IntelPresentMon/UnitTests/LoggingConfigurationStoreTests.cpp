// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT

#include <CommonUtilities/win/WinAPI.h>
#include <CppUnitTest.h>
#include <Core/source/infra/LoggingConfigurationStore.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <CommonUtilities/str/String.h>
#include <CommonUtilities/file/TempFile.h>
#include <CommonUtilities/log/GlobalPolicy.h>
#include <CommonUtilities/log/Level.h>
#include <filesystem>
#include <fstream>
#include <string>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
namespace fs = std::filesystem;

namespace LoggingConfigurationStoreTests
{
	using namespace p2c;
	using ::pmon::util::log::Level;

	namespace
	{
		std::wstring Widen_(const std::string& text)
		{
			return std::wstring(text.begin(), text.end());
		}

		std::string ReadFile_(const fs::path& path)
		{
			std::ifstream in{ path };
			return std::string{ std::istreambuf_iterator<char>{ in }, {} };
		}

		void WriteFile_(const fs::path& path, const std::string& text)
		{
			std::ofstream out{ path, std::ios::trunc | std::ios::binary };
			out << text;
		}
	}

	TEST_CLASS(LoggingConfigurationStoreUnitTests)
	{
	public:
		TEST_METHOD(DefaultsFollowGlobalLoggingPolicy)
		{
			const auto cfg = LoggingConfigurationStore::MakeDefaults();
			Assert::AreEqual((int)::pmon::util::log::GlobalPolicy::GetDefaultLogLevel(),
				(int)cfg.logLevel);
			Assert::AreEqual((int)::pmon::util::log::GlobalPolicy::GetDefaultTraceLevel(),
				(int)cfg.traceLevel);
		}

		TEST_METHOD(ParsesVersionedJson)
		{
			const auto json = R"({
				"version": 1,
				"logLevel": "info",
				"traceLevel": "warning",
				"verboseModules": 5,
				"folder": "C:\\logs",
				"filePrefix": "legacy-key-is-ignored",
				"exceptionTrace": true
			})";
			DiagnosticLoggingConfiguration cfg;
			std::string error;
			Assert::IsTrue(LoggingConfigurationStore::TryParseJson(json, cfg, &error),
				Widen_(error).c_str());
			Assert::AreEqual((int)Level::Info, (int)cfg.logLevel);
			Assert::AreEqual((int)Level::Warning, (int)cfg.traceLevel);
			Assert::AreEqual((uint64_t)5, cfg.verboseModules);
			Assert::AreEqual(std::string{ "C:\\logs" }, cfg.folder.string());
			Assert::IsTrue(cfg.exceptionTrace);
		}

		TEST_METHOD(RejectsMalformedAndVersionMismatch)
		{
			DiagnosticLoggingConfiguration cfg;
			std::string error;
			Assert::IsFalse(LoggingConfigurationStore::TryParseJson("{not-json", cfg, &error));
			Assert::IsFalse(LoggingConfigurationStore::TryParseJson(
				R"({"version":2,"logLevel":"error"})", cfg, &error));
			Assert::IsFalse(LoggingConfigurationStore::TryParseJson(
				R"({"version":0,"logLevel":"error"})", cfg, &error));
			Assert::IsFalse(LoggingConfigurationStore::TryParseJson(
				R"({"version":1,"logLevel":"not-a-level"})", cfg, &error));
			Assert::IsFalse(LoggingConfigurationStore::TryParseJson(
				R"({"version":1,"exceptionTrace":"yes"})", cfg, &error));
		}

		TEST_METHOD(MergePrecedenceUiOverCliOverPersisted)
		{
			auto persisted = LoggingConfigurationStore::MakeDefaults();
			persisted.logLevel = Level::Error;
			persisted.folder = "persist";

			auto cli = LoggingConfigurationStore::MakeDefaults();
			cli.logLevel = Level::Debug;
			cli.folder = "cli";

			auto ui = LoggingConfigurationStore::MakeDefaults();
			ui.logLevel = Level::Verbose;
			ui.folder = "ui";

			std::array<bool, (size_t)LoggingConfigField::Count> cliPresent{};
			std::array<bool, (size_t)LoggingConfigField::Count> uiPresent{};
			cliPresent[(size_t)LoggingConfigField::LogLevel] = true;
			cliPresent[(size_t)LoggingConfigField::Folder] = true;
			uiPresent[(size_t)LoggingConfigField::LogLevel] = true;

			std::array<LoggingConfigFieldStatus, (size_t)LoggingConfigField::Count> status{};
			const auto effective = LoggingConfigurationStore::MergeLayers(
				persisted, cli, cliPresent, ui, uiPresent, &status);

			Assert::AreEqual((int)Level::Verbose, (int)effective.logLevel);
			Assert::IsTrue(fs::path{ "cli" } == effective.folder);
			Assert::AreEqual((uint32_t)LoggingConfigSource::Ui,
				(uint32_t)status[(size_t)LoggingConfigField::LogLevel].source);
			Assert::AreEqual((uint32_t)LoggingConfigSource::Cli,
				(uint32_t)status[(size_t)LoggingConfigField::Folder].source);
			Assert::IsTrue(status[(size_t)LoggingConfigField::LogLevel].cliOverride);
		}

		TEST_METHOD(RoundTripJsonAndAtomicPersistDoesNotWriteCliImplicitly)
		{
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			const auto storePath = dir / "logging.json";

			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(storePath);

			auto cfg = LoggingConfigurationStore::MakeDefaults();
			cfg.logLevel = Level::Info;
			cfg.exceptionTrace = true;
			WriteFile_(storePath, LoggingConfigurationStore::ToJson(cfg));

			DiagnosticLoggingConfiguration loaded;
			Assert::IsTrue(LoggingConfigurationStore::TryParseJson(ReadFile_(storePath), loaded));
			Assert::AreEqual((int)Level::Info, (int)loaded.logLevel);
			Assert::IsTrue(loaded.exceptionTrace);

			{
				auto disk = loaded;
				disk.traceLevel = Level::Debug;
				WriteFile_(storePath, LoggingConfigurationStore::ToJson(disk));
			}
			{
				DiagnosticLoggingConfiguration disk;
				Assert::IsTrue(LoggingConfigurationStore::TryParseJson(ReadFile_(storePath), disk));
				Assert::AreEqual((int)Level::Info, (int)disk.logLevel);
				Assert::AreEqual((int)Level::Debug, (int)disk.traceLevel);
				Assert::IsTrue(disk.exceptionTrace);
			}

			fs::remove_all(dir);
			store.ResetForTesting();
		}

		TEST_METHOD(VersionMismatchResetsToDefaultsAndRewritesStore)
		{
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			const auto storePath = dir / "logging.json";

			WriteFile_(storePath, R"({
				"version": 2,
				"logLevel": "verbose",
				"traceLevel": "error",
				"verboseModules": 0,
				"folder": "",
				"exceptionTrace": false
			})");

			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(storePath);
			store.LoadFromDiskForTesting();

			const auto defaults = LoggingConfigurationStore::MakeDefaults();
			Assert::AreEqual((int)defaults.logLevel, (int)store.GetPersisted().logLevel);
			Assert::AreEqual((int)defaults.traceLevel, (int)store.GetPersisted().traceLevel);

			DiagnosticLoggingConfiguration rewritten;
			Assert::IsTrue(LoggingConfigurationStore::TryParseJson(ReadFile_(storePath), rewritten));
			Assert::AreEqual((int)defaults.logLevel, (int)rewritten.logLevel);
			Assert::AreEqual((int)defaults.traceLevel, (int)rewritten.traceLevel);

			fs::remove_all(dir);
			store.ResetForTesting();
		}

		TEST_METHOD(MalformedFileResetsToDefaultsAndRewritesStore)
		{
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			const auto storePath = dir / "logging.json";
			WriteFile_(storePath, "{not-valid-json");

			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(storePath);
			store.LoadFromDiskForTesting();

			const auto defaults = LoggingConfigurationStore::MakeDefaults();
			Assert::AreEqual((int)defaults.logLevel, (int)store.GetPersisted().logLevel);
			Assert::AreEqual((int)defaults.traceLevel, (int)store.GetPersisted().traceLevel);
			DiagnosticLoggingConfiguration rewritten;
			Assert::IsTrue(LoggingConfigurationStore::TryParseJson(ReadFile_(storePath), rewritten));
			Assert::AreEqual((int)defaults.logLevel, (int)rewritten.logLevel);
			Assert::AreEqual((int)defaults.traceLevel, (int)rewritten.traceLevel);

			fs::remove_all(dir);
			store.ResetForTesting();
		}

		TEST_METHOD(MissingFileUsesDefaultsWithoutCreatingStore)
		{
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			const auto storePath = dir / "logging.json";

			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(storePath);
			store.LoadFromDiskForTesting();

			const auto defaults = LoggingConfigurationStore::MakeDefaults();
			Assert::AreEqual((int)defaults.logLevel, (int)store.GetPersisted().logLevel);
			Assert::AreEqual((int)defaults.traceLevel, (int)store.GetPersisted().traceLevel);
			Assert::IsFalse(fs::exists(storePath));

			fs::remove_all(dir);
			store.ResetForTesting();
		}

		TEST_METHOD(RejectedPatchAppliesNothing)
		{
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(dir / "logging.json");

			// valid level paired with an invalid module mask must not leave the level applied
			const auto before = store.GetPersisted();
			LoggingConfigFieldPatch patch;
			patch.logLevel = before.logLevel == Level::Verbose ? Level::Error : Level::Verbose;
			patch.verboseModules = ~0ull;
			Assert::ExpectException<std::exception>([&] { store.ApplyPatch(patch, true); });
			Assert::AreEqual((int)before.logLevel, (int)store.GetPersisted().logLevel);
			Assert::IsFalse(fs::exists(dir / "logging.json"));

			fs::remove_all(dir);
			store.ResetForTesting();
		}

		TEST_METHOD(BuildCefArgsFromEffective)
		{
			const auto logFolder = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(logFolder);
			auto cfg = LoggingConfigurationStore::MakeDefaults();
			cfg.logLevel = Level::Debug;
			cfg.traceLevel = Level::Warning;
			cfg.exceptionTrace = true;
			cfg.folder = logFolder;
			cfg.verboseModules = 0b101;
			const auto args = LoggingConfigurationStore::BuildCefLaunchArgs(cfg);
			Assert::IsTrue(std::find(args.begin(), args.end(), "--p2c-log-level") != args.end());
			Assert::IsTrue(std::find(args.begin(), args.end(), "debug") != args.end());
			Assert::IsTrue(std::find(args.begin(), args.end(), "--p2c-trace-exceptions") != args.end());
			Assert::IsTrue(std::find(args.begin(), args.end(), "--p2c-log-folder") != args.end());
			Assert::IsTrue(std::find(args.begin(), args.end(), logFolder.string()) != args.end());
			Assert::IsTrue(std::find(args.begin(), args.end(), "--p2c-log-verbose-modules") != args.end());
			fs::remove_all(logFolder);
		}

		TEST_METHOD(BuildCefArgsOmitsMissingLogFolder)
		{
			auto cfg = LoggingConfigurationStore::MakeDefaults();
			cfg.folder = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			const auto args = LoggingConfigurationStore::BuildCefLaunchArgs(cfg);
			Assert::IsTrue(std::find(args.begin(), args.end(), "--p2c-log-folder") == args.end());
		}

		TEST_METHOD(NonAsciiFolderRoundTripsAsUtf8)
		{
			// includes a character outside common ANSI code pages (CJK)
			const auto logFolder = fs::temp_directory_path() /
				(Widen_(::pmon::util::file::TempFile::MakeRandomName()) + L"-\u00E9\u4E2D");
			fs::create_directories(logFolder);
			auto cfg = LoggingConfigurationStore::MakeDefaults();
			cfg.folder = logFolder;

			// persist and reload
			const auto jsonText = LoggingConfigurationStore::ToJson(cfg);
			DiagnosticLoggingConfiguration loaded;
			Assert::IsTrue(LoggingConfigurationStore::TryParseJson(jsonText, loaded));
			Assert::IsTrue(logFolder.wstring() == loaded.folder.wstring());

			// merge from each layer
			std::array<bool, (size_t)LoggingConfigField::Count> cliPresent{};
			std::array<bool, (size_t)LoggingConfigField::Count> uiPresent{};
			const auto defaults = LoggingConfigurationStore::MakeDefaults();
			Assert::IsTrue(logFolder.wstring() == LoggingConfigurationStore::MergeLayers(
				cfg, defaults, cliPresent, defaults, uiPresent, nullptr).folder.wstring());
			uiPresent[(size_t)LoggingConfigField::Folder] = true;
			Assert::IsTrue(logFolder.wstring() == LoggingConfigurationStore::MergeLayers(
				defaults, defaults, cliPresent, cfg, uiPresent, nullptr).folder.wstring());

			// UI process args carry the folder as UTF-8
			const auto args = LoggingConfigurationStore::BuildCefLaunchArgs(cfg);
			const auto it = std::find(args.begin(), args.end(), "--p2c-log-folder");
			Assert::IsTrue(it != args.end() && std::next(it) != args.end());
			Assert::IsTrue(logFolder.wstring() == ::pmon::util::str::ToWide(*std::next(it)));

			fs::remove_all(logFolder);
		}

		TEST_METHOD(MissingConfiguredFolderDoesNotOverrideLogPath)
		{
			using ::p2c::infra::util::FolderResolver;
			const auto dir = fs::temp_directory_path() /
				::pmon::util::file::TempFile::MakeRandomName();
			fs::create_directories(dir);
			const auto storePath = dir / "logging.json";
			const auto logFolder = dir / "logs";
			auto cfg = LoggingConfigurationStore::MakeDefaults();
			cfg.folder = logFolder;
			WriteFile_(storePath, LoggingConfigurationStore::ToJson(cfg));

			auto& store = LoggingConfigurationStore::Get();
			store.ResetForTesting(storePath);
			FolderResolver::SetLogPathOverride({});

			// missing: effective still reports the configured folder, but resolution falls back
			store.LoadFromDiskForTesting();
			Assert::IsTrue(logFolder == store.GetEffective().folder);
			Assert::IsTrue(logFolder != FolderResolver::Get().ResolveLogPath());

			// present: resolution uses the configured folder
			fs::create_directories(logFolder);
			store.LoadFromDiskForTesting();
			Assert::IsTrue(logFolder == FolderResolver::Get().ResolveLogPath());

			FolderResolver::SetLogPathOverride({});
			fs::remove_all(dir);
			store.ResetForTesting();
		}
	};
}
