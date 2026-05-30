#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_DEVICE_TYPE(X_) \
		X_(DEVICE_TYPE, INDEPENDENT, "Device Independent", "", "This device type is used for metrics independent of any specific hardware device (e.g. FPS metrics). Set device ID to 0.") \
		X_(DEVICE_TYPE, GRAPHICS_ADAPTER, "Graphics Adapter", "", "Graphics adapter or GPU device") \
		X_(DEVICE_TYPE, SYSTEM, "System", "", "Special virtual device represening core system components (such as the CPU). Set device ID to 0.")