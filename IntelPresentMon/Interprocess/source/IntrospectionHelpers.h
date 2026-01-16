#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"

namespace pmon::ipc::intro
{
	size_t GetDataTypeSize(PM_DATA_TYPE v);
	size_t GetDataTypeAlignment(PM_DATA_TYPE v);
}
