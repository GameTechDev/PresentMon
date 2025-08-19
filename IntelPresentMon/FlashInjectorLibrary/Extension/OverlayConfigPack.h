#pragma once
#include <atomic>
#include <mutex>
#include "OverlayConfig.h"

namespace GfxLayer::Extension
{
    class OverlayConfigPack
    {
    public:
        bool IsDirty() const;
        GfxLayer::Extension::OverlayConfig Read() const;
        void Update(const GfxLayer::Extension::OverlayConfig& cfg);
        static OverlayConfigPack& Get();
    private:
        mutable std::atomic<bool> dirty = false;
        mutable std::mutex configMtx;
        GfxLayer::Extension::OverlayConfig data{};
    };
}