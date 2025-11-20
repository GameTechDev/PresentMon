#pragma once
#include "SharedMemoryTypes.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/Log.h"

namespace pmon::ipc
{
	template<typename T, size_t ReadBufferSize = 4>
	class ShmRing
	{
	public:
		ShmRing(size_t capacity, ShmSegmentManager* pSegmentManager)
			:
			data_{ capacity, pSegmentManager->get_allocator<T>() }
		{
			if (capacity < ReadBufferSize * 2) {
				throw std::logic_error{ "The capacity of a ShmRing must be at least double its ReadBufferSize" };
			}
		}
		void Push(const T& val)
		{
			data_[IndexFromSerial_(nextWriteSerial_)] = val;
			nextWriteSerial_++;
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
	private:
		// functions
		size_t IndexFromSerial_(size_t serial) const
		{
			return serial % data_.size();
		}
		// data
		std::atomic<size_t> nextWriteSerial_ = 0;
		ShmVector<T> data_;
	};
}