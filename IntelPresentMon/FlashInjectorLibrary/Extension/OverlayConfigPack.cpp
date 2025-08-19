#include "OverlayConfigPack.h"

namespace GfxLayer::Extension
{
    bool OverlayConfigPack::IsDirty() const
    {
        return dirty.load();
    }
    GfxLayer::Extension::OverlayConfig OverlayConfigPack::Read() const
    {
        dirty.store(false);
        std::lock_guard lk{ configMtx };
        return data;
    }
    void OverlayConfigPack::Update(const GfxLayer::Extension::OverlayConfig& cfg)
    {
        std::lock_guard lk{ configMtx };
        data = cfg;
        dirty.store(true);
    }
    OverlayConfigPack& OverlayConfigPack::Get()
    {
        static OverlayConfigPack pack;
        return pack;
    }
}