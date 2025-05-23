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
#include <Core/source/cli/CliOptions.h>
#include <PresentMonAPI2Loader/Loader.h>
#include <Core/source/infra/LogSetup.h>
#include <CommonUtilities/win/Utilities.h>
#include <CommonUtilities/win/Privileges.h>
#include <Versioning/BuildId.h>
#include <Shobjidl.h>
#include <boost/process/v2.hpp>
#include <boost/process/v2/windows/as_user_launcher.hpp>
#include <array>
#include <ranges>


using namespace pmon;
namespace vi = std::views;
namespace rn = std::ranges;
using namespace std::literals;
namespace bp2 = boost::process::v2;
namespace as = boost::asio;

namespace kproc
{
	using KernelServer = ipc::act::SymmetricActionServer<kact::KernelExecutionContext>;

	class KernelHandler : public p2c::kern::KernelHandler
	{
	public:
		KernelHandler(KernelServer& server) : server_{ server } {}
		void OnTargetLost(uint32_t pid) override
		{
			server_.DispatchDetached(p2c::client::util::cact::TargetLostAction::Params{ pid });
		}
		void OnOverlayDied() override
		{
			server_.DispatchDetached(p2c::client::util::cact::OverlayDiedAction::Params{});
		}
		void OnPresentmonInitFailed() override
		{
			server_.DispatchDetached(p2c::client::util::cact::PresentmonInitFailedAction::Params{});
		}
		void OnStalePidSelected() override
		{
			server_.DispatchDetached(p2c::client::util::cact::StalePidAction::Params{});
		}
	private:
		// data
		KernelServer& server_;
	};
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
		// parse the command line arguments and make them globally available
		if (auto err = cli::Options::Init(__argc, __argv, true)) {
			if (*err == 0) {
				MessageBoxA(nullptr, cli::Options::GetDiagnostics().c_str(), "Command Line Help",
					MB_ICONINFORMATION | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			else {
				MessageBoxA(nullptr, cli::Options::GetDiagnostics().c_str(), "Command Line Parse Error",
					MB_ICONERROR | MB_APPLMODAL | MB_SETFOREGROUND);
			}
			return *err;
		}
		const auto& opt = cli::Options::Get();
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

		// set the app id so that windows get grouped
		// TODO: verify operation when multiple app instances running concurrently
		SetCurrentProcessExplicitAppUserModelID(L"Intel.PresentMon");

		pmlog_info(std::format("== kernel process starting build#{} clean:{} ==",
			bid::BuildIdShortHash(), !bid::BuildIdDirtyFlag()));

		// launch the service as a child process if desired (typically during development)
		const auto logSvcPipe = opt.logSvcPipe.AsOptional().value_or(
			std::format("pm2-child-svc-log-{}", GetCurrentProcessId()));
		as::io_context svcIoctx;
		std::optional<bp2::basic_process<as::io_context::executor_type>> svcChild;
		if (opt.svcAsChild) {
			svcChild = bp2::windows::default_launcher{}(
				svcIoctx, "PresentMonService.exe"s, std::vector{
				"--control-pipe"s, *opt.controlPipe,
				"--nsm-prefix"s, "pm-frame-nsm"s,
				"--intro-nsm"s, *opt.shmName,
				"--etw-session-name"s, *opt.etwSessionName,
				"--log-level"s, util::log::GetLevelName(util::log::GlobalPolicy::Get().GetLogLevel()),
				"--log-pipe-name"s, logSvcPipe,
				"--enable-stdio-log"s
			});

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

		// this pointer serves as a way to set the kernel on the server exec context after the server is created
		p2c::kern::Kernel* pKernel = nullptr;
		// active object that creates a window and sinks raw input messages to listen for hotkey presses
		p2c::win::Hotkeys hotkeys;
		// this server receives a connection from the CEF render process
		const auto actName = std::format(R"(\\.\pipe\ipm-cef-channel-{})", GetCurrentProcessId());
		KernelServer server{ kact::KernelExecutionContext{ .ppKernel = &pKernel, .pHotkeys = &hotkeys },
			actName, 1, "D:(A;;GA;;;WD)S:(ML;;NW;;;ME)" };
		// set the hotkey manager to send notifications via the action server
		hotkeys.SetHandler([&](int action) { 
			pmlog_info("hey hot");
			server.DispatchDetached(p2c::client::util::cact::HotkeyFiredAction::Params{ .actionId = action });
		});
		// this handler receives events from the kernel and transmits them to the render process via the server
		KernelHandler kernHandler{ server };
		// the kernel manages the PresentMon data collection and the overlay rendering
		p2c::kern::Kernel kernel{ &kernHandler };
		// new we set this pointer, giving the server access to the Kernel
		pKernel = &kernel;

		// compose optional cli args for cef process tree
		auto args = std::vector<std::string>{
			opt.filesWorking ? "--p2c-files-working"s : ""s,
			opt.traceExceptions ? "--p2c-trace-exceptions"s : ""s,
			opt.logFolder ? "--p2c-log-folder"s : "", *opt.logFolder,
		} | vi::filter(std::not_fn(&std::string::empty)) | rn::to<std::vector>();
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
		boost::asio::io_context cefIoctx;
		auto cefChild = [&] {
			if (util::win::WeAreElevated()) {
				try {
					pmlog_info("detected elevation, attempting integrity downgrade");
					auto mediumTokenPack = util::win::PrepareMediumIntegrityToken();
					return bp2::windows::as_user_launcher{ mediumTokenPack.hMediumToken.Get() }(
						cefIoctx, "PresentMonUI.exe"s, args
					);
				}
				catch (...) {
					pmlog_warn(util::ReportException("Failed to downgrade integrity, falling back to standard process spawn"));
				}
			}
			return bp2::windows::default_launcher{}(
				cefIoctx, "PresentMonUI.exe"s, args
			);
		}(); 

		// connect logging to the CEF process constellation
		ConnectToLoggingSourcePipe(cefLogPipe);

		// don't exit this process until the CEF control panel exits
		cefChild.wait();

		pmlog_info("== kernel process exiting ==");
	}
	catch (...) {
		pmlog_error(util::ReportException());
		return -1;
	}

	return 0;
}