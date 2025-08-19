#include <assert.h>

#include "Context.h"
#include "Custom/IUnknown_Wrapper.h"

namespace GfxLayer
{
	struct ThreadData
	{
		MemoryPool m_MemoryPool;
	};

	std::unique_ptr<Context>					Context::s_pInstance  = nullptr;
	thread_local std::unique_ptr<ThreadData>	Context::s_ThreadData = nullptr;
	thread_local uint32_t						Context::s_CallScope  = 0;

	Context::Context()
	{}

	Context& Context::GetInstance()
	{
		if (s_pInstance == nullptr)
		{
			s_pInstance.reset(new Context);
		}

		return *s_pInstance;
	}

	void Context::AddWrapper(IUnknown_Wrapper* pUnknownWrapper)
	{
		assert(pUnknownWrapper);
		std::lock_guard<std::mutex> lock(m_WrappersMutex);
		m_Wrappers[pUnknownWrapper->GetRootIUnknown()] = pUnknownWrapper;
	}

	void Context::RemoveWrapper(IUnknown_Wrapper* pUnknownWrapper)
	{
		assert(pUnknownWrapper);
		std::lock_guard<std::mutex> lock(m_WrappersMutex);
		m_Wrappers.erase(pUnknownWrapper->GetRootIUnknown());
	}

	IUnknown_Wrapper* Context::FindWrapper(IUnknown* pUnknown)
	{
		std::lock_guard<std::mutex> lock(m_WrappersMutex);
		auto* pRootUnknown = IUnknown_Wrapper::GetRootIUnknown(pUnknown);
		if (pRootUnknown && m_Wrappers.contains(pRootUnknown))
		{
			return m_Wrappers.at(pRootUnknown);
		}
		return nullptr;
	}

	uint32_t Context::IncrementCallScope()
	{
		return ++s_CallScope;
	}

	uint32_t Context::DecrementCallScope()
	{
		return --s_CallScope;
	}

	MemoryPool& Context::GetMemoryPool()
	{
		auto* pThreadData = GetThreadData();
		assert(pThreadData && "ThreadData is null!");

		pThreadData->m_MemoryPool.Reset();
		return pThreadData->m_MemoryPool;
	}

	ThreadData* Context::GetThreadData()
	{
		if (!s_ThreadData)
		{
			s_ThreadData = std::make_unique<ThreadData>();
		}

		return s_ThreadData.get();
	}
}