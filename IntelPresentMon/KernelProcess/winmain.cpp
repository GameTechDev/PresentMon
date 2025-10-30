#include "../CommonUtilities/win/WinAPI.h"
#include "../Core/source/kernel/Kernel.h"
#include "../Core/source/infra/util/FolderResolver.h"
#include "../Interprocess/source/act/SymmetricActionServer.h"
#include "kact/KernelExecutionContext.h"
#include "../AppCef/source/util/cact/TargetLostAction.h"
#include "../AppCef/source/util/cact/OverlayDiedAction.h"
#include "../AppCef/source/util/cact/PresentmonInitFailedAction.h"
#include "../AppCef/source/util/cact/StalePidAction.h"
#include "../AppCef/source/util/cact/HotkeyFiredAction.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../PresentMonAPIWrapperCommon/EnumMap.h"
#include <Core/source/cli/CliOptions.h>
#include <PresentMonAPI2Loader/Loader.h>
#include <Core/source/infra/LogSetup.h>
#include <CommonUtilities/win/Utilities.h>
#include <CommonUtilities/win/Privileges.h>
#include <CommonUtilities/win/ProcessMapBuilder.h>
#include <Versioning/BuildId.h>
#include <Shobjidl.h>
#include <boost/process/v2/process.hpp>
#include <boost/process/v2/windows/as_user_launcher.hpp>
#include <array>
#include <ranges>
#include <iostream>


using namespace pmon;
namespace vi = std::views;
namespace rn = std::ranges;
using namespace std::literals;
namespace bp2 = boost::process::v2;
namespace as = boost::asio;

namespace kproc
{
	using KernelServer = ipc::act::SymmetricActionServer<kact::KernelExecutionContext>;

	namespace cact = p2c::client::util::cact;

	class KernelHandler : public p2c::kern::KernelHandler
	{
	public:
		KernelHandler(KernelServer& server) : server_{ server } {}
		void OnTargetLost(uint32_t pid) override
		{
			server_.DispatchDetached(cact::TargetLostAction::Params{ pid });
		}
		void OnOverlayDied() override
		{
			server_.DispatchDetached(cact::OverlayDiedAction::Params{});
		}
		void OnPresentmonInitFailed() override
		{
			server_.DispatchDetached(cact::PresentmonInitFailedAction::Params{});
		}
		void OnStalePidSelected() override
		{
			server_.DispatchDetached(cact::StalePidAction::Params{});
		}
	private:
		// data
		KernelServer& server_;
	};
	class HeadlessKernelHandler : public p2c::kern::KernelHandler
	{
	public:
		void OnTargetLost(uint32_t pid) override
		{
			std::cerr << "Target lost.\n";
			stopEvent_.Set();
		}
		void OnOverlayDied() override
		{
			std::cerr << "Tracking terminated.\n";
			stopEvent_.Set();
		}
		void OnPresentmonInitFailed() override
		{
			std::cerr << "PresentMon initialization failed. Check PresentMon service status.\n";
			stopEvent_.Set();
		}
		void OnStalePidSelected() override
		{
			std::cerr << "Target not present.\n";
			stopEvent_.Set();
		}
		// data
		::pmon::util::win::Event stopEvent_;
	};

	bool TryAttachToParentConsole_()
	{
		if (AttachConsole(ATTACH_PARENT_PROCESS)) {
			// Rebind stdout/stderr to the console
			FILE* f;
			freopen_s(&f, "CONOUT$", "w", stdout);
			freopen_s(&f, "CONOUT$", "w", stderr);
			freopen_s(&f, "CONIN$", "r", stdin);
			return true;
		}
		return false;
	}

	void AllocAndBindConsole_()
	{
		AllocConsole();
		FILE* f;
		freopen_s(&f, "CONOUT$", "w", stdout);
		freopen_s(&f, "CONOUT$", "w", stderr);
		freopen_s(&f, "CONIN$", "r", stdin);
	}
}

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	using namespace kproc;
	using namespace p2c;

