#pragma once
#include <cstdint>
#include <guiddef.h>


// interface for injecting a listener callback that will record all provider:event filters
class IFilterBuildListener
{
public:
    virtual void EventAdded(uint16_t Id) = 0;
    virtual void ProviderEnabled(const GUID& providerGuid, uint64_t anyKey, uint64_t allKey, uint8_t maxLevel) = 0;
    virtual void ClearEvents() = 0;
};