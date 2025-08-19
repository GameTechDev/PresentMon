// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <string>
#include <thread>
#include <condition_variable>
#include <mutex>
#include <optional>
#include <string>
#include <semaphore>
#include <CommonUtilities/win/Process.h>
#include <Core/source/pmon/PresentMon.h>
#include <CommonUtilities/mt/Thread.h>
#include "OverlaySpec.h"
#include "KernelHandler.h"
#include "../../../FlashInjectorLibrary/Extension/OverlayConfig.h"

#pragma comment(lib, "user32")
#pragma comment(lib, "winmm")

namespace p2c::gfx
{
    class Graphics;
    namespace lay
    {
        class GraphData;
    }
}

namespace p2c::kern
{
    class InjectorComplex;
    struct OverlaySpec;
    class OverlayContainer;

    struct Process : public ::pmon::util::win::Process
    {
        Process(::pmon::util::win::Process base) : ::pmon::util::win::Process{ std::move(base) } {}
        std::optional<std::wstring> windowName;
    };

    class Kernel
    {
    public:
        Kernel(KernelHandler* pHandler);
        Kernel(const Kernel&) = delete;
        Kernel& operator=(const Kernel&) = delete;
        ~Kernel();
        void PushSpec(std::unique_ptr<OverlaySpec> pSpec);
        void UpdateInjection(bool enableInjection, std::optional<uint32_t> currentlyTargettedPid,
            std::optional<std::string> overrideTargetName,
            const GfxLayer::Extension::OverlayConfig& cfg);
        void ClearOverlay();
        void SetAdapter(uint32_t id);
        std::vector<pmon::AdapterInfo> EnumerateAdapters() const;
        void SetCapture(bool active);
        const pmapi::intro::Root& GetIntrospectionRoot() const;
    private:
        // functions
        bool IsIdle_() const;
        std::unique_ptr<OverlaySpec> PullSpec_();
        void HandleMarshalledException_() const;
        // top level root acts like state machine for spawning/running overlay
        void ThreadProcedure_();
        // loop runs while overlay window active, holds message pump etc.
        void RunOverlayLoop_();
        void ConfigurePresentMon_(const OverlaySpec& newSpec);
        // data
        KernelHandler* pHandler = nullptr;
        std::optional<pmon::PresentMon> pm; // optional to defer creation to when the thread is run
        bool dying = false;
        bool clearRequested = false;
        bool inhibitTargetLostSignal = false;
        std::optional<bool> pushedCaptureActive;
        std::unique_ptr<OverlaySpec> pPushedSpec;
        std::unique_ptr<OverlayContainer> pOverlayContainer;
        std::unique_ptr<InjectorComplex> pInjectorComplex;
        mutable std::condition_variable cv;
        mutable std::mutex mtx;
        std::binary_semaphore constructionSemaphore;
        std::exception_ptr marshalledException;
        std::atomic<bool> hasMarshalledException = false;
        ::pmon::util::mt::Thread thread;
    };
}