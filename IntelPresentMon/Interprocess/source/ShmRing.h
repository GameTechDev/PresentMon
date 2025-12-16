#pragma once
#include "SharedMemoryTypes.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/Log.h"
#include <chrono>
#include <thread>
#include <optional>

namespace pmon::ipc
{
	using namespace std::literals;

	// shared memory ring buffer for broadcast
	template<typename T, size_t ReadBufferSize = 4>
	class ShmRing
	{
	public:
		ShmRing(size_t capacity, ShmVector<T>::allocator_type alloc, bool backpressured = false)
			:
			backpressured_{ backpressured },
			data_{ capacity, alloc }
		{
			if (capacity < ReadBufferSize * 2) {
				throw std::logic_error{ "The capacity of a ShmRing must be at least double its ReadBufferSize" };
			}
		}


		ShmRing(const ShmRing&) = delete;
		ShmRing& operator=(const ShmRing&) = delete;
		// we need to enable move for use inside vectors
		// not enabled by default because of the atomic member
		ShmRing(ShmRing&& other)
			:
			backpressured_{ other.backpressured_ },
			data_{ std::move(other.data_) },
			nextWriteSerial_{ other.nextWriteSerial_.load() }
		{
		}
		ShmRing& operator=(ShmRing&& other)
		{
			if (this != &other) {
				data_ = std::move(other.data_);
				nextWriteSerial_ = other.nextWriteSerial_.load();
			}
			return *this;
		}
		~ShmRing() = default;


		bool Push(const T& val, std::optional<uint32_t> timeoutMs = {})
		{
			using clock = std::chrono::high_resolution_clock;
			if (backpressured_) {
				const auto start = timeoutMs ? clock::now() : decltype(clock::now()){};
				while (nextWriteSerial_ >= nextReadSerial_ + data_.size() - ReadBufferSize) {
					// bail with false to signal timeout without writing the pushed value
					if (timeoutMs && (clock::now() - start >= *timeoutMs * 1ms)) {
						return false;
					}
					std::this_thread::sleep_for(10ms);
				}
			}
			data_[IndexFromSerial_(nextWriteSerial_)] = val;
			nextWriteSerial_++;
			return true;
		}
		const T& At(size_t serial) const
		{
			// adds a one element buffer to help prevent reading partially updated data
			// note this is much less buffer than GetSerialRange because that range
			// returns the "safe" range considering processing time, while this check
			// determines whether actual stale data is being accessed at this instant
			if (serial + data_.size() <= nextWriteSerial_) {
				pmlog_warn("Reading stale serial").pmwatch(serial);
			}
			else if (nextWriteSerial_ <= serial) {
				pmlog_warn("Reading nonexistent serial").pmwatch(serial);
			}
			return data_[IndexFromSerial_(serial)];
		}
		std::pair<size_t, size_t> GetSerialRange() const
		{
			if (nextWriteSerial_ < data_.size()) {
				return { 0, nextWriteSerial_ };
			}
			else {
				// lock current next serial to keep the returned range logically consistent
				const size_t serial = nextWriteSerial_;
				// once we have looped around the vector, we need to maintain a buffer
				// to help avoid the client reading partially-updated data (data race)
				return { nextWriteSerial_ - data_.size() + ReadBufferSize, nextWriteSerial_ };
			}
		}
		void MarkNextRead(size_t serial) const
		{
			if (serial > nextReadSerial_) {
				nextReadSerial_ = serial;
			}
		}
		bool Empty() const
		{
			return nextWriteSerial_ == 0;
		}
	private:
		// functions
		size_t IndexFromSerial_(size_t serial) const
		{
			return serial % data_.size();
		}
		// data
		const bool backpressured_;
		std::atomic<size_t> nextWriteSerial_ = 0;
		mutable std::atomic<size_t> nextReadSerial_ = 0;
		ShmVector<T> data_;
	};
}
