#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_METRIC_AVAILABILITY(X_) \
		X_(METRIC_AVAILABILITY, AVAILABLE, "Available", "", "Metric is available on the indicated device") \
		X_(METRIC_AVAILABILITY, UNAVAILABLE, "Unavailable", "", "Metric is unavailable on the indicated device") \
		X_(METRIC_AVAILABILITY, NOT_EXPORTED_BY_SOURCE, "Not supported by API", "", "Not supported by the API") \
		X_(METRIC_AVAILABILITY, NOT_SUPPORTED_BY_DEVICE, "Not supported by device", "", "Not supported by this device") \
		X_(METRIC_AVAILABILITY, NOT_IMPLEMENTED_BY_PRESENTMON, "Not implemented by PresentMon", "", "Not implemented by PresentMon currently")