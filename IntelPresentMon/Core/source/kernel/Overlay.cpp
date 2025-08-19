// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Overlay.h"
#include <Core/source/gfx/layout/GraphElement.h>
#include <Core/source/gfx/layout/FlexElement.h>
#include <Core/source/gfx/layout/TextElement.h>
#include <Core/source/gfx/layout/ReadoutElement.h>
#include <Core/source/pmon/PresentMon.h>
#include <Core/source/pmon/RawFrameDataWriter.h>
#include <Core/source/pmon/Timekeeper.h>
#include <Core/source/gfx/layout/style/StyleProcessor.h>
#include <Core/source/win/StandardWindow.h>
#include <Core/source/win/OverlayWindow.h>
#include <Core/source/cli/CliOptions.h>
#include <ranges>
#include <set>
#include <chrono>
#include <format>
#include <thread>
#include <array>
#include <cassert>
#include "TargetLostException.h"
#include "MetricPackMapper.h"
#include <PresentMonAPIWrapper/StaticQuery.h>
#include <CommonUtilities/Exception.h>


namespace p2c::kern
{
    using namespace gfx;
    using namespace lay;
    using namespace ::pmon::util;

    PM_DEFINE_EX(OverlayDocumentException);

    // free functions used by Overlay
    namespace
    {
        auto MakeDocument_(
            gfx::Graphics& gfx,
            const OverlaySpec& spec,
            MetricPackMapper& mapper,
            pmon::MetricFetcherFactory& fetcherFactory,
            std::shared_ptr<TextElement>& captureIndicator)
        {
            auto pRoot = FlexElement::Make({}, { "doc" });
            std::shared_ptr<gfx::lay::Element> pReadoutContainer;

            for (const auto& w : spec.widgets) {
                try {
                    if (auto pGraphSpec = std::get_if<GraphSpec>(&w)) {
                        auto packsData = pGraphSpec->metrics |
                            std::views::transform([&](const GraphMetricSpec& gms) {
                                auto metInfo = fetcherFactory.GetMetricInfo(gms.metric);
                                return std::make_shared<GraphLinePack>(GraphLinePack{
                                    .data = mapper[gms.metric].graphData,
                                    .axisAffinity = gms.axisAffinity,
                                    .label = std::move(metInfo.fullName),
                                    .units = std::move(metInfo.unitLabel),
                                });
                            }) | rn::to<std::vector>();
                        pRoot->AddChild(GraphElement::Make(
                            pGraphSpec->type, std::move(packsData), { pGraphSpec->tag }
                        ));
                        pReadoutContainer.reset();
                    }
                    else if (auto pReadoutSpec = std::get_if<ReadoutSpec>(&w)) {
                        if (!pReadoutContainer) {
                            pReadoutContainer = gfx::lay::FlexElement::Make({}, { "readout-container" });
                            pRoot->AddChild(pReadoutContainer);
                        }

                        auto metInfo = fetcherFactory.GetMetricInfo(pReadoutSpec->metric);
                        pReadoutContainer->AddChild(gfx::lay::ReadoutElement::Make(
                            metInfo.isNonNumeric, std::move(metInfo.fullName), std::move(metInfo.unitLabel),
                            mapper[pReadoutSpec->metric].textData.get(), { pReadoutSpec->tag }
                        ));
                    }
                    else {
                        throw Except<OverlayDocumentException>("Bad widget variant");
                    }
                }
                catch (...) {
                    pmlog_warn("Failed building a widget into document");
                }
            }

            // capture state indicator
            pRoot->AddChild(FlexElement::Make(
                {
                    TextElement::Make(L"Capture Status:", {"label"}),
                    captureIndicator = TextElement::Make(L"-------------------", {"value"}),
                },
                { "cap" }
            ));

            // trigger layout calculation of entire document
            pRoot->FinalizeAsRoot(DimensionsSpec{ (float)spec.overlayWidth }, spec.sheets, gfx);

            return pRoot;
        }
    }



    // Overlay member functions

