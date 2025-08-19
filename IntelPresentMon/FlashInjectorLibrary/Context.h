#pragma once
#include "../CommonUtilities/win/WinAPI.h"
#include <comdef.h>


#include <memory>
#include <mutex>
#include <unordered_map>

#include "NonCopyable.h"
#include "MemoryPool.h"

namespace GfxLayer
{
	struct ThreadData;
	struct  IUnknown_Wrapper;

	using WrapperMap = std::unordered_map<IUnknown*, IUnknown_Wrapper*>;

	class Context: public NonCopyable
	{
	public:
		~Context() = default;
		static Context&									GetInstance();

		void											AddWrapper(IUnknown_Wrapper* pUnknownWrapper);
		void											RemoveWrapper(IUnknown_Wrapper* pUnknownWrapper);
		IUnknown_Wrapper*								FindWrapper(IUnknown* pUnknown);

		MemoryPool&										GetMemoryPool();
		uint32_t										IncrementCallScope();
		uint32_t										DecrementCallScope();

	private:
		Context();

		ThreadData*										GetThreadData();

		WrapperMap										m_Wrappers;
		std::mutex										m_WrappersMutex;

		static thread_local	std::unique_ptr<ThreadData> s_ThreadData;
		static thread_local uint32_t					s_CallScope;
		static std::unique_ptr<Context>					s_pInstance;
	};
}