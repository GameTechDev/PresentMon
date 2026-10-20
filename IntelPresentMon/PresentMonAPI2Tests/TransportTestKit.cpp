// Copyright (C) 2022-2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "TransportTestKit.h"

namespace TransportDemuxTests
{
	void RegisterActions_()
	{
		static std::once_flag once;
		std::call_once(once, [] {
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<OpenSession>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<EchoWithPush>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<BulkRequest>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<StallServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction<ProtocolFault>();
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<DropServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<SpoofServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<LateDoubleServer>());
			AsyncActionCollection<ServerExecutionContext>::Get().AddAction(std::make_unique<EchoTokenServer>());
			AsyncActionCollection<ClientExecutionContext>::Get().AddAction<PushNotice>();
			AsyncActionCollection<ClientExecutionContext>::Get().AddAction<BulkNotice>();
			AsyncActionCollection<ClientExecutionContext>::Get().AddAction(std::make_unique<StallReaderClient>());
		});
	}
}
