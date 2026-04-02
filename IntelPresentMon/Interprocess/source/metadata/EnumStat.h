#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_STAT(X_) \
		X_(STAT, NONE, "None", "", "Null stat, typically used when querying static or consuming frame events") \
		X_(STAT, AVG, "Average", "avg", "Average or mean of observations over the sliding window") \
		X_(STAT, PERCENTILE_99, "99th Percentile", "99%", "Value below which 99% of the observations within the sliding window fall") \
		X_(STAT, PERCENTILE_95, "95th Percentile", "95%", "Value below which 95% of the observations within the sliding window fall") \
		X_(STAT, PERCENTILE_90, "90th Percentile", "90%", "Value below which 90% of the observations within the sliding window fall") \
		X_(STAT, PERCENTILE_01, "1st Percentile", "1%", "Value below which 1% of the observations within the sliding window fall") \
		X_(STAT, PERCENTILE_05, "5th Percentile", "5%", "Value below which 5% of the observations within the sliding window fall") \
		X_(STAT, PERCENTILE_10, "10th Percentile", "10%", "Value below which 10% of the observations within the sliding window fall") \
		X_(STAT, MAX, "Maximum", "max", "Maximum value of observations within the sliding window") \
		X_(STAT, MIN, "Minimum", "min", "Minimum value of observations within the sliding window") \
		X_(STAT, MID_POINT, "Midpoint", "mpt", "Point sample of the observation nearest to the middle of the sliding window") \
		X_(STAT, MID_LERP, "Mid Lerp", "mlp", "Linear interpolation between the two observations nearest to the middle of the sliding window") \
		X_(STAT, NEWEST_POINT, "Newest Point", "raw", "Point sample of the most recent observation in the sliding window") \
		X_(STAT, OLDEST_POINT, "Oldest Point", "opt", "Point sample of the least recent observation in the sliding window") \
		X_(STAT, COUNT, "Count", "cnt", "Count of observations in the sliding window matching a predicate (e.g. counting # of observations for which a field is boolean true)") \
		X_(STAT, NON_ZERO_AVG, "Non-zero Average", (const char*)u8"øavg", "Average or mean of frame samples over the sliding window, excluding all zero values")