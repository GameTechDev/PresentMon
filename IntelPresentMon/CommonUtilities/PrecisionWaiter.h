// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "win/Handle.h"
#include "Qpc.h"


namespace pmon::util
{
	class PrecisionWaiter
	{
	public:
		static constexpr double standardWaitBuffer = 0.001;

		PrecisionWaiter(double defaultWaitBuffer = standardWaitBuffer) noexcept;
		PrecisionWaiter(const PrecisionWaiter&) = delete;
		PrecisionWaiter& operator=(const PrecisionWaiter&) = delete;
		PrecisionWaiter(PrecisionWaiter&&) = delete;
		PrecisionWaiter& operator=(PrecisionWaiter&&) = delete;
		~PrecisionWaiter() = default;
		double Wait(double seconds, bool alertable = false) noexcept;
		double WaitUnbuffered(double seconds, bool alertable = false) noexcept;
		double WaitWithBuffer(double seconds, double buffer, bool alertable = false) noexcept;
		// todo: add function to set wait and receive handle, make compatible with event wait functions
	private:
		// amount of time we should wake up early by to do hyper-accurate spin wait
		double defaultWaitBuffer_;
		QpcTimer qpcTimer_;
		win::Handle waitableTimer_;
	};
}