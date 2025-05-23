// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "AsyncEndpointCollection.h"
#include "Logging.h"
#include <CommonUtilities/str/String.h>

#include "async/BrowseReadSpec.h"
#include "async/BrowseStoreSpec.h"
#include "async/EnumerateProcesses.h"
#include "async/EnumerateKeys.h"
#include "async/EnumerateModifiers.h"
#include "async/ExploreCaptures.h"
#include "async/LoadFile.h"
#include "async/StoreFile.h"
#include "async/GetTopGpuProcess.h"
#include "async/LoadEnvVars.h"
#include "async/CheckPathExistence.h"

namespace p2c::client::util
{
	template<class T>
	void AsyncEndpointCollection::AddEndpoint()
	{
		if (auto&& [i, inserted] = endpoints.insert({ T::GetKey(), std::make_unique<T>() }); !inserted)
		{
			pmlog_warn(std::format("Duplicate key for async {}", T::GetKey()));
		}
	}

	AsyncEndpointCollection::AsyncEndpointCollection()
	{
		using namespace async;
		AddEndpoint<BrowseReadSpec>();
		AddEndpoint<BrowseStoreSpec>();
		AddEndpoint<EnumerateProcesses>();
		AddEndpoint<EnumerateKeys>();
		AddEndpoint<EnumerateModifiers>();
		AddEndpoint<ExploreCaptures>();
		AddEndpoint<LoadFile>();
		AddEndpoint<StoreFile>();
		AddEndpoint<GetTopGpuProcess>();
		AddEndpoint<LoadEnvVars>();
		AddEndpoint<CheckPathExistence>();
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