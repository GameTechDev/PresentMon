// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <Core/source/win/Process.h>
#include <Core/source/win/EventHookManager.h>
#include <Core/source/gfx/Graphics.h>
#include <Core/source/win/KernelWindow.h>
#include <Core/source/infra/util/IntervalWaiter.h>
#include <memory>
#include <vector>
#include <atomic>
#include "WindowMoveHandler.h"
#include "WindowActivateHandler.h"
#include "OverlaySpec.h"
#include "GraphDataPack.h"
#include "TextDataPack.h"

namespace p2c::gfx::lay
{
    class Element;
    class TextElement;
}

namespace p2c::pmon
{
    class RawFrameDataWriter;
    class PresentMon;
}

namespace p2c::kern
{
    // TODO: more aggressive forward decl of dependencies for Overlay member data
    class Overlay
    {
    public:
        // functions
        Overlay(
            win::Process proc_,
            std::shared_ptr<OverlaySpec> pSpec_, 
            pmon::PresentMon* pm_,
            std::map<size_t, GraphDataPack> graphPacks_ = {},
            std::optional<gfx::Vec2I> pos = {});
        ~Overlay();
        void UpdateTargetRect(const gfx::RectI& newRect);
        void UpdateTargetOrder(bool topmost);
        void RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_);
        void InitiateClose();
        void RunTick();
        void SetCaptureState(bool active, std::wstring path, std::wstring name);
        bool IsTargetLive() const;
        bool IsStandardWindow() const;
        const win::Process& GetProcess() const;
        void UpdateTargetFullscreenStatus();
        bool NeedsFullscreenReboot() const;
        const OverlaySpec& GetSpec() const;
        std::unique_ptr<Overlay> SacrificeClone(std::optional<HWND> hWnd_ = {}, std::shared_ptr<OverlaySpec> pSpec_ = {});
        std::unique_ptr<Overlay> RetargetPidClone(win::Process proc_);
        const gfx::RectI& GetTargetRect() const;
    private:
        // functions
        void AdjustOverlaySituation_(OverlaySpec::OverlayPosition position);
        void UpdateGraphData_(double timestamp);
        void Render_();
        void UpdateCaptureStatusText_();
        void UpdateDataSets_();
        std::unique_ptr<win::KernelWindow> MakeWindow_(std::optional<gfx::Vec2I> pos_);
        gfx::Vec2I CalculateOverlayPosition_() const;
        bool IsHidden_() const;
        // types
        enum class HideMode
        {
            Never,
            Capture,
            Always,
        };
        // data
        win::Process proc;
        std::shared_ptr<OverlaySpec> pSpec;
        pmon::PresentMon* pm;
        std::map<size_t, GraphDataPack> graphPacks;
        HANDLE hProcess;
        win::EventHookManager::Token moveHandlerToken;
        win::EventHookManager::Token activateHandlerToken;
        // map metric indices to DataPacks (for graphs)
        std::vector<TextDataPack> textPacks;
        OverlaySpec::OverlayPosition position;
        float upscaleFactor = 2.f;
        gfx::DimensionsI graphicsDimensions;
        gfx::DimensionsI windowDimensions;
        gfx::RectI targetRect;
        std::atomic<bool> targetFullscreen;
        std::unique_ptr<win::KernelWindow> pWindow;
        gfx::Graphics gfx;
        std::shared_ptr<gfx::lay::Element> pRoot;
        std::shared_ptr<pmon::RawFrameDataWriter> pWriter;
        std::shared_ptr<gfx::lay::TextElement> pCaptureIndicatorText;
        int samplingPeriodMs;
        int samplesPerFrame;
        infra::util::IntervalWaiter samplingWaiter;
        bool hideDuringCapture;
        bool hideAlways;
        std::optional<std::chrono::high_resolution_clock::time_point> lastMoveTime;
    };
}