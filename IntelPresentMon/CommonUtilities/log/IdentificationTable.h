#pragma once
#include <string>
#include <cstdint>
#include <unordered_map>
#include <shared_mutex>
#include <optional>
#include <memory>
#include <vector>

namespace pmon::util::log
{
	class IIdentificationSink
	{
	public:
		virtual void AddThread(uint32_t tid, uint32_t pid, std::string name) = 0;
		virtual void AddProcess(uint32_t pid, std::string name) = 0;
	};

	class IdentificationTable
	{
	public:
		// types
		struct Thread
		{
			uint32_t tid;
			uint32_t pid;
			std::string name;
		};
		struct Process
		{
			uint32_t pid;
			std::string name;
		};
		struct Bulk
		{
			std::vector<Thread> threads;
			std::vector<Process> processes;
		};
		// interface global functions
		static void AddThread(uint32_t tid, uint32_t pid, std::string name) noexcept;
		static void AddProcess(uint32_t pid, std::string name) noexcept;
		static void AddThisThread(std::string name) noexcept;
		static void AddThisProcess(std::string name) noexcept;
		static std::optional<Thread> LookupThread(uint32_t tid) noexcept;
		static std::optional<Process> LookupProcess(uint32_t pid) noexcept;
		static Bulk GetBulk() noexcept;
		static void RegisterSink(std::shared_ptr<IIdentificationSink> pSink) noexcept;
		static void UnregisterSink(const IIdentificationSink* pSink) noexcept;
		static IdentificationTable* GetPtr() noexcept;

		// implementation functions, made public for purposes of cross-module access
		static IdentificationTable& Get_();
		// l/r-value ref semantics instead of value semantics used to enable cross-module string copying
		void AddThread_(uint32_t tid, uint32_t pid, std::string&& name);
		void AddThread_(uint32_t tid, uint32_t pid, const std::string& name);
		void AddProcess_(uint32_t pid, std::string&& name);
		void AddProcess_(uint32_t pid, const std::string& name);
		std::optional<Thread> LookupThread_(uint32_t tid) const;
		std::optional<Process> LookupProcess_(uint32_t pid) const;
		Bulk GetBulk_() const;
		void RegisterSink_(std::shared_ptr<IIdentificationSink> pSink);
		void UnregisterSink_(const IIdentificationSink* pSink);
	private:
		// functions
		IdentificationTable();
		// data
		mutable std::shared_mutex mtx_;
		std::unordered_map<uint32_t, Process> processes_;
		std::unordered_map<uint32_t, Thread> threads_;
		std::vector<std::shared_ptr<IIdentificationSink>> sinks_;
	};
}