    Overlay::Overlay(
        ::pmon::util::win::Process proc_,
        std::shared_ptr<OverlaySpec> pSpec_,
        pmon::PresentMon* pm_,
        std::unique_ptr<MetricPackMapper> pPackMapper_,
        std::optional<gfx::Vec2I> pos_)
        :
        proc{ std::move(proc_) },
        pm{ pm_ },
        pSpec{ std::move(pSpec_) },
        scheduler_{ pSpec->metricPollRate, pSpec->overlayDrawRate, 10 },
        fetcherFactory{ *pm },
        pPackMapper{ std::move(pPackMapper_) },
        hProcess{ OpenProcess(SYNCHRONIZE, TRUE, proc.pid) },
        moveHandlerToken{ win::EventHookManager::AddHandler(std::make_shared<WindowMoveHandler>(proc, this)) },
        activateHandlerToken{ win::EventHookManager::AddHandler(std::make_shared<WindowActivateHandler>(proc, this)) },
        targetRect{ win::GetWindowClientRect(proc.hWnd) },
        position{ pSpec->overlayPosition },
        upscaleFactor{ pSpec->upscale ? pSpec->upscaleFactor : 1.f },
        graphicsDimensions{ pSpec->overlayWidth, 240 },
        windowDimensions{ Dimensions{ graphicsDimensions } * upscaleFactor },
        pWindow{ MakeWindow_(pos_) },
        gfx{ pWindow->GetHandle(), graphicsDimensions, upscaleFactor, cli::Options::Get().allowTearing, !cli::Options::Get().disableAlpha },
        hideDuringCapture{ pSpec->hideDuringCapture },
        hideAlways{ pSpec->hideAlways },
        samplingWaiter{ 1.f / pSpec->metricPollRate }
    {
        UpdateDataSets_();
        pRoot = MakeDocument_(gfx, *pSpec, *pPackMapper, fetcherFactory, pCaptureIndicatorText);
        UpdateCaptureStatusText_();
        AdjustOverlaySituation_(position);
        pm->StartTracking(proc.pid);
    }

    Overlay::~Overlay()
    {
        if (pm) {
            try { pm->StopTracking(); }
            catch (...) {}
        }
    }

    void Overlay::UpdateDataSets_()
    {
        // loop all widgets
        for (auto& w : pSpec->widgets) {
            // if widget is a graph
            if (auto pGraphSpec = std::get_if<GraphSpec>(&w)) {
                // loop all lines in graph
                for (auto& gms : pGraphSpec->metrics) {
                    pPackMapper->AddGraph(gms.metric, pSpec->graphDataWindowSize);
                }
            }
            // if widget is a readout
            else if (auto pReadoutSpec = std::get_if<ReadoutSpec>(&w)) {
                pPackMapper->AddReadout(pReadoutSpec->metric);
            }
        }
        // remove stale data packs, register new query, fill new fetchers
        pPackMapper->CommitChanges(proc.pid, pSpec->averagingWindowSize,
            pSpec->metricsOffset, fetcherFactory);
    }

    std::unique_ptr<win::KernelWindow> Overlay::MakeWindow_(std::optional<Vec2I> pos_)
    {
        UpdateTargetFullscreenStatus();
        std::unique_ptr<win::KernelWindow> pWindow;
        if (pSpec->independentKernelWindow) {
            // try and get the position of the control (CEF) window for calculating independent metrics window pos
            // TODO: connect this more assuredly to the cef control window
            // tried CefBrowserHost::GetWindowHandle, but it causes crashes for some unknown reason
            // should make this at least less brittle with respect to window classname / title
            bool bringToFrontOnCreation = false;
            if (!pos_) {
                bringToFrontOnCreation = true;
                pos_ = Vec2I{ CW_USEDEFAULT, CW_USEDEFAULT };
                if (auto hWndControl = FindWindowA("BrowserWindowClass", "Intel PresentMon")) {
                    RECT controlRect{};
                    if (GetWindowRect(hWndControl, &controlRect)) {
                        pos_ = Vec2I{ controlRect.left + 25, controlRect.top + 25 };
                    }
                    else {
                        pmlog_warn("failed to get rect of control window").hr();
                    }
                }
                else {
                    pmlog_warn("failed to find control window");
                }
            }
            // make the metrics window
            pWindow = std::make_unique<win::StandardWindow>(
                pos_->x, pos_->y,
                windowDimensions,
                L"PresentMon Data Display",
                bringToFrontOnCreation
            );
        }
        else {
            const auto pos = CalculateOverlayPosition_();
            pWindow = std::make_unique<win::OverlayWindow>(
                targetFullscreen,
                pos.x, pos.y,
                windowDimensions,
                L"P2C#OVERLAY"
            );
            // place overlay just above target in the z-order
            pWindow->Reorder(proc.hWnd);
        }
        return pWindow;
    }

