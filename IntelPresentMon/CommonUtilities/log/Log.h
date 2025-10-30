#pragma once
#include "EntryBuilder.h"
#include "IChannel.h"
#include "GlobalPolicy.h"
#include "Subsystem.h"
#include "../Macro.h"
#include <functional>

namespace pmon::util::log
{
	// this function must be implemented by the module using this log utility
	std::shared_ptr<IChannel> GetDefaultChannel() noexcept;
	// eager initialize logging on separate thread
	void BootDefaultChannelEager() noexcept;
	// set the default channel directly
	void InjectDefaultChannel(std::shared_ptr<IChannel>) noexcept;
	// utility that client module can use to help implement GetDefaultChannel
	std::shared_ptr<IChannel> GetDefaultChannelWithFactory(std::function<std::shared_ptr<IChannel>()> factory) noexcept;
	// flush channel worker queue and exit thread in preparation for entry point return
	void FlushEntryPoint() noexcept;
		
	struct Voidifier_ { template<class T> void operator&(T&&) const {} };
}

#ifdef PMLOG_BUILD_LEVEL
#define PMLOG_BUILD_LEVEL_ ::pmon::util::log::Level::PMLOG_BUILD_LEVEL
#endif

#ifndef PMLOG_BUILD_LEVEL_
#define PMLOG_BUILD_LEVEL_ ::pmon::util::log::Level::Verbose
#endif

#define pmlog_from_(lvl, file, function, line) ((PMLOG_BUILD_LEVEL_ < lvl) || (::pmon::util::log::GlobalPolicy::Get().GetLogLevel() < lvl)) \
	? (void)0 : ::pmon::util::log::Voidifier_{} & ::pmon::util::log::EntryBuilder{ lvl, file, function, line } \
	.subsys(::pmon::util::log::GlobalPolicy::Get().GetSubsystem()) \
	.to(::pmon::util::log::GetDefaultChannel())
#define pmlog_(lvl) pmlog_from_(lvl, __FILE__, __FUNCTION__, __LINE__)
#define pmlog_fatal	pmlog_(::pmon::util::log::Level::Fatal).note
#define pmlog_error	pmlog_(::pmon::util::log::Level::Error).note
#define pmlog_warn	pmlog_(::pmon::util::log::Level::Warning).note
#define pmlog_info	pmlog_(::pmon::util::log::Level::Info).note
#define pmlog_dbg	pmlog_(::pmon::util::log::Level::Debug).note
#define pmlog_verb(vtag) !::pmon::util::log::GlobalPolicy::VCheck(vtag) ? (void)0 : pmlog_(::pmon::util::log::Level::Verbose).note
#define pmlog_verb2(v2tag) !v2tag ? (void)0 : pmlog_(::pmon::util::log::Level::Verbose2).note
#define pmlog_perf(ptag) !ptag ? (void)0 : pmlog_(::pmon::util::log::Level::Performance).note

#define pmwatch(expr) watch(#expr, (expr))

#define pmlog_mark ::pmon::util::log::TimePoint
