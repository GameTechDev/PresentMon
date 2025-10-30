#include "LogDemo.h"
#include <iostream>
#include <format>
#include <map>
#include <conio.h>
#include "CliOptions.h"
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"
#include "../CommonUtilities/win/WinAPI.h"
#include "../CommonUtilities/log/Log.h"
#include "../CommonUtilities/log/NamedPipeMarshallReceiver.h"
#include "../CommonUtilities/log/NamedPipeMarshallSender.h"
#include "../CommonUtilities/log/MarshallDriver.h"
#include "../CommonUtilities/log/EntryMarshallInjector.h"
#include "../CommonUtilities/log/LineTable.h"
#include "../CommonUtilities/Exception.h"
#include "LogSetup.h"

#include "../CommonUtilities/pipe/Pipe.h"

using namespace std::literals;
using namespace pmon::util;

PM_DEFINE_EX(LogDemoException);

void f2() {
	pmlog_error();
}
void f1() {
	f2();
}

void g2() {
	throw Except<LogDemoException>("something bad happened here");
}
void g1() {
	g2();
}

void RunLogDemo(int mode)
{
	pmapi::Session sesh;

	for (auto&& o : clio::Options::Get().GetForwardedOptions()) {
		std::cout << std::format("({}, {})", o.first, o.second) << std::endl;
	}
	
	// basic log info w/ message
	if (mode == 0) {
		pmlog_info("information goes here");
	}
	// basic warn w/ format message
	else if (mode == 1) {
		const int x = 4224;
		pmlog_warn(std::format("warning with variable x = {}", x));
	}
	// error log has trace (no message)
	else if (mode == 2) {
		f1();
	}
	// disable info at runtime
	else if (mode == 3) {
		log::GlobalPolicy::Get().SetLogLevel(log::Level::Warning);
		pmlog_info("info hidden");
		pmlog_warn("warn gets through");
	}
	// show build release disable (info/warn)
	else if (mode == 4) {
		pmlog_info("info hidden in release, shows in debug");
		pmlog_error("error always shows");
	}
	// set trace level
	else if (mode == 5) {
		log::GlobalPolicy::Get().SetTraceLevel(log::Level::Warning);
		pmlog_warn("enable stack trace for warning");
	}
	// error code logging hr
	else if (mode == 6) {
		CreateFileA("this-doesnt-exist.fake", 0, 0, nullptr, OPEN_EXISTING, 0, nullptr);
		pmlog_error().hr();
	}
	// error code logging pmon
	else if (mode == 7) {
		pmlog_error().code(PM_STATUS_INVALID_ADAPTER_ID);
	}
	// watch for variables
	else if (mode == 8) {
		const int x = 2, y = 10;
		pmlog_info().pmwatch(x).pmwatch(y).pmwatch(std::pow(x, y));
	}
	// loop frequency control 3 variants
	else if (mode == 9) {
		for (int i = 0; i < 9; i++) {
			pmlog_warn("every 3").every(3);
			pmlog_warn("first 2").first(2);
			pmlog_warn("after 6").after(6);
		}
	}
	// define, throw, catch custom exception
	else if (mode == 11) {
		try {
			g1();
		}
		catch (...) {
			std::cout << ReportException().first;
		}
	}
	// throw exception with trace (nested)
	else if (mode == 12) {
		log::GlobalPolicy::Get().SetExceptionTrace(true);
		try {
			g1();
		}
		catch (...) {
			std::cout << ReportException().first;
		}
	}
	// throw std exception
	else if (mode == 13) {
		try {
			auto x = std::map<int, int>{}.at(0);
		}
		catch (...) {
			std::cout << ReportException().first;
		}
	}
	// throw unknown exception
	else if (mode == 14) {
		try {
			throw std::vector<int>{};
		}
		catch (...) {
			std::cout << ReportException().first;
		}
	}
	// Seh exception catching with trace (nested)
	else if (mode == 15) {
		log::GlobalPolicy::Get().SetExceptionTrace(true);
		try {
			auto h = CreateEventA(nullptr, 0, 0, nullptr);
			CloseHandle(h);
			CloseHandle(h);
		}
		catch (...) {
			std::cout << ReportException().first;
		}
	}
	// named process / thread
	else if (mode == 16) {
		log::IdentificationTable::AddThisProcess("myprocy");
		log::IdentificationTable::AddThisThread("threadx");
		pmlog_info("rich point-of-origin context");
	}
	// ipc server setup (server always setup), set thread name special command
	else if (mode == 17) {
		auto pSender = std::make_shared<log::NamedPipeMarshallSender>("pml_demopipe");
		{
			log::IdentificationTable::RegisterSink(pSender);
			auto pDriver = std::make_shared<log::MarshallDriver>(pSender);
			log::GetDefaultChannel()->AttachComponent(std::move(pDriver));
		}
		std::string note;
		while (true) {
			int x = 3;
			std::cout << "SAY> ";
			std::getline(std::cin, note);
			if (note.empty()) break;
			if (note.front() == L'%') {
				log::IdentificationTable::AddThisThread(note);
			}
			else if (note.front() == L'$') {
				log::IdentificationTable::AddThisProcess(note);
			}
			else if (note.front() == L'#') {
				std::cout << "Waiting for connection..." << std::endl;
				pSender->WaitForConnection();
				std::cout << "We in there!" << std::endl;
			}
			else {
				pmlog_info(note);
			}
		}
	}
	// ipc client
	else if (mode == 18) {
		log::GetDefaultChannel()->AttachComponent(std::make_shared<log::EntryMarshallInjector>(log::GetDefaultChannel(),
			std::make_shared<log::NamedPipeMarshallReceiver>("pml_demopipe", log::IdentificationTable::GetPtr())));
		while (!_kbhit());
	}
	// blacklist to disable and force trace
	else if (mode == 19) {
		try {
			pmon::util::log::LineTable::IngestList("black.txt", true);
			pmon::util::log::LineTable::SetListMode(pmon::util::log::LineTable::ListMode::Black);
			pmon::util::log::LineTable::SetTraceOverride(true);
		}
		catch (...) {
			std::cout << "Couldn't process black.txt\n";
			return;
		}
		pmlog_info("b1");
		pmlog_info("b2");
		pmlog_info("b3");
	}
	else {
		std::cout << "unknown mode for log demo" << std::endl;
	}
}