// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "ThreadCompass.h"
#include "../CommonUtilities/win/WinAPI.h"
#include <include/wrapper/cef_helpers.h>

namespace p2c::client::util
{
	std::string GetThreadName()
	{
		if (CefCurrentlyOn(TID_UI))
		{
			return "TID_UI";
		}
		else if (CefCurrentlyOn(TID_FILE_BACKGROUND))
		{
			return "TID_FILE_BACKGROUND";
		}
		else if (CefCurrentlyOn(TID_FILE_USER_VISIBLE))
		{
			return "TID_FILE_USER_VISIBLE";
		}
		else if (CefCurrentlyOn(TID_FILE_USER_BLOCKING))
		{
			return "TID_FILE_USER_BLOCKING";
		}
		else if (CefCurrentlyOn(TID_PROCESS_LAUNCHER))
		{
			return "TID_PROCESS_LAUNCHER";
		}
		else if (CefCurrentlyOn(TID_IO))
		{
			return "TID_IO";
		}
		else if (CefCurrentlyOn(TID_RENDERER))
		{
			return "TID_RENDERER";
		}
		else
		{
			return "";
		}
	}
}