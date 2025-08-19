// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include "Overlay.h"
#include <CommonUtilities/win/com/WbemConnection.h>
#include <CommonUtilities/win/com/WbemListener.h>
#include <CommonUtilities/win/com/ProcessSpawnSink.h>
#include <Core/source/win/ProcessMapBuilder.h>
#include <Core/source/win/EventHookManager.h>
#include "WindowSpawnHandler.h"

namespace p2c::kern
{
	// OverlayContainer represents entire process ancestry, handles event-driven
	// switches between processes within ancestry tree
	class OverlayContainer
	{
	public:
        // functions
        OverlayContainer(::pmon::util::win::com::WbemConnection& wbemConn_, std::shared_ptr<OverlaySpec> pSpec_,
            pmon::PresentMon* pm_);
        ~OverlayContainer() = default;
        void RebuildDocument(std::shared_ptr<OverlaySpec> pSpec_);
        void InitiateClose();
        void RunTick();
        void SetCaptureState(bool active, std::wstring path, std::wstring name);
        bool IsTargetLive() const;
        const ::pmon::util::win::Process& GetProcess() const;
        void UpdateTargetFullscreenStatus();
        const OverlaySpec& GetSpec() const;
        void CheckAndProcessFullscreenTransition();
        void RegisterWindowSpawn(DWORD pid, HWND hWnd, const RECT& r);
        void RebootOverlay(std::shared_ptr<OverlaySpec> pSpec_);
	private:
        // functions
        void HandleProcessSpawnEvents_();
        // data
        ::pmon::util::win::com::WbemConnection& wbemConn;
		std::unique_ptr<Overlay> pOverlay;
        // TODO: listen for process death and prune this map
        std::unordered_map<DWORD, ::pmon::util::win::Process> ancestorMap;
        std::vector<win::EventHookManager::Token> windowSpawnListeners;
        DWORD rootPid = 0;
        DWORD curPid = 0;
        ::pmon::util::win::com::ProcessSpawnSink::EventQueue spawnQueue;
        std::unique_ptr<::pmon::util::win::com::WbemListener> pChildListener;
	};
}