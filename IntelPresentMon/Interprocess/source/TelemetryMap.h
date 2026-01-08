#pragma once
#include "SharedMemoryTypes.h"
#include "HistoryRing.h"
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include <variant>

namespace pmon::ipc
{
    // container for multiple history rings organized by PM_METRIC x index
    class TelemetryMap
    {
    public:
        template<typename T>
        using HistoryRingVect = ShmVector<HistoryRing<T>>;
        using MapValueType = std::variant<
            HistoryRingVect<double>, HistoryRingVect<uint64_t>,
            HistoryRingVect<bool>, HistoryRingVect<int>>;
        using MapType = ShmMap<PM_METRIC, MapValueType>;
        using AllocatorType = MapType::allocator_type;
        TelemetryMap(AllocatorType alloc)
            :
            ringMap_{ alloc }
        {}
        void AddRing(PM_METRIC id, size_t size, size_t count, PM_DATA_TYPE type)
        {
            switch (type) {
            case PM_DATA_TYPE_DOUBLE:
                AddRing<double>(id, size, count);
                break;
            case PM_DATA_TYPE_UINT64:
                AddRing<uint64_t>(id, size, count);
                break;
            case PM_DATA_TYPE_BOOL:
                AddRing<bool>(id, size, count);
                break;
            case PM_DATA_TYPE_ENUM:
                AddRing<int>(id, size, count);
                break;
            default: throw util::Except<>("Unsupported ring type for TelemetryMap");
            }
        }
        template<typename T>
        void AddRing(PM_METRIC id, size_t size, size_t count)
        {
            // extra guard of misuse at compile time
            static_assert(
                std::is_same_v<T, double> ||
                std::is_same_v<T, uint64_t> ||
                std::is_same_v<T, bool> ||
                std::is_same_v<T, int>,
                "Unsupported ring type for TelemetryMap"
            );

            using RingAlloc = typename MapType::allocator_type::template rebind<HistoryRing<T>>::other;

            // Construct an allocator for HistoryRing<T> from the map's allocator
            RingAlloc ringAlloc(ringMap_.get_allocator());

            // Insert (or get existing) entry for this id, constructing the correct variant alternative
            auto [it, inserted] = ringMap_.emplace(
                std::piecewise_construct,
                std::forward_as_tuple(id),
                std::forward_as_tuple(
                    std::in_place_type<HistoryRingVect<T>>,
                    ringAlloc
                )
            );
            // don't allow AddRing when one already exists for the given id
            if (!inserted) {
                throw std::logic_error("TelemetryMap::AddRing: id already exists");
            }

            // Get the vector for this T
            auto& rings = std::get<HistoryRingVect<T>>(it->second);
            // construct (count) rings in the vector
            rings.reserve(count);
            for (size_t i = 0; i < count; ++i) {
                rings.emplace_back(size, ringAlloc);
            }
        }
        template<typename T>
        HistoryRingVect<T>& FindRing(PM_METRIC id)
        {
            return std::get<HistoryRingVect<T>>(FindRingVariant(id));
        }
        template<typename T>
        const HistoryRingVect<T>& FindRing(PM_METRIC id) const
        {
            return std::get<HistoryRingVect<T>>(FindRingVariant(id));
        }
        MapValueType& FindRingVariant(PM_METRIC id)
        {
            return ringMap_.at(id);
        }
        const MapValueType& FindRingVariant(PM_METRIC id) const
        {
            return ringMap_.at(id);
        }
        size_t ArraySize(PM_METRIC id) const
        {
            auto it = ringMap_.find(id);
            if (it == ringMap_.end()) {
                return 0;
            }
            return std::visit([](auto const& rings) -> size_t {
                return rings.size();
            }, it->second);
        }
        auto Rings()
        {
            return std::ranges::subrange{ ringMap_.begin(), ringMap_.end() };
        }
        auto Rings() const
        {
            return std::ranges::subrange{ ringMap_.begin(), ringMap_.end() };
        }
    private:
        MapType ringMap_;
    };
}