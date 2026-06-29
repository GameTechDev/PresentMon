// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <memory>

namespace p2c::kern
{
	struct OverlaySpec;
}

namespace kproc::kact::push_spec_impl
{
	struct Params;
}

namespace pmapi::intro
{
    class Root;
}

namespace kproc
{
	std::unique_ptr<p2c::kern::OverlaySpec> MakeOverlaySpec(const kact::push_spec_impl::Params& spec,
		const pmapi::intro::Root& intro);
}