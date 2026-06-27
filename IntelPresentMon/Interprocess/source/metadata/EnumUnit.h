#pragma once
#include "../../../PresentMonAPI2/PresentMonAPI.h"

// enum annotation (enum_name_fragment, key_name_fragment, name, short_name, description)
#define ENUM_KEY_LIST_UNIT(X_) \
		X_(UNIT, DIMENSIONLESS, "Dimensionless", "", "Dimensionless numeric metric") \
		X_(UNIT, RATIO, "Ratio", "", "Ratio of one value to another (such as used / total memory)") \
		X_(UNIT, BOOLEAN, "Boolean", "", "Boolean value with 1 indicating present/active and 0 indicating vacant/inactive") \
		X_(UNIT, PERCENT, "Percent", "%", "Proportion or ratio represented as a fraction of 100") \
		X_(UNIT, FPS, "Frames Per Second", "fps", "Rate of application frames being presented per unit time") \
		X_(UNIT, MICROSECONDS, "Microseconds", "us", "Time duration in microseconds") \
		X_(UNIT, MILLISECONDS, "Milliseconds", "ms", "Time duration in milliseconds") \
		X_(UNIT, SECONDS, "Seconds", "s", "Time duration in seconds") \
		X_(UNIT, MINUTES, "Minutes", "m", "Time duration in minutes") \
		X_(UNIT, HOURS, "Hours", "h", "Time duration in hours") \
		X_(UNIT, MILLIWATTS, "Milliwatts", "mW", "Power in milliwatts (millijoules per second)") \
		X_(UNIT, WATTS, "Watts", "W", "Power in watts (Joules per second)") \
		X_(UNIT, KILOWATTS, "Kilowatts", "kW", "Power in kilowatts (kilojoules per second)") \
		X_(UNIT, VERTICAL_BLANKS, "Vertical Blanks", "vblk", "A count of vertical blanks (hardware display updates)") \
		X_(UNIT, MILLIVOLTS, "Millivolts", "mV", "Electric potential 0.001 V") \
		X_(UNIT, VOLTS, "Volts", "V", "Electric potential") \
		X_(UNIT, HERTZ, "Hertz", "Hz", "Frequency in cycles per second") \
		X_(UNIT, KILOHERTZ, "Kilohertz", "kHz", "Frequency in thousands of cycles per second") \
		X_(UNIT, MEGAHERTZ, "Megahertz", "MHz", "Frequency in millions of cycles per second") \
		X_(UNIT, GIGAHERTZ, "Gigahertz", "GHz", "Frequency in billions of cycles per second") \
		X_(UNIT, CELSIUS, "Degrees Celsius", (const char*)u8"�C", "Temperature in degrees Celsius") \
		X_(UNIT, RPM, "Revolutions per Minute", "RPM", "Angular speed in revolutions per minute") \
		X_(UNIT, BITS_PER_SECOND, "Bits per Second", "bps", "Bandwidth / data throughput in bits per second") \
		X_(UNIT, KILOBITS_PER_SECOND, "Kilobits per Second", "kbps", "Bandwidth / data throughput in kilobits per second") \
		X_(UNIT, MEGABITS_PER_SECOND, "Megabits per Second", "Mbps", "Bandwidth / data throughput in megabits per second") \
		X_(UNIT, GIGABITS_PER_SECOND, "Gigabits per Second", "Gbps", "Bandwidth / data throughput in gigabits per second") \
		X_(UNIT, BYTES, "Bytes", "B", "Data volume in bytes") \
		X_(UNIT, KILOBYTES, "Kilobytes", "kB", "Data volume in kilobytes") \
		X_(UNIT, MEGABYTES, "Megabytes", "MB", "Data volume in megabytes") \
		X_(UNIT, GIGABYTES, "Gigabytes", "GB", "Data volume in gigabytes") \
		X_(UNIT, QPC, "High-performance timestamp", "qpc", "Timestamp obtained via QueryPerformanceCounter (or compatible)")