#ifdef NDEBUG
	constexpr bool is_debug = false;
#else
	constexpr bool is_debug = true;
#endif

	try {
		// if we were run from a parent with a console (terminal?), try to attach there
		const bool fromTerminal = TryAttachToParentConsole_();
		// parse the command line arguments and make them globally available
		if (auto err = cli::Options::Init(__argc, __argv, true)) {
			if (*err == 0) {
				// we don't have a console connection by default, so get one
				if (!fromTerminal) {
					AllocAndBindConsole_();
				}
				std::cout << cli::Options::GetDiagnostics() << std::endl;
				// if we're not run from terminal, make sure created console does not close immediately
				if (!fromTerminal) {
					std::cout << "Press <ENTER> to continue...";
					std::cin.get();
				}
			}
			else {
				if (fromTerminal) {
					std::cerr << cli::Options::GetDiagnostics() << std::endl;
				}
				else {
					MessageBoxA(nullptr, cli::Options::GetDiagnostics().c_str(), "Command Line Parse Error",
						MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
				}
			}
			return *err;
		}
		const auto& opt = cli::Options::Get();
		// do some post validation here
		if (opt.subcCapture.Active()) {
			// make sure target is specified
			if (!opt.capTargetName && !opt.capTargetPid) {
				std::cerr << "Must specify one of --target-name or --target-pid for capture" << std::endl;
				return -1;
			}
		}
		if (opt.subcList.Active()) {
			// make sure target is specified
			if (!opt.listMetrics && !opt.listDevices) {
				std::cerr << "Must specify one of --metrics or --devices for list" << std::endl;
				return -1;
			}
		}
		// pause process to allow for attaching debugger
		if (opt.waitForDebugger) {
			while (!IsDebuggerPresent()) {
				std::this_thread::sleep_for(5ms);
			}
			DebugBreak();
		}
		// resolve files relative to output folder instead of installed/user folders during development
		if (opt.filesWorking) {
			infra::util::FolderResolver::SetDevMode();
		}
		// optionally override the middleware dll path (typically when running from IDE in dev cycle)
		if (auto path = opt.middlewareDllPath.AsOptional()) {
			pmLoaderSetPathToMiddlewareDll_(path->c_str());
		}

		// create logging system and ensure cleanup before main ext
		LogChannelManager zLogMan_;

		// configure the logging system (partially based on command line options)
		ConfigureLogging();

		// determine if we're running headless
		const bool headless = opt.subcCapture.Active() || opt.subcList.Active();

		// pipe logging into stdio when running headless
		if (headless) {
			ConfigureHeadlessLogging();
		}

		// set the app id so that windows get grouped
		// TODO: verify operation when multiple app instances running concurrently
		SetCurrentProcessExplicitAppUserModelID(L"Intel.PresentMon");

		pmlog_info(std::format("== kernel process starting build#{} clean:{} ==",
			bid::BuildIdShortHash(), !bid::BuildIdDirtyFlag()));

		// launch the service as a child process if desired (typically during development)
		const auto logSvcPipe = opt.logSvcPipe.AsOptional().value_or(
			std::format("pm2-child-svc-log-{}", GetCurrentProcessId()));
		as::io_context ioctx;
		std::optional<bp2::basic_process<as::io_context::executor_type>> svcChild;
		if (opt.svcAsChild) {
			// compile fixed CLI options
			auto args = std::vector<std::string>{
				"--control-pipe"s, *opt.controlPipe,
				"--nsm-prefix"s, "pm-frame-nsm"s,
				"--intro-nsm"s, *opt.shmName,
				"--etw-session-name"s, *opt.etwSessionName,
				"--log-level"s, util::log::GetLevelName(util::log::GlobalPolicy::Get().GetLogLevel()),
				"--log-pipe-name"s, logSvcPipe,
				"--enable-stdio-log"s,
			};
			// append verbose module options
			if (opt.logVerboseModules) {
				args.push_back("--log-verbose-modules"s);
				args.append_range(*opt.logVerboseModules | vi::transform(util::log::GetVerboseModuleName));
			}
			// launch service child process
			svcChild = bp2::windows::default_launcher{}(ioctx, "PresentMonService.exe"s, std::move(args));
			// wait for pipe availability of service api
			if (!::pmon::util::win::WaitForNamedPipe(*opt.controlPipe + "-in", 1500)) {
				pmlog_error("timeout waiting for child service control pipe to go online");
				return -1;
			}
		}

		if (opt.logSvcPipeEnable) {
			// connect to service's log pipe (best effort)
			ConnectToLoggingSourcePipe(logSvcPipe);
		}

		//// connect to the middleware diagnostic layer (not generally used by ipm since we connect
		//// to logging directly via copy channel in dev and log middleware to file in production)
		//std::optional<pmapi::DiagnosticHandler> diag;
		//try {
		//    if (opt.enableDiagnostic && opt.cefType && *opt.cefType == "renderer") {
		//        diag.emplace(
		//            (PM_DIAGNOSTIC_LEVEL)opt.logLevel.AsOptional().value_or(log::GlobalPolicy::Get().GetLogLevel()),
		//            PM_DIAGNOSTIC_OUTPUT_FLAGS_DEBUGGER | PM_DIAGNOSTIC_OUTPUT_FLAGS_QUEUE,
		//            [](const PM_DIAGNOSTIC_MESSAGE& msg) {
		//            auto ts = msg.pTimestamp ? msg.pTimestamp : std::string{};
		//            pmlog_(log::Level(msg.level)).note(std::format("@@ D I A G @@ => <{}> {}", ts, msg.pText));
		//        }
		//        );
		//    }
		//} pmcatch_report;

		// if we are just listing, do not launch Kernel, just use API directly here and exit
		if (opt.subcList.Active()) {
			// connect to service
			auto session = [&] {
				if (svcChild || opt.controlPipe) {
					return pmapi::Session{ *opt.controlPipe };
				}
				else {
					return pmapi::Session{};
				}
			}();
			// get introspection data
			auto pIntro = session.GetIntrospectionRoot();
			// get lookup for metric type enum
			auto pMetricTypeLut = pmapi::EnumMap::GetKeyMap(PM_ENUM_METRIC_TYPE);
			// list metrics
			if (opt.listMetrics) {
				std::cout << "List of metrics:\n";
				for (auto&& m : pIntro->GetMetrics()) {
					// filtering
					auto t = m.GetType();
					if (opt.listFilterFrame) {
						if (t == PM_METRIC_TYPE_DYNAMIC) {
							continue;
						}
					}
					if (opt.listFilterDynamic) {
						if (t == PM_METRIC_TYPE_STATIC) {
							continue;
						}
					}
					auto&& s = m.Introspect();
					if (opt.listSearch) {
						auto search = [nd = util::str::ToLower(*opt.listSearch)](const std::string& hs) {
							return util::str::ToLower(hs).contains(nd);
						};
						if (!search(s.GetSymbol()) && !search(s.GetName()) && !search(s.GetDescription())) {
							continue;
						}
					}
					// output
					std::cout << s.GetSymbol() << "  [" << pMetricTypeLut->at(t).narrowName << "]:\n";
					std::cout << "   " << s.GetDescription() << "\n";
					if (opt.listMetricsStats) {
						// output list of stats for this metric if requested
						std::cout << "   Stats: { ";
						for (auto&& s : m.GetStatInfo()) {
							std::cout << s.IntrospectStat().GetSymbol() << " ";
						}
						std::cout << "}\n";
					}
					std::cout << "\n";
				}
			}
			// list adapter devices
			if (opt.listDevices) {
				std::cout << "List of graphics adapters:\n";
				for (auto&& d : pIntro->GetDevices()) {
					if (!d.GetId()) continue;
					std::cout << d.GetName() << " [" << d.GetId() << "] (" << d.IntrospectVendor().GetName() << ")\n";
				}
			}
			return 0;
		}

		// this pointer serves as a way to set the kernel on the server exec context after the server is created
		p2c::kern::Kernel* pKernel = nullptr;
		// active object that creates a window and sinks raw input messages to listen for hotkey presses
		p2c::win::Hotkeys hotkeys;
		// this server receives a connection from the CEF render process
		const auto actName = std::format(R"(\\.\pipe\ipm-cef-channel-{})", GetCurrentProcessId());
		KernelServer server{ kact::KernelExecutionContext{ .ppKernel = &pKernel, .pHotkeys = &hotkeys },
			actName, 1, "D:(A;;GA;;;WD)S:(ML;;NW;;;ME)", headless };
		// set the hotkey manager to send notifications via the action server
		hotkeys.SetHandler([&](int action) {
			server.DispatchDetached(p2c::client::util::cact::HotkeyFiredAction::Params{ .actionId = action });
		});
		// select which handler to use for kernel async events/signals
		auto pKernelHandler = [&]() -> std::unique_ptr<::p2c::kern::KernelHandler> {
			if (headless) {
				// this handler receives events from the kernel and prints to cerr while signaling early exit
				return std::make_unique<HeadlessKernelHandler>();
			}
			else {
				// this handler receives events from the kernel and transmits them to the render process via the server
				return std::make_unique<KernelHandler>(server);
			}
		}();
		// the kernel manages the PresentMon data collection and the overlay rendering
		p2c::kern::Kernel kernel{ pKernelHandler.get(), headless};
		// new we set this pointer, giving the server access to the Kernel
		pKernel = &kernel;
		// run the UI when not headless
		if (!headless) {
			// compose optional cli args for cef process tree
			auto args = std::vector<std::string>{
				opt.filesWorking ? "--p2c-files-working"s : ""s,
				opt.traceExceptions ? "--p2c-trace-exceptions"s : ""s,
				opt.logFolder ? "--p2c-log-folder"s : ""s, *opt.logFolder,
			} | vi::filter(std::not_fn(&std::string::empty)) | rn::to<std::vector>();
			// forward verbose module options
			if (opt.logVerboseModules) {
				args.push_back("--p2c-log-verbose-modules"s);
				args.append_range(*opt.logVerboseModules | vi::transform(util::log::GetVerboseModuleName));
			}
			bool allOriginsAllowed = false;
			for (auto& f : *opt.uiFlags) {
				if (f == "enable-chromium-debug") {
					// needed in order to connect Chrome debuggers to CEF
					args.push_back("--remote-allow-origins=*");
					allOriginsAllowed = true;
				}
				args.push_back("--p2c-" + f);
			}
			for (auto& o : *opt.uiOptions) {
				if (o.first == "url" && is_debug && !allOriginsAllowed) {
					// needed in order to connect Chrome debuggers to CEF
					args.push_back("--remote-allow-origins=*");
				}
				args.push_back("--p2c-" + o.first);
				args.push_back(o.second);
			}
			const auto cefLogPipe = std::format("pm-ui-log-{}", GetCurrentProcessId());
			// add fixed CLI options to the args vector
			args.append_range(std::vector{
				"--p2c-log-level"s, util::log::GetLevelName(*opt.logLevel),
				"--p2c-log-trace-level"s, util::log::GetLevelName(*opt.logTraceLevel),
				"--p2c-act-name"s, actName,
				"--p2c-log-pipe-name"s, cefLogPipe
			});
			// launch the CEF browser process, which in turn launches all the other processes in the CEF process constellation
			auto cefChild = [&] {
				if (util::win::WeAreElevated()) {
					try {
						pmlog_info("detected elevation, attempting integrity downgrade");
						auto mediumTokenPack = util::win::PrepareMediumIntegrityToken();
						return bp2::windows::as_user_launcher{ mediumTokenPack.hMediumToken.Get() }(
							ioctx, "PresentMonUI.exe"s, args
							);
					}
					catch (...) {
						pmlog_warn(util::ReportException("Failed to downgrade integrity, falling back to standard process spawn"));
					}
				}
				return bp2::windows::default_launcher{}(
					ioctx, "PresentMonUI.exe"s, args
					);
			}();

			// connect logging to the CEF process constellation
			ConnectToLoggingSourcePipe(cefLogPipe);

			// don't exit this process until the CEF control panel exits
			cefChild.wait();
		}
		else if (opt.subcCapture.Active()) {
			DWORD pid;
			if (opt.capTargetPid) {
				pmlog_info("Running headless capture").pmwatch(*opt.capTargetPid);
				try {
					util::win::OpenProcess(*opt.capTargetPid);
					pid = *opt.capTargetPid;
				}
				catch (...) {
					pmlog_error("Failed to find any process with specified pid")
						.pmwatch(*opt.capTargetPid).no_trace();
					return -1;
				}
			}
			else if (opt.capTargetName) {
				pmlog_info("Running headless capture").pmwatch(*opt.capTargetName);
				const auto procs = util::win::ProcessMapBuilder{}.AsNameMap(true);
				const auto target = util::str::ToWide(util::str::ToLower(*opt.capTargetName));
				try {
					pid = procs.at(target).pid;
				}
				catch (...) {
					pmlog_error("Failed to find any processes matching supplied main module name")
						.pmwatch(*opt.capTargetName).no_trace();
					return -1;
				}
			}
			auto fullPathOverride = opt.capOutput.AsOptional().transform([](const auto& path) {
				return util::str::ToWide(path);
			});
			auto pSpec = std::make_unique<kern::OverlaySpec>(kern::OverlaySpec{
				.pid = pid,
				.captureFullPathOverride = std::move(fullPathOverride),
				.etwFlushPeriod = 20.,
				.manualEtwFlush = true,
				.telemetrySamplingPeriodMs = *opt.capTelemetryPeriod,
				.hideAlways = true,
			});
			std::cout << "Starting capture..." << std::endl;
			kernel.PushSpec(std::move(pSpec));
			kernel.SetCapture(true);
			// setup thread to handle stdin commands (currently only stop capture -> quit)
			util::win::Event stopCommandEvent;
			std::thread{ [&] {
				std::ios::sync_with_stdio(false);
				for (std::string line; std::getline(std::cin, line);) {
					const auto trimmed = util::str::TrimWhitespace(line);
					if (trimmed == "%q") {
						stopCommandEvent.Set();
						break;
					}
				}
				pmlog_dbg("Exiting stdin command listener thread");
			} }.detach();
			// wait for desired capture duration, with option to wake early on kernel signal or %q command
			if (opt.capDuration) {
				const auto dur = *opt.capDuration * 1s + 0.3s;
				auto res = util::win::WaitAnyEventFor(dur,
					dynamic_cast<HeadlessKernelHandler*>(pKernelHandler.get())->stopEvent_,
					stopCommandEvent);
				if (!res) {
					std::cout << "Capture complete." << std::endl;
				}
				else if (*res == 1) {
					std::cerr << "Capture terminated by %q command." << std::endl;
				}
				else {
					std::cerr << "Capture terminated prematurely." << std::endl;
				}
			}
			else { // wait indefinitely until %q command received or kernel signal
				auto res = util::win::WaitAnyEvent(
					dynamic_cast<HeadlessKernelHandler*>(pKernelHandler.get())->stopEvent_,
					stopCommandEvent);
				if (res && *res == 1) {
					std::cerr << "Capture terminated by %q command." << std::endl;
				}
				else {
					std::cerr << "Capture terminated prematurely." << std::endl;
				}
			}
			kernel.SetCapture(false);
		}

		pmlog_info("== kernel process exiting ==");
	}
	catch (...) {
		pmlog_error(util::ReportException());
		return -1;
	}

	return 0;
}