#pragma once
#include "SharedMemoryTypes.h"
#include "HistoryRing.h"
#include <variant>

namespace pmon::ipc
{
    // container for multiple history rings organized by PM_METRIC x index
    class TelemetryMap
    {
        template<typename T>
        using HistoryRingVect = ShmVector<HistoryRing<T>>;
        using MapValueType = std::variant<HistoryRingVect<double>, HistoryRingVect<uint64_t>, HistoryRingVect<bool>>;
        using MapType = ShmMap<int, MapValueType>;
    public:
        TelemetryMap(const MapType::allocator_type& alloc)
            :
            ringMap_{ alloc }
        {
        }
        template<typename T>
        void AddRing(int id, size_t size, size_t count)
        {
            // extra guard of misuse at compile time
            static_assert(
                std::is_same_v<T, double> ||
                std::is_same_v<T, uint64_t> ||
                std::is_same_v<T, bool>,
                "Unsupported ring type for TelemetryMap"
            );

            using RingType = HistoryRing<T>;
            using RingAlloc = typename MapType::allocator_type::template rebind<RingType>::other;

            // Construct an allocator for ShmRing<T> from the map's allocator
            HistoryRingVect ringAlloc(ringMap_.get_allocator());

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
        HistoryRingVect<T>& FindRing(int id)
        {
            return std::get<HistoryRingVect<T>>(FindRingVariant(id));
        }
        template<typename T>
        const HistoryRingVect<T>& FindRing(int id) const
        {
            return std::get<HistoryRingVect<T>>(FindRingVariant(id));
        }
        MapValueType& FindRingVariant(int id)
        {
            return ringMap_.at(id);
        }
        const MapValueType& FindRingVariant(int id) const
        {
            return ringMap_.at(id);
        }
    private:
        MapType ringMap_;
    };
}