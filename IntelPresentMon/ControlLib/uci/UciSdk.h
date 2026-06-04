// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#if __has_include(<uci/uci.h>) && __has_include(<uci/uci-data-records.h>)
#define PMON_HAS_UCI_SDK 1
#include <uci/uci-data-records.h>
#include <uci/uci.h>
#else
#define PMON_HAS_UCI_SDK 0
#endif
