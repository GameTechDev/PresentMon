// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#include "Kernel.h"
#include <Core/source/win/WinAPI.h>
#include <Core/source/win/MessageBox.h>
#include <Core/source/infra/util/Util.h>
#include <Core/source/infra/log/Logging.h>
#include <Core/source/infra/svc/Services.h>
#include <Core/source/infra/util/FolderResolver.h>
#include <random>
#include "OverlayContainer.h"
#include "TargetLostException.h"
#include <Core/source/win/ProcessMapBuilder.h>
#include <Core/source/win/com/WbemConnection.h>

using namespace std::literals;

namespace p2c::kern
{
    Kernel::Kernel(KernelHandler* pHandler) noexcept
        :
        pHandler{ pHandler },
        thread{ &Kernel::ThreadProcedure_, this }
    {}

    Kernel::~Kernel()
    {
        {
            std::lock_guard lk{ mtx };
            dying = true;
        }
        cv.notify_one();
    }

    void Kernel::PushSpec(std::unique_ptr<OverlaySpec> pSpec)
    {
        {
            std::lock_guard lk{ mtx };
            pPushedSpec = std::move(pSpec);
        }
        cv.notify_one();
    }

    void Kernel::ClearOverlay()
    {
        std::lock_guard lk{ mtx };
        if (pOverlayContainer)
        {
            clearRequested = true;
            pPushedSpec.reset();
        }
    }

    std::vector<Process> Kernel::ListProcesses()
    {
        win::ProcessMapBuilder builder;
        builder.FillWindowHandles();
        builder.FilterHavingWindow();
        auto pmap = builder.Extract();

        std::vector<Process> list;
        list.reserve(pmap.size());
        for (auto& entry : pmap)
        {
            Process proc = std::move(entry.second);
            if (proc.hWnd) {
                proc.windowName = win::GetWindowTitle(proc.hWnd);
            }
            list.push_back(std::move(proc));
        }
        return list;
    }

    std::vector<pmon::Metric::Info> Kernel::EnumerateMetrics() const
    {
        std::lock_guard lk{ mtx };
        if (!pm) {
            p2clog.warn(L"presentmon not initialized").commit();
            return {};
        }
        return pm->EnumerateMetrics();
    }

    void Kernel::SetAdapter(uint32_t id)
    {
        std::lock_guard lk{ mtx };
        if (!pm) {
            p2clog.warn(L"presentmon not initialized").commit();
            return;
        }
        pm->SetAdapter(id);
    }

    std::vector<pmon::PresentMon::AdapterInfo> Kernel::EnumerateAdapters() const
    {
        std::lock_guard lk{ mtx };
        if (!pm) {
            p2clog.warn(L"presentmon not initialized").commit();
            return {};
        }
        try { return pm->EnumerateAdapters(); }
        catch (...) { return {}; }
    }

    void Kernel::SetCapture(bool active)
    {
        std::lock_guard lk{ mtx };
        pushedCaptureActive = active;
    }

    bool Kernel::IsIdle_() const
    {
        return !dying && !pPushedSpec && !pOverlayContainer;
    }

    std::unique_ptr<OverlaySpec> Kernel::PullSpec_()
    {
        std::lock_guard g{ mtx };
        return std::move(pPushedSpec);
    }

