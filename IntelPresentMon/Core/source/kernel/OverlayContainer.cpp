// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "OverlayContainer.h"
#include <Core/source/win/ProcessMapBuilder.h>
#include "TargetLostException.h"

using namespace pmon::util;

namespace p2c::kern
{
    using namespace gfx;
    using namespace lay;

    OverlayContainer::OverlayContainer(
        ::pmon::util::win::com::WbemConnection& wbemConn_, std::shared_ptr<OverlaySpec> pSpec_,
        pmon::PresentMon* pm_)
        :
        wbemConn{ wbemConn_ },
        rootPid{ pSpec_->pid }
    {
        win::ProcessMapBuilder builder;
        builder.FilterHavingAncestor(rootPid);
        builder.FillWindowHandles();
        ancestorMap = builder.Extract();
        if (auto i = ancestorMap.find(rootPid); i != ancestorMap.end())
        {
            // try and find a child process with a window
            const auto filter = [root = rootPid](const auto& e) {
                return e.second.pid != root && e.second.hWnd != nullptr;
            };
            if (auto j = std::ranges::find_if(ancestorMap, filter); j != ancestorMap.end())
            {
                curPid = j->second.pid;
                pmlog_verb(v::procwatch)(std::format("overlay container found candidate child process|pid:{} hwn:{:6x}",
                    curPid, (uintptr_t)j->second.hWnd));
                pOverlay = std::make_unique<Overlay>(j->second, std::move(pSpec_), pm_, std::make_unique<MetricPackMapper>());
            }
            // if no windowed child exists, use the root
            else
            {
                curPid = rootPid;
                pmlog_verb(v::procwatch)(std::format("overlay container using root process|pid:{} hwn:{:6x}",
                    curPid, (uintptr_t)i->second.hWnd));
                pOverlay = std::make_unique<Overlay>(i->second, std::move(pSpec_), pm_, std::make_unique<MetricPackMapper>());
            }
            // begin listening for children
            pChildListener = wbemConn.MakeListener<::pmon::util::win::com::ProcessSpawnSink>(spawnQueue);
            // listen for window spawn events
            // TODO: possibly add window listeners for all processes in ancestor map
            windowSpawnListeners.push_back(win::EventHookManager::AddHandler(
                std::make_shared<WindowSpawnHandler>(curPid, this)
            ));
        }
        else
        {
            pmlog_warn("spec-specified pid not in process map");
            throw Except<TargetLostException>();
        }
    }
    void OverlayContainer::RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_)
    {
        pOverlay->RebuildDocument(std::move(pSpec_));
    }
    void OverlayContainer::InitiateClose()
    {
        pOverlay->InitiateClose();
    }
    void OverlayContainer::RunTick()
    {
        HandleProcessSpawnEvents_();
        try {
            pOverlay->RunTick();
        }
        catch (const TargetLostException&) {
            // if lost target is a child process, revert to root
            if (curPid != rootPid) {
                curPid = rootPid;
                pOverlay = pOverlay->RetargetPidClone(ancestorMap[curPid]);
            }
            else throw;
        }
    }
    void OverlayContainer::SetCaptureState(bool active, std::wstring path, std::wstring name)
    {
        pOverlay->SetCaptureState(active, std::move(path), std::move(name));
    }
    bool OverlayContainer::IsTargetLive() const
    {
        return pOverlay->IsTargetLive();
    }
    const ::pmon::util::win::Process& OverlayContainer::GetProcess() const
    {
        return pOverlay->GetProcess();
    }
    void OverlayContainer::UpdateTargetFullscreenStatus()
    {
        pOverlay->UpdateTargetFullscreenStatus();
    }
    const OverlaySpec& OverlayContainer::GetSpec() const
    {
        return pOverlay->GetSpec();
    }
    void OverlayContainer::CheckAndProcessFullscreenTransition()
    {
        if (pOverlay->NeedsFullscreenReboot()) {
            pOverlay = pOverlay->SacrificeClone();
        }
    }
    void OverlayContainer::RegisterWindowSpawn(DWORD pid, HWND hWnd, const RECT& r)
    {
        pmlog_verb(v::procwatch)(std::format("register-win-spawn-entry | hwn:{:8x}", (uintptr_t)hWnd));
        // no need for mtx here since the window event listener runs on kernel thread (msg pump)
        if (auto i = ancestorMap.find(pid); i != ancestorMap.end()) {
            const auto prevHwnd = i->second.hWnd;
            // select update hwnd if current is none or invalid
            if (!prevHwnd || !IsWindow(prevHwnd)) {
                pmlog_verb(v::procwatch)(std::format("register-win-spawn-sel-nul | hwn: {:8x} => {:8x}", (uintptr_t)i->second.hWnd, (uintptr_t)hWnd));
                i->second.hWnd = hWnd;
            }
            // select update hwnd if current smaller
            else {
                RECT prevRect;
                if (!GetWindowRect(prevHwnd, &prevRect)) {
                    prevRect = { 0, 0, 0, 0 };
                }
                if (win::RectToDims(prevRect) < win::RectToDims(r)) {
                    pmlog_verb(v::procwatch)(std::format("register-win-spawn-sel-size | hwn: {:8x}@{} sq px => {:8x}@{} sq px",
                        (uintptr_t)i->second.hWnd, win::RectToDims(prevRect).GetArea(),
                        (uintptr_t)hWnd, win::RectToDims(r).GetArea()));
                    i->second.hWnd = hWnd;
                }
            }

            // if an update occurred, consider retargetting process
            if (i->second.hWnd == hWnd) {
                // case when we are in root and hit a child spawn window of interest
                if (pid != rootPid && curPid == rootPid && win::RectToDims(r).GetArea() >= (640 * 480)) {
                    pmlog_verb(v::procwatch)(std::format("register-win-spawn-upg-root-to-child | hwn: {:5} => {:5}", rootPid, pid));
                    curPid = pid;
                    pOverlay = pOverlay->RetargetPidClone(i->second);
                }
                // case when we are in child and window upgrade occurs
                else if (pid != rootPid && pid == curPid) {
                    pmlog_verb(v::procwatch)(std::format("register-win-spawn-upg-child | hwn: {:8x} => {:8x}", (uintptr_t)prevHwnd, (uintptr_t)hWnd));
                    // standard overlay doesn't really care what window is being targetted
                    if (!pOverlay->IsStandardWindow()) {
                        pOverlay = pOverlay->SacrificeClone(hWnd);
                    }
                }
                // case of window upgrade in root
                else if (pid == rootPid && curPid == rootPid) {
					pmlog_verb(v::procwatch)(std::format("register-win-spawn-upg-root | hwn: {:8x} => {:8x}", (uintptr_t)prevHwnd, (uintptr_t)hWnd));
                    // standard overlay doesn't really care what window is being targetted
                    if (!pOverlay->IsStandardWindow()) {
                        pOverlay = pOverlay->SacrificeClone(hWnd);
                    }
				}
            }
        }
        else {
            pmlog_verb(v::procwatch)(std::format("register-win-spawn-reject | hwn: {:8x}", (uintptr_t)hWnd));
        }
    }
    void OverlayContainer::RebootOverlay(std::shared_ptr<OverlaySpec> pSpec_)
    {
        pOverlay = pOverlay->SacrificeClone({}, std::move(pSpec_));
    }
    void OverlayContainer::HandleProcessSpawnEvents_()
    {
        std::optional<DWORD> nextPid;
        while (auto spawnOpt = spawnQueue.Pop()) {
            auto spawnProc = std::move(*spawnOpt);
            pmlog_verb(v::procwatch)(std::format("handle-proc-spawn | pid: {:5}", spawnProc.pid));
            // check if event process is part of ancestry, discard if not
            const auto parentId = spawnProc.parentId;
            if (!ancestorMap.contains(parentId)) {
                pmlog_verb(v::procwatch)("handle-proc-spawn-not-ancestor");
                continue;
            }
            // pull event off queue and insert into ancestor map
            const auto pid = spawnProc.pid;
            auto [itProc, dummy] = ancestorMap.emplace(pid, std::move(spawnProc));
            // begin listening for window creation events
            // TODO: storing listeners together with Proc info in ancestor map
            // TODO: consider using show listeners to control upgrades
            // TODO: listeners periodically based on some trigger
            windowSpawnListeners.push_back(win::EventHookManager::AddHandler(
                std::make_shared<WindowSpawnHandler>(pid, this)
            ));
            // check for existing window
            struct WindowEnum {
                static BOOL CALLBACK Callback(HWND hWnd, LPARAM lParam)
                {
                    auto pProc = reinterpret_cast<::pmon::util::win::Process*>(lParam);

                    DWORD pid = 0;
                    GetWindowThreadProcessId(hWnd, &pid);

                    if (pid == pProc->pid) {
                        if constexpr (v::procwatch) {
                            RECT r;
                            GetWindowRect(hWnd, &r);
                            pmlog_verb(true)(std::format("handle-proc-spawn-enum-win | pid:{:5} hwd:{:8x} own:{:8x} vis:{} siz:{} nam:{}",
                                pid,
                                reinterpret_cast<uintptr_t>(hWnd),
                                reinterpret_cast<uintptr_t>(GetWindow(hWnd, GW_OWNER)),
                                IsWindowVisible(hWnd),
                                win::RectToDims(r).GetArea(),
                                str::ToNarrow(win::GetWindowTitle(hWnd))
                            ));
                        }

                        // Enumeration only walks top level windows
                        // help prevent hwnd destuction race with getrect status check
                        if (RECT newRect;  GetWindowRect(hWnd, &newRect)) {
                            // filter out windows with zero size
                            if (const auto newDims = win::RectToDims(newRect); newDims.GetArea() > 0) {
                                // get current hwnd rect w/ status check
                                if (RECT oldRect;  GetWindowRect(pProc->hWnd, &oldRect)) {
                                    const auto oldDims = win::RectToDims(oldRect);
                                    // if new win is larger, choose it as the representative hwnd for the pid
                                    if (newDims.GetArea() > oldDims.GetArea()) {
                                        pProc->hWnd = hWnd;
                                    }
                                }
                                // current has no rect: take new
                                else {
                                    pProc->hWnd = hWnd;
                                }
                            }
                        }
                    }
                    // always continue and enumerate all top level windows in system
                    return TRUE;
                }
            };
            EnumWindows(&WindowEnum::Callback, reinterpret_cast<LPARAM>(&itProc->second));
            // check if promotion required
            if (itProc->second.hWnd && curPid == rootPid) {
                pmlog_verb(v::procwatch)(std::format("handle-proc-spawn-sel | pid: {:5}", itProc->second.pid));
                nextPid = itProc->second.pid;
            }
        }
        // retarget for promotion if required
        if (nextPid) {
            pmlog_verb(v::procwatch)(std::format("handle-proc-spawn-sel-upg | pid: {:5} => {:5}", curPid, *nextPid));
            curPid = *nextPid;
            pOverlay = pOverlay->RetargetPidClone(ancestorMap[curPid]);
        }
    }
}