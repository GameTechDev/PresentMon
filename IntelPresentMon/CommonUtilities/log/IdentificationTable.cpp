#include "IdentificationTable.h"
#include "../win/WinAPI.h"
#include <ranges>
#include <algorithm>
#include "../Exception.h"

namespace pmon::util::log
{
	void IdentificationTable::AddThread(uint32_t tid, uint32_t pid, std::string name) noexcept
	{
		pmquell(Get_().AddThread_(tid, pid, std::move(name)))
	}
	void IdentificationTable::AddProcess(uint32_t pid, std::string name) noexcept
	{
		pmquell(Get_().AddProcess_(pid, std::move(name)))
	}
	void IdentificationTable::AddThisThread(std::string name) noexcept
	{
		AddThread(GetCurrentThreadId(), GetCurrentProcessId(), std::move(name));
	}
	void IdentificationTable::AddThisProcess(std::string name) noexcept
	{
		AddProcess(GetCurrentProcessId(), std::move(name));
	}
	std::optional<IdentificationTable::Thread> IdentificationTable::LookupThread(uint32_t tid) noexcept
	{
		try {
			return Get_().LookupThread_(tid);
		}
		catch (...) {
			return {};
		}
	}
	std::optional<IdentificationTable::Process> IdentificationTable::LookupProcess(uint32_t pid) noexcept
	{
		try {
			return Get_().LookupProcess_(pid);
		}
		catch (...) {
			return {};
		}
	}

	IdentificationTable::Bulk IdentificationTable::GetBulk() noexcept
	{
		try {
			return Get_().GetBulk_();
		}
		catch (...) {
			return {};
		}
	}

	void IdentificationTable::RegisterSink(std::shared_ptr<IIdentificationSink> pSink) noexcept
	{
		try {
			Get_().RegisterSink_(std::move(pSink));
		}
		catch (...) {}
	}

	void IdentificationTable::UnregisterSink(const IIdentificationSink* pSink) noexcept
	{
		try {
			Get_().UnregisterSink_(pSink);
		}
		catch (...) {}
	}

	IdentificationTable* IdentificationTable::GetPtr() noexcept
	{
		try {
			return &Get_();
		}
		catch (...) {
			return nullptr;
		}
	}

	IdentificationTable::IdentificationTable() = default;
	IdentificationTable& IdentificationTable::Get_()
	{
		// @SINGLETON
		static IdentificationTable table;
		return table;
	}
	void IdentificationTable::AddThread_(uint32_t tid, uint32_t pid, std::string&& name)
	{
		if (pid == GetCurrentProcessId()) {
			SetThreadDescription(GetCurrentThread(), str::ToWide(name).c_str());
		}
		std::lock_guard lk{ mtx_ };
		for (auto& p : sinks_) {
			pmquell(p->AddThread(tid, pid, name))
		}
		threads_.insert_or_assign(tid,
			Thread{ tid, pid, std::move(name) }
		);
	}
	void IdentificationTable::AddThread_(uint32_t tid, uint32_t pid, const std::string& name)
	{
		AddThread_(tid, pid, std::string{ name });
	}
	void IdentificationTable::AddProcess_(uint32_t pid, std::string&& name)
	{
		std::lock_guard lk{ mtx_ };
		for (auto& p : sinks_) {
			pmquell(p->AddProcess(pid, name))
		}
		processes_.insert_or_assign(pid,
			Process{ pid, std::move(name) }
		);
	}
	void IdentificationTable::AddProcess_(uint32_t pid, const std::string& name)
	{
		AddProcess_(pid, std::string{ name });
	}
	std::optional<IdentificationTable::Thread> IdentificationTable::LookupThread_(uint32_t tid) const
	{
		std::shared_lock lk{ mtx_ };
		if (auto i = threads_.find(tid); i != threads_.end()) {
			return i->second;
		}
		return {};
	}
	std::optional<IdentificationTable::Process> IdentificationTable::LookupProcess_(uint32_t pid) const
	{
		std::shared_lock lk{ mtx_ };
		if (auto i = processes_.find(pid); i != processes_.end()) {
			return i->second;
		}
		return {};
	}
	IdentificationTable::Bulk IdentificationTable::GetBulk_() const
	{
		std::shared_lock lk{ mtx_ };
		return Bulk{
			.threads = threads_ | std::views::values | std::ranges::to<std::vector>(),
			.processes = processes_ | std::views::values | std::ranges::to<std::vector>(),
		};
	}
	void IdentificationTable::RegisterSink_(std::shared_ptr<IIdentificationSink> pSink)
	{
		std::lock_guard lk{ mtx_ };
		sinks_.push_back(std::move(pSink));
	}

	void IdentificationTable::UnregisterSink_(const IIdentificationSink* pSink)
	{
		std::lock_guard lk{ mtx_ };
		std::erase_if(sinks_, [pSink](const auto& pRegistered) {
			return pRegistered.get() == pSink;
		});
	}
}
