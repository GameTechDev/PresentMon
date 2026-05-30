#pragma once
#include "Handle.h"
#include <span>
#include <cstdint>
#include <optional>
#include <concepts>
#include <chrono>
#include <string>

struct _SECURITY_ATTRIBUTES;
typedef struct _SECURITY_ATTRIBUTES SECURITY_ATTRIBUTES;

namespace pmon::util::win
{
    class Event : public Handle
    {
    public:
        // types
        struct ConstructEmptyTag {};
        // functions
        Event(ConstructEmptyTag);
        Event(Event&& other) noexcept = default;
        Event& operator=(Event&& other) noexcept = default;
        Event(bool manualReset = true, bool initialState = false);
        static Event CreateNamed(const std::string& name, bool manualReset = true,
            bool initialState = false, SECURITY_ATTRIBUTES* pSecurityAttributes = nullptr);
        static Event OpenNamed(const std::string& name);
        static Event OpenNamed(const std::string& name, unsigned long desiredAccess);
        // set the event flag value to signalled
        void Set();
        // quickly sets and resets the event flag
        void Pulse();
        // resets the event flag value to unsignalled
        void Reset();
    private:
        explicit Event(HandleType handle);
    };

    std::optional<uint32_t> WaitOnMultipleEvents(std::span<Event::HandleType> events, bool waitAll = false, uint32_t milli = 0xFFFF'FFFF);

    template<typename T> concept Eventable = std::derived_from<T, Handle> || std::same_as<T, Handle::HandleType>;
    // TODO: this can just return a non-optional more simply
    template<Eventable...E >
    uint32_t WaitAnyEvent(const E&...events)
    {
        Event::HandleType handles[] = { events... };
        return *WaitOnMultipleEvents(handles);
    }

    template<class D, Eventable...E>
    std::optional<uint32_t> WaitAnyEventFor(D timeout, const E&...events)
    {
        Event::HandleType handles[] = { events... };
        const auto milli = (uint32_t)std::chrono::duration_cast<std::chrono::milliseconds>(timeout).count();
        return WaitOnMultipleEvents(handles, false, milli);
    }
}
