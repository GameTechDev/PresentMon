// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "OverlayContainer.h"
#include <Core/source/win/ProcessMapBuilder.h>
#include "TargetLostException.h"
#include <Core/source/infra/log/v/ProcWatch.h>

using p2c::infra::log::v::procwatch;

namespace p2c::kern
{
    using infra::log::v::procwatch;
    using namespace gfx;
    using namespace lay;
#define vlog p2cvlog(procwatch)

    OverlayContainer::OverlayContainer(
        win::com::WbemConnection& wbemConn_, std::shared_ptr<OverlaySpec> pSpec_,
        pmon::PresentMon* pm_, std::map<size_t, GraphDataPack> graphPacks_)
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
                vlog.note(std::format(L"overlay container found candidate child process|pid:{} hwn:{:6x}",
                    curPid, (uintptr_t)j->second.hWnd)).commit();
                pOverlay = std::make_unique<Overlay>(j->second, std::move(pSpec_), pm_, std::move(graphPacks_));
            }
            // if no windowed child exists, use the root
            else
            {
                curPid = rootPid;
                vlog.note(std::format(L"overlay container using root process|pid:{} hwn:{:6x}",
                    curPid, (uintptr_t)i->second.hWnd)).commit();
                pOverlay = std::make_unique<Overlay>(i->second, std::move(pSpec_), pm_, std::move(graphPacks_));
            }
            // begin listening for children
            pChildListener = wbemConn.MakeListener<win::com::ProcessSpawnSink>(spawnQueue);
            // listen for window spawn events
            // TODO: possibly add window listeners for all processes in ancestor map
            windowSpawnListeners.push_back(win::EventHookManager::AddHandler(
                std::make_shared<WindowSpawnHandler>(curPid, this)
            ));
        }
        else
        {
            p2clog.warn(L"spec-specified pid not in process map").commit();
            throw TargetLostException{};
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
    const win::Process& OverlayContainer::GetProcess() const
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
        p2cvlog(procwatch).note(std::format(L"register-win-spawn-entry | hwn:{:8x}", (uintptr_t)hWnd)).commit();
        // no need for mtx here since the window event listener runs on kernel thread (msg pump)
        if (auto i = ancestorMap.find(pid); i != ancestorMap.end()) {
            const auto prevHwnd = i->second.hWnd;
            // select update hwnd if current is none or invalid
            if (!prevHwnd || !IsWindow(prevHwnd)) {
                p2cvlog(procwatch).note(std::format(L"register-win-spawn-sel-nul | hwn: {:8x} => {:8x}", (uintptr_t)i->second.hWnd, (uintptr_t)hWnd)).commit();
                i->second.hWnd = hWnd;
            }
            // select update hwnd if current smaller
            else {
                RECT prevRect;
                if (!GetWindowRect(prevHwnd, &prevRect)) {
                    prevRect = { 0, 0, 0, 0 };
                }
                if (win::RectToDims(prevRect) < win::RectToDims(r)) {
                    p2cvlog(procwatch).note(std::format(L"register-win-spawn-sel-size | hwn: {:8x}@{} sq px => {:8x}@{} sq px",
                        (uintptr_t)i->second.hWnd, win::RectToDims(prevRect).GetArea(),
                        (uintptr_t)hWnd, win::RectToDims(r).GetArea())).commit();
                    i->second.hWnd = hWnd;
                }
            }

            // if an update occurred, consider retargetting process
            if (i->second.hWnd == hWnd) {
                // case when we are in root and hit a child spawn window of interest
                if (pid != rootPid && curPid == rootPid) {
                    p2cvlog(procwatch).note(std::format(L"register-win-spawn-upg-root-to-child | hwn: {:5} => {:5}", rootPid, pid)).commit();
                    curPid = pid;
                    pOverlay = pOverlay->RetargetPidClone(i->second);
                }
                // case when we are in child and window upgrade occurs
                else if (pid != rootPid && pid == curPid) {
                    p2cvlog(procwatch).note(std::format(L"register-win-spawn-upg-child | hwn: {:8x} => {:8x}", (uintptr_t)prevHwnd, (uintptr_t)hWnd)).commit();
                    // standard overlay doesn't really care what window is being targetted
                    if (!pOverlay->IsStandardWindow()) {
                        pOverlay = pOverlay->SacrificeClone(hWnd);
                    }
                }
                // case of window upgrade in root
                else if (pid == rootPid && curPid == rootPid) {
					p2cvlog(procwatch).note(std::format(L"register-win-spawn-upg-root | hwn: {:8x} => {:8x}", (uintptr_t)prevHwnd, (uintptr_t)hWnd)).commit();
                    // standard overlay doesn't really care what window is being targetted
                    if (!pOverlay->IsStandardWindow()) {
                        pOverlay = pOverlay->SacrificeClone(hWnd);
                    }
				}
            }
        }
        else {
            p2cvlog(procwatch).note(std::format(L"register-win-spawn-reject | hwn: {:8x}", (uintptr_t)hWnd)).commit();
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
            p2cvlog(procwatch).note(std::format(L"handle-proc-spawn | pid: {:5}", spawnProc.pid)).commit();
            // check if event process is part of ancestry, discard if not
            const auto parentId = spawnProc.parentId;
            if (!ancestorMap.contains(parentId)) {
                p2cvlog(procwatch).note(L"handle-proc-spawn-not-ancestor").commit();
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
                    auto pProc = reinterpret_cast<win::Process*>(lParam);

                    DWORD pid = 0;
                    GetWindowThreadProcessId(hWnd, &pid);

                    if (pid == pProc->pid) {
                        if constexpr (procwatch) {
                            RECT r;
                            GetWindowRect(hWnd, &r);
                            p2clog.verbose(std::format(L"handle-proc-spawn-enum-win | pid:{:5} hwd:{:8x} own:{:8x} vis:{} siz:{} nam:{}",
                                pid,
                                reinterpret_cast<uintptr_t>(hWnd),
                                reinterpret_cast<uintptr_t>(GetWindow(hWnd, GW_OWNER)),
                                IsWindowVisible(hWnd),
                                win::RectToDims(r).GetArea(),
                                win::GetWindowTitle(hWnd)
                            )).commit();
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
                p2cvlog(procwatch).note(std::format(L"handle-proc-spawn-sel | pid: {:5}", itProc->second.pid)).commit();
                nextPid = itProc->second.pid;
            }
        }
        // retarget for promotion if required
        if (nextPid) {
            p2cvlog(procwatch).note(std::format(L"handle-proc-spawn-sel-upg | pid: {:5} => {:5}", curPid, *nextPid)).commit();
            curPid = *nextPid;
            pOverlay = pOverlay->RetargetPidClone(ancestorMap[curPid]);
        }
    }
}