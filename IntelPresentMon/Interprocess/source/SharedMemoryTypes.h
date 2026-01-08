#pragma once
#include <boost/interprocess/managed_windows_shared_memory.hpp>
#include <boost/interprocess/containers/string.hpp>
#include <boost/interprocess/containers/vector.hpp>
#include <boost/interprocess/containers/map.hpp>
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>

namespace pmon::ipc
{
	namespace bip = boost::interprocess;

	using ShmSegment = bip::managed_windows_shared_memory;
	using ShmSegmentManager = ShmSegment::segment_manager;
	template<typename T>
	using ShmAllocator = ShmSegmentManager::allocator<T>::type;
	using ShmString = bip::basic_string<char, std::char_traits<char>, ShmAllocator<char>>;
	template<typename T>
	using ShmVector = bip::vector<T, ShmAllocator<T>>;
	template<typename T>
	using ShmUniquePtr = typename bip::managed_unique_ptr<T, ShmSegment>::type;
	template<typename K, typename T>
	using ShmMap = bip::map<K, T, std::less<K>, ShmAllocator<std::pair<const K, T>>>;

	namespace impl {
		template<typename T, typename...P>
		ShmUniquePtr<T> ShmMakeUnique_(bip::ipcdetail::char_ptr_holder<char> name, ShmSegmentManager* pSegmentManager, P&&...params)
		{
			// construct the instance in shared memory
			auto ptr = pSegmentManager->construct<T>(name)(std::forward<P>(params)...);
			// construct uptr compatible with storage in a shared segment
			return ShmUniquePtr<T>(ptr, bip::deleter<T, ShmSegmentManager>{ pSegmentManager });
		}
	}
	template<typename T, typename...P>
	ShmUniquePtr<T> ShmMakeUnique(ShmSegmentManager* pSegmentManager, P&&...params)
	{
		return impl::ShmMakeUnique_<T>(bip::anonymous_instance, pSegmentManager, std::forward<P>(params)...);
	}
	template<typename T, typename...P>
	ShmUniquePtr<T> ShmMakeNamedUnique(const std::string& name, ShmSegmentManager* pSegmentManager, P&&...params)
	{
		return impl::ShmMakeUnique_<T>(name.c_str(), pSegmentManager, std::forward<P>(params)...);
	}
}