    void Kernel::ThreadProcedure_()
    {
        try {
            // mutex that prevents frontend from accessing before pmon is connected
            std::unique_lock startLck{ mtx };
            p2clog.info(L"== kernel thread started ==").pid().tid().commit();

            // connect to wbem
            win::com::WbemConnection wbemConn;

            // create the PresentMon object
            try { pm.emplace(); }
            catch (...) {
                pHandler->OnPresentmonInitFailed();
                p2clog.note(L"Failed to init presentmon api").nox().notrace().commit();
                throw;
            }

            startLck.unlock();

            while (!dying)
            {
                try
                {
                    {
                        std::unique_lock u{ mtx };
                        cv.wait(u, [this] {return !IsIdle_(); });
                    }
                    if (pPushedSpec && !dying)
                    {
                        // spawn overlay (container)
                        ConfigurePresentMon_(*pPushedSpec);
                        pOverlayContainer = std::make_unique<OverlayContainer>(wbemConn, std::move(pPushedSpec), &*pm);
                    }
                    if (pOverlayContainer && !dying)
                    {
                        // blocks while overlay is active
                        RunOverlayLoop_();
                    }
                }
                catch (const TargetLostException&)
                {
                    if (pOverlayContainer) {
                        pHandler->OnTargetLost(pOverlayContainer->GetProcess().pid);
                        pOverlayContainer.reset();
                        pPushedSpec.reset();
                    }
                    else {
                        pHandler->OnStalePidSelected();
                    }
                }
                catch (...)
                {
                    pHandler->OnOverlayDied();
                    p2clog.note(L"Overlay died").nox().notrace().commit();
                    pOverlayContainer.reset();
                    pPushedSpec.reset();
                }
            }

            pm.reset();

            p2clog.info(L"== core thread exiting ==").pid().commit();
        }
        catch (...)
        {
            p2clog.note(L"Exiting kernel thread due to uncaught exception").nox().notrace().commit();
        }
    }

    void Kernel::RunOverlayLoop_()
    {
        // this loop runs while the overlay window is active
        while (pOverlayContainer)
        {
            // checking thread sync signals
            {
                std::lock_guard lk{ mtx };
                if (dying || clearRequested)
                {
                    pOverlayContainer->InitiateClose();
                    pPushedSpec.reset();
                    clearRequested = false;
                    inhibitTargetLostSignal = true;
                }
                else if (pPushedSpec)
                {
                    auto& curSpec = pOverlayContainer->GetSpec();
                    if (pPushedSpec->pid != curSpec.pid) {
                        pOverlayContainer->InitiateClose();
                    }
                    else if (pPushedSpec->independentKernelWindow != curSpec.independentKernelWindow ||
                        pPushedSpec->upscale != curSpec.upscale ||
                        (pPushedSpec->upscale && pPushedSpec->upscaleFactor != curSpec.upscaleFactor)) {
                        ConfigurePresentMon_(*pPushedSpec);
                        inhibitTargetLostSignal = true;
                        pOverlayContainer->RebootOverlay(std::move(pPushedSpec));
                    }
                    else {
                        ConfigurePresentMon_(*pPushedSpec);
                        pOverlayContainer->RebuildDocument(std::move(pPushedSpec));
                    }
                }
                else if (pushedCaptureActive)
                {
                    std::wstring path;
                    if (auto pFolder = infra::svc::Services::ResolveOrNull<infra::util::FolderResolver>()) {
                        path = pFolder->Resolve(infra::util::FolderResolver::Folder::Documents) + L"\\Captures\\";
                    }
                    pOverlayContainer->SetCaptureState(*pushedCaptureActive, std::move(path), pOverlayContainer->GetSpec().captureName);
                    pushedCaptureActive.reset();
                }
            }
            pOverlayContainer->CheckAndProcessFullscreenTransition();
            // process windows messages
            {
                MSG msg;
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                    {
                        if (!dying && !inhibitTargetLostSignal) {
                            pHandler->OnTargetLost(pOverlayContainer->GetProcess().pid);
                        }
                        inhibitTargetLostSignal = false;
                        pOverlayContainer.reset();
                        break;
                    }
                    TranslateMessage(&msg);
                    DispatchMessageW(&msg);
                }
            }
            // handle data and overlay
            if (pOverlayContainer)
            {
                pOverlayContainer->RunTick();
            }
        }
    }

    void Kernel::ConfigurePresentMon_(const OverlaySpec& newSpec)
    {
        if (newSpec.metricsOffset != pm->GetOffset())
        {
            pm->SetOffset(newSpec.metricsOffset);
        }
        if (newSpec.averagingWindowSize != pm->GetWindow())
        {
            pm->SetWindow(newSpec.averagingWindowSize);
        }
        if (newSpec.telemetrySamplingPeriodMs != pm->GetGpuTelemetryPeriod())
        {
            pm->SetGpuTelemetryPeriod(newSpec.telemetrySamplingPeriodMs);
        }
    }
}