    void Overlay::RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_)
    {
        pSpec = std::move(pSpec_);
        UpdateDataSets_();
        pRoot = MakeDocument_(gfx, *pSpec, *pPackMapper, fetcherFactory, pCaptureIndicatorText);
        UpdateCaptureStatusText_();
        scheduler_ = { pSpec->metricPollRate, pSpec->overlayDrawRate, 10 },
        hideDuringCapture = pSpec->hideDuringCapture;
        hideAlways = pSpec->hideAlways;
        AdjustOverlaySituation_(pSpec->overlayPosition);
        if (IsHidden_())
        {
            pWindow->Hide();
        }
        else
        {
            pWindow->Show();
            pWindow->Reorder(proc.hWnd);
        }
    }

    void Overlay::AdjustOverlaySituation_(OverlaySpec::OverlayPosition position_)
    {
        if (const DimensionsI newDims = pRoot->GetElementDims(); newDims != graphicsDimensions)
        {
            graphicsDimensions = newDims;
            windowDimensions = Dimensions{ graphicsDimensions } * upscaleFactor;
            pWindow->Resize(windowDimensions);
            if (!pWindow->Standard()) {
                pWindow->Move(CalculateOverlayPosition_());
            }
            gfx.Resize(graphicsDimensions);
            position = position_;
        }
        else if (position != position_)
        {
            position = position_;
            if (!pWindow->Standard()) {
                pWindow->Move(CalculateOverlayPosition_());
            }
        }
    }

    void Overlay::UpdateGraphData_(double timestamp)
    {
        if (!IsTargetLive()) {
            throw TargetLostException{};
        }
        pPackMapper->Populate(pm->GetTracker(), timestamp);
    }

    void Overlay::UpdateTargetRect(const RectI& newRect)
    {
        if (pWindow->Standard()) {
            // if we are a independent overlay (which is a standard) window, don't move when target moves
            return;
        }

        if (targetRect != newRect) {
            targetRect = newRect;
            pWindow->Move(CalculateOverlayPosition_());
        }
        // hide window during move, record timepoint to determine when to show again
        pWindow->Hide();
        lastMoveTime = std::chrono::high_resolution_clock::now();
    }

    void Overlay::UpdateTargetOrder(bool topmost)
    {
        if (pWindow->Standard()) {
            // if we are a independent overlay (which is a standard) window, don't do reordering
            return;
        }

        if (pWindow->Fullscreen()) {
            // if we are a fullscreen attached overlay, always stay on top
            // TODO: examine how topmost is set on SOTTR with logging to improve this fullscreen kludge
            pWindow->SetTopmost();
        }
        else {
            if (topmost) {
                // this function called with topmost=true when a window activation happens
                // and the activated window is the target, so we need topmost to get on top
                // of it
                pWindow->SetTopmost();
            }
            else {
                // if other window was activated, we need to stop being topmost and insert
                // ourself just above the target in the window order
                pWindow->ClearTopmost();
                pWindow->Reorder(proc.hWnd);
            }
        }
    }

    Vec2I Overlay::CalculateOverlayPosition_() const
    {
        if (position == OverlaySpec::OverlayPosition::Center)
        {
            pmlog_error("center overlay position unimplimented");
            throw Except<Exception>();
        }

        int x, y;
        if (position == OverlaySpec::OverlayPosition::TopLeft || position == OverlaySpec::OverlayPosition::BottomLeft)
        {
            x = targetRect.left;
        }
        else
        {
            x = targetRect.right - windowDimensions.width;
        }
        if (position == OverlaySpec::OverlayPosition::TopLeft || position == OverlaySpec::OverlayPosition::TopRight)
        {
            y = targetRect.top;
        }
        else
        {
            y = targetRect.bottom - windowDimensions.height;
        }

        return { x, y };
    }

    bool Overlay::IsHidden_() const
    {
        return (pWriter && hideDuringCapture) || hideAlways;
    }

    void Overlay::Render_()
    {
        // update window contents
        gfx.BeginFrame();
        pRoot->Draw(gfx);
        gfx.EndFrame();
    }

    void Overlay::UpdateCaptureStatusText_()
    {
        if (pWriter) {
            pCaptureIndicatorText->SetText(L"In Progress");
        }
        else {
            pCaptureIndicatorText->SetText(L"Standing By");
        }
    }

    void Overlay::InitiateClose()
    {
        pWindow->Close();
    }

    void Overlay::RunTick()
    {
        const auto wait = scheduler_.GetNextWait();
        samplingWaiter.SetInterval(wait);
        samplingWaiter.Wait();
        pmon::Timekeeper::LockNow();

        if (scheduler_.AtPoll() && !IsHidden_()) {
            pmlog_mark mkPoll;
            UpdateGraphData_(pmon::Timekeeper::GetLockedNow());
            pmlog_perf(v::overlay)("Data update time").mark(mkPoll);
        }
        if (scheduler_.AtRender()) {
            // handle hide during move logic (show if time elapsed and now otherwise hidden)
            if (lastMoveTime) {
                const auto now = std::chrono::high_resolution_clock::now();
                if (std::chrono::duration<float>(now - *lastMoveTime).count() >= 0.1f) {
                    lastMoveTime = {};
                    if (!IsHidden_()) {
                        pWindow->Show();
                    }
                }
            }
            if (!IsHidden_()) {
                pmlog_mark mkRender;
                Render_();
                pmlog_perf(v::overlay)("Overlay draw time").mark(mkRender);
            }
        }
        if (scheduler_.AtTrace() && pWriter) {
            pWriter->Process();
        }
    }

    void Overlay::SetCaptureState(bool active, std::wstring path, std::wstring name)
    {
        pmlog_info(std::format("Capture set to {}", active));

        if (active && !pWriter)
        {
            const std::chrono::zoned_time now{ std::chrono::current_zone(), std::chrono::system_clock::now() };
            auto fullPath = std::format(L"{0}{1}-{3}-{2:%y}{2:%m}{2:%d}-{2:%H}{2:%M}{2:%OS}.csv", path, name, now, proc.name);
            // create optional path for stats file
            auto fullStatsPath = [&]() -> std::optional<std::wstring> {
                if (pSpec->generateStats) {
                    return std::format(L"{0}{1}-{3}-{2:%y}{2:%m}{2:%d}-{2:%H}{2:%M}{2:%OS}-stats.csv", path, name, now, proc.name);
                }
                else {
                    return std::nullopt;
                }
            }();
            pWriter = { pm->MakeRawFrameDataWriter(std::move(fullPath), std::move(fullStatsPath), proc.pid, proc.name) };
        }
        else if (!active && pWriter)
        {
            pWriter.reset();
        }

        // handle window visibility
        if (IsHidden_())
        {
            pWindow->Hide();
        }
        else
        {
            pWindow->Show();
            pWindow->Reorder(proc.hWnd);
        }

        // update indicator on overlay
        UpdateCaptureStatusText_();
    }

    bool Overlay::IsTargetLive() const
    {
        const auto ret = WaitForSingleObject(hProcess, 0);
        if (ret == WAIT_FAILED)
        {
            pmlog_error().hr();
        }
        return ret != WAIT_OBJECT_0;
    }

    bool Overlay::IsStandardWindow() const
    {
        return pWindow->Standard();
    }

    const ::pmon::util::win::Process& Overlay::GetProcess() const
    {
        return proc;
    }

    void Overlay::UpdateTargetFullscreenStatus()
    {
        if (const auto hMon = MonitorFromWindow(proc.hWnd, MONITOR_DEFAULTTONULL)) {
            MONITORINFOEXW monInfo{};
            monInfo.cbSize = (DWORD)sizeof(MONITORINFOEXW);
            if (GetMonitorInfoW(hMon, &monInfo) != 0) {
                const auto monRect = win::RectToRectI(monInfo.rcMonitor);
                targetFullscreen = targetRect == monRect;
            }
            else {
                pmlog_warn("didn't get monitor info");
            }
        }
        else {
            pmlog_warn("didn't get monitor from window");
        }
    }

    bool Overlay::NeedsFullscreenReboot() const
    {
        return !pWindow->Standard() && (targetFullscreen != pWindow->Fullscreen());
    }

    const OverlaySpec& Overlay::GetSpec() const
    {
        return *pSpec;
    }

    std::unique_ptr<Overlay> Overlay::SacrificeClone(std::optional<HWND> hWnd_, std::shared_ptr<OverlaySpec> pSpec_)
    {
        pmlog_info("doing SacrificeClone");

        std::optional<Vec2I> pos;
        if (pWindow->Standard()) {
            pos = pWindow->GetPosition();
        }

        if (!pSpec_) {
            pSpec_ = std::move(pSpec);
        }

        proc.hWnd = hWnd_.value_or(proc.hWnd);

        auto pNewOverlay = std::make_unique<Overlay>(
            proc,
            std::move(pSpec_),
            pm,
            std::move(pPackMapper),
            pos
        );
        // clear pm so that stream isn't closed when this overlay dies
        pm = nullptr;
        pNewOverlay->pWriter = std::move(pWriter);
        pNewOverlay->UpdateCaptureStatusText_();
        return pNewOverlay;
    }
    std::unique_ptr<Overlay> Overlay::RetargetPidClone(::pmon::util::win::Process proc_)
    {
        pmlog_info("doing RetargetPidClone");

        std::optional<Vec2I> pos;
        if (pWindow->Standard()) {
            pos = pWindow->GetPosition();
        }

        pm->StopTracking();
        auto pNewOverlay = std::make_unique<Overlay>(
            proc_,
            std::move(pSpec),
            pm,
            std::make_unique<MetricPackMapper>(),
            pos
        );
        // clear pm so that stream isn't closed when this overlay dies
        // (because we manually close it above)
        pm = nullptr;
        return pNewOverlay;
    }

    const gfx::RectI& Overlay::GetTargetRect() const
    {
        return targetRect;
    }

    Overlay::TaskScheduler::TaskScheduler(size_t pollRate, size_t renderRate, size_t traceRate)
    {
        assert(pollRate != 0);
        assert(renderRate != 0);
        assert(traceRate != 0);
        const size_t tickRate = std::lcm(pollRate, std::lcm(renderRate, traceRate));
        periods_[Poll_] = tickRate / pollRate;
        periods_[Render_] = tickRate / renderRate;
        periods_[Trace_] = tickRate / traceRate;
        for (int i = 0; i < Count_; i++) {
            remainings_[i] = periods_[i];
        }
        using namespace std::chrono_literals;
        tickDuration_ = 1s / double(tickRate);
    }
    Overlay::TaskScheduler::nano Overlay::TaskScheduler::GetNextWait()
    {
        // reset all zeros
        for (auto&&[r, p] : vi::zip(remainings_, periods_)) {
            if (r == 0) r = p;
        }
        // find the lowest remaining
        const auto min = *rn::min_element(remainings_);
        // step all by lowest
        for (auto& r : remainings_) {
            r -= min;
        }
        // return step duration
        return std::chrono::duration_cast<std::chrono::nanoseconds>(tickDuration_) * min;
    }
    bool Overlay::TaskScheduler::AtPoll() const
    {
        return remainings_[Poll_] == 0;
    }
    bool Overlay::TaskScheduler::AtRender() const
    {
        return remainings_[Render_] == 0;
    }
    bool Overlay::TaskScheduler::AtTrace() const
    {
        return remainings_[Trace_] == 0;
    }
}