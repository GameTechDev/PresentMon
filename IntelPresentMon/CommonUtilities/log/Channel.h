#pragma once
#include "IChannel.h"
#include <span>
#include <vector>
#include <memory>
#include <chrono>
#include "../mt/Thread.h"
#include <atomic>

namespace pmon::util::log
{
	class IPolicy;
	class IDriver;
	class IChannelObject;

	namespace
	{
		// internal channel implementation, has functions that are called only from the worker thread
		// in response to entries / packets placed in the queue
		class ChannelInternal_
		{
			friend struct QueueAccessor_;
		public:
			~ChannelInternal_();

			ChannelInternal_(const ChannelInternal_&) = delete;
			ChannelInternal_& operator=(const ChannelInternal_&) = delete;

			void Flush();
			void SignalExit();
			void DisableTraceResolution();
			void AttachComponentBlocking(std::shared_ptr<IChannelComponent>, std::string);
			std::shared_ptr<IChannelComponent> GetComponentBlocking(std::string) const;
			void RemoveComponentByTagBlocking(const std::string&);
			void EnqueueEntry(Entry&&);
			void EnqueueEntry(const Entry&);
			void EnqueueEntryWait(Entry&&);
			void EnqueueEntryWait(const Entry&);
			template<class P, typename...Args>
			void EnqueuePacketWait(Args&&...args);
			template<class P, typename...Args>
			bool EnqueuePacketWaitFor(std::chrono::milliseconds timeout, Args&&...args);
			template<class P, typename...Args>
			void EnqueuePacketAsync(Args&&...args);
		protected:
			ChannelInternal_(std::vector<std::pair<std::string, std::shared_ptr<IChannelComponent>>> components,
				bool synchronousMode);
			bool IsWorkerThread() const noexcept;
			bool IsSynchronousModeEnabled() const noexcept;
		private:
			// functions
			void AttachComponent_(std::shared_ptr<IChannelComponent>, std::string);
			// data
			// mutex used for infrequent operations like managing components
			mutable std::mutex mtx_;
			bool resolvingTraces_ = true;
			bool exiting_ = false;
			std::vector<std::pair<std::string, std::shared_ptr<IDriver>>> driverPtrs_;
			std::vector<std::pair<std::string, std::shared_ptr<IPolicy>>> policyPtrs_;
			std::vector<std::pair<std::string, std::shared_ptr<IChannelObject>>> objectPtrs_;
			std::shared_ptr<void> pEntryQueue_;
			const bool synchronousMode_;
			std::atomic<uint32_t> workerTid_ = 0;
			mt::Thread worker_;
		};
	}

	// Channel: standard free-threaded lockfree-queued implementation of IChannel
	// external channel implementation, has functions that are only called from outside of the channel
	// worker thread and which only place entries / packets in the queue
	class Channel : public IChannel, private ChannelInternal_
	{
	public:
		explicit Channel(bool synchronousMode);
		Channel(std::vector<std::pair<std::string, std::shared_ptr<IChannelComponent>>> componentPtrs = {},
			bool synchronousMode = false);
		Channel(const Channel&) = delete;
		Channel& operator=(const Channel&) = delete;
		~Channel();
		void Submit(Entry&&) noexcept override;
		void Submit(const Entry&) noexcept override;
		void Flush() override;
		bool TryFlushFor(std::chrono::milliseconds timeout) noexcept override;
		void AttachComponent(std::shared_ptr<IChannelComponent>, std::string = {}) override;
		std::shared_ptr<IChannelComponent> GetComponent(std::string tag) const override;
		void FlushEntryPointExit() override;
	};
}
