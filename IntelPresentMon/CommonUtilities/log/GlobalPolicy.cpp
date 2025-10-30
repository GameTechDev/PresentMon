#include "GlobalPolicy.h"

#ifndef PMLOG_GPOL_DEFAULT_LEVEL
#ifndef NDEBUG
#define PMLOG_GPOL_DEFAULT_LEVEL Level::Info
#else
#define PMLOG_GPOL_DEFAULT_LEVEL Level::Error
#endif
#endif

namespace pmon::util::log
{
	GlobalPolicy::GlobalPolicy() noexcept
		:
		logLevel_{ PMLOG_GPOL_DEFAULT_LEVEL }
	{}
	GlobalPolicy& GlobalPolicy::Get() noexcept
	{
		// @SINGLETON
		static GlobalPolicy policy;
		return policy;
	}

	Level GlobalPolicy::GetLogLevel() const noexcept
	{
		return logLevel_;
	}
	void GlobalPolicy::SetLogLevel(Level level) noexcept
	{
		logLevel_ = level;
	}
	void GlobalPolicy::SetLogLevelDefault() noexcept
	{
		SetLogLevel(PMLOG_GPOL_DEFAULT_LEVEL);
	}
	Level GlobalPolicy::GetTraceLevel() const noexcept
	{
		return traceLevel_;
	}
	void GlobalPolicy::SetTraceLevel(Level level) noexcept
	{
		traceLevel_ = level;
	}
	void GlobalPolicy::SetTraceLevelDefault() noexcept
	{
		SetTraceLevel(Level::Error);
	}
	bool GlobalPolicy::GetResolveTraceInClientThread() const noexcept
	{
		return resolveTraceInClientThread_;
	}
	void GlobalPolicy::SetResolveTraceInClientThread(bool setting) noexcept
	{
		resolveTraceInClientThread_ = setting;
	}
	bool GlobalPolicy::GetExceptionTrace() const noexcept
	{
		return exceptionTracePolicy_;
	}
	void GlobalPolicy::SetExceptionTrace(bool policy) noexcept
	{
		exceptionTracePolicy_ = policy;
	}
	bool GlobalPolicy::GetSehTracing() const noexcept
	{
		return sehTraceOn_;
	}
	void GlobalPolicy::SetSehTracing(bool on) noexcept
	{
		sehTraceOn_ = on;
	}
	Subsystem GlobalPolicy::GetSubsystem() const noexcept
	{
		return subsystem_;
	}
	void GlobalPolicy::SetSubsystem(Subsystem sys) noexcept
	{
		subsystem_ = sys;
	}
	void GlobalPolicy::ActivateVerboseModule(V mod) noexcept
	{
		activeVerboseModules_ |= (1ull << uint64_t(mod));
	}
	void GlobalPolicy::DeactivateVerboseModule(V mod) noexcept
	{
		activeVerboseModules_ &= ~(1ull << uint64_t(mod));
	}
	bool GlobalPolicy::CheckVerboseModule(V mod) const noexcept
	{
		return activeVerboseModules_ & (1ull << uint64_t(mod));
	}
	void GlobalPolicy::ClearVerboseModules() noexcept
	{
		activeVerboseModules_ = 0;
	}
	void GlobalPolicy::StoreVerboseModules(uint64_t modset) noexcept
	{
		activeVerboseModules_.store(modset);
	}
}