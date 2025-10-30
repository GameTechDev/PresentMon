// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/Process.h>
#include <Core/source/win/EventHookManager.h>
#include <Core/source/gfx/Graphics.h>
#include <Core/source/win/KernelWindow.h>
#include <CommonUtilities/IntervalWaiter.h>
#include <CommonUtilities/win/Handle.h>
#include <memory>
#include <vector>
#include <atomic>
#include <unordered_map>
#include "WindowMoveHandler.h"
#include "WindowActivateHandler.h"
#include "OverlaySpec.h"
#include "MetricPackMapper.h"

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
            ::pmon::util::win::Process proc_,
            std::shared_ptr<OverlaySpec> pSpec_, 
            pmon::PresentMon* pm_,
            std::unique_ptr<MetricPackMapper> pPackMapper_,
            bool headless,
            std::optional<gfx::Vec2I> pos = {});
        ~Overlay();
        void UpdateTargetRect(const gfx::RectI& newRect);
        void UpdateTargetOrder(bool topmost);
        void RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_);
        void InitiateClose();
        void RunTick();
        void SetCaptureState(bool active);
        bool IsTargetLive() const;
        bool IsStandardWindow() const;
        const ::pmon::util::win::Process& GetProcess() const;
        void UpdateTargetFullscreenStatus();
        bool NeedsFullscreenReboot() const;
        const OverlaySpec& GetSpec() const;
        std::unique_ptr<Overlay> SacrificeClone(std::optional<HWND> hWnd_ = {}, std::shared_ptr<OverlaySpec> pSpec_ = {});
        std::unique_ptr<Overlay> RetargetPidClone(::pmon::util::win::Process proc_);
        const gfx::RectI& GetTargetRect() const;
        bool IsHeadless() const;
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
        class TaskScheduler
        {
            enum Task_ : size_t
            {
                Poll_,
                Render_,
                Trace_,
                Count_,
            };
            using nano = std::chrono::nanoseconds;
        public:
            TaskScheduler(size_t pollRate, size_t renderRate, size_t traceRate);
            nano GetNextWait();
            bool AtPoll() const;
            bool AtRender() const;
            bool AtTrace() const;
        private:
            std::chrono::duration<double, std::milli> tickDuration_;
            size_t periods_[Count_];
            size_t remainings_[Count_];
        };
        // data
        ::pmon::util::win::Process proc;
        std::shared_ptr<OverlaySpec> pSpec;
        TaskScheduler scheduler_;
        pmon::PresentMon* pm;
        pmon::MetricFetcherFactory fetcherFactory;
        std::unique_ptr<MetricPackMapper> pPackMapper;
        ::pmon::util::win::Handle hProcess;
        win::EventHookManager::Token moveHandlerToken;
        win::EventHookManager::Token activateHandlerToken;
        OverlaySpec::OverlayPosition position;
        float upscaleFactor = 2.f;
        gfx::DimensionsI graphicsDimensions;
        gfx::DimensionsI windowDimensions;
        gfx::RectI targetRect;
        std::atomic<bool> targetFullscreen;
        std::unique_ptr<win::KernelWindow> pWindow;
        std::unique_ptr<gfx::Graphics> pGfx;
        std::shared_ptr<gfx::lay::Element> pRoot;
        std::shared_ptr<pmon::RawFrameDataWriter> pWriter;
        std::shared_ptr<gfx::lay::TextElement> pCaptureIndicatorText;
        ::pmon::util::IntervalWaiter samplingWaiter;
        bool hideDuringCapture;
        bool hideAlways;
        std::optional<std::chrono::high_resolution_clock::time_point> lastMoveTime;
        bool headless;
    };
}