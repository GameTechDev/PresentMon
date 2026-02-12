#include "PowerTelemetryContainer.h"
#include "../ControlLib/Exceptions.h"
#include <algorithm>
#include <ranges>
#include <functional>

#include "../CommonUtilities/log/GlogShim.h"

bool PowerTelemetryContainer::Repopulate() {
	try {
		telemetry_providers_.clear();
		telemetry_adapters_.clear();

		// create providers
		for (int iVendor = 0; iVendor < int(PM_DEVICE_VENDOR_UNKNOWN); iVendor++) {
			try {
				if (auto pProvider = pwr::PowerTelemetryProviderFactory::Make(
					PM_DEVICE_VENDOR(iVendor), device_id_allocator_)) {
					telemetry_providers_.push_back(std::move(pProvider));
				}
			}
			catch (const TelemetrySubsystemAbsent& e) {
				LOG(INFO) << "Skipping Telemetry Provider: " << e.what();
			}
			catch (const std::runtime_error& e) {
				LOG(INFO) << "Power Telemetry Failure: " << e.what();
			}
			catch (...) {
				LOG(INFO) << "Unknown Telemetry Failure.";
			}
		}
		// collect all adapters together from providers
		for (const auto& pProvider : telemetry_providers_) {
			auto& adapters = pProvider->GetAdapters();
			telemetry_adapters_.insert(telemetry_adapters_.end(), adapters.begin(),
				adapters.end());
		}
		// bail if there are not adapters
		if (telemetry_adapters_.size() == 0) {
			return false;
		}
		// Re-sort adapters based on video memory size in an attempt to return back the most
		// capable adapters
		constexpr auto ComparisonField = &pwr::PowerTelemetryAdapter::GetDedicatedVideoMemory;
		std::ranges::sort(telemetry_adapters_, std::greater{}, ComparisonField);
		return true;
	}
	catch (...) {
		return false;
	}
}
