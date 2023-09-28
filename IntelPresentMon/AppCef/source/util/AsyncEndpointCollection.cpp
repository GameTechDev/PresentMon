// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AsyncEndpointCollection.h"
#include <Core/source/infra/log/Logging.h>
#include <Core/source/infra/util/Util.h>

#include "async/BrowseReadSpec.h"
#include "async/BrowseStoreSpec.h"
#include "async/BindHotkey.h"
#include "async/ClearHotkey.h"
#include "async/EnumerateAdapters.h"
#include "async/SetAdapter.h"
#include "async/LaunchKernel.h"
#include "async/PushSpecification.h"
#include "async/EnumerateProcesses.h"
#include "async/EnumerateMetrics.h"
#include "async/EnumerateKeys.h"
#include "async/EnumerateModifiers.h"
#include "async/SetCapture.h"
#include "async/ExploreCaptures.h"
#include "async/LoadFile.h"
#include "async/StoreFile.h"
#include "async/GetTopGpuProcess.h"

namespace p2c::client::util
{
	template<class T>
	void AsyncEndpointCollection::AddEndpoint()
	{
		if (auto&& [i, inserted] = endpoints.insert({ T::GetKey(), std::make_unique<T>() }); !inserted)
		{
			p2clog.warn(std::format(L"Duplicate key for async {}", infra::util::ToWide(T::GetKey()))).commit();
		}
	}

	AsyncEndpointCollection::AsyncEndpointCollection()
	{
		using namespace async;
		AddEndpoint<BrowseReadSpec>();
		AddEndpoint<BrowseStoreSpec>();
		AddEndpoint<BindHotkey>();
		AddEndpoint<ClearHotkey>();
		AddEndpoint<EnumerateAdapters>();
		AddEndpoint<SetAdapter>();
		AddEndpoint<LaunchKernel>();
		AddEndpoint<PushSpecification>();
		AddEndpoint<EnumerateProcesses>();
		AddEndpoint<EnumerateMetrics>();
		AddEndpoint<EnumerateKeys>();
		AddEndpoint<EnumerateModifiers>();
		AddEndpoint<SetCapture>();
		AddEndpoint<ExploreCaptures>();
		AddEndpoint<LoadFile>();
		AddEndpoint<StoreFile>();
		AddEndpoint<GetTopGpuProcess>();
	}

	const AsyncEndpoint* AsyncEndpointCollection::Find(const std::string& key) const
	{
		if (auto i = endpoints.find(key); i != endpoints.end())
		{
			return i->second.get();
		}
		else
		{
			return nullptr;
		}
	}
}