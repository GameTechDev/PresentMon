#pragma once
#include "SharedMemoryTypes.h"

namespace pmon::ipc
{
	// manages shared memory segment and hosts data store T
	template<class T>
	class StreamedDataSegment
	{
	public:
		StreamedDataSegment(const std::string& segmentName, size_t size)
			:
			shm_{ bip::create_only }
		{}
		T& GetStore()
		{
			return *pData_;
		}
		const T& GetStore() const
		{
			return *pData_;
		}
	private:
		ShmSegment shm_;
		ShmUniquePtr<T> pData_;
	};
}