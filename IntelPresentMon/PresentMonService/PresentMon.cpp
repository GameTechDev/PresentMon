// Copyright (C) 2022-2023 Intel Corporation
// SPDX-License-Identifier: MIT
#include "PresentMon.h"

#include "CliOptions.h"
#include "Logging.h"
#include "..\CommonUtilities\str\String.h"
#include "RealtimePresentMonSession.h"
#include "MockPresentMonSession.h"
#include <VersionHelpers.h>
#include <shlwapi.h>
#include <span>
#include "../CommonUtilities/win/Privileges.h"

PresentMon::PresentMon(svc::FrameBroadcaster& broadcaster,
	bool isRealtime)
	:
	broadcaster_{ broadcaster },
	etwLogger_{ util::win::WeAreElevated() },
	isRealtime_{ isRealtime }
{
	if (isRealtime) {
		pSession_ = std::make_unique<RealtimePresentMonSession>(broadcaster);
	}
	else {
		pSession_ = std::make_unique<MockPresentMonSession>(broadcaster);
	}
}

PresentMon::~PresentMon()
{
	pSession_->CheckTraceSessions(true);
	pmlog_dbg("PresentMon object destructor finishing");
}

PM_STATUS PresentMon::UpdateTracking(const std::unordered_set<uint32_t>& trackedPids)
{
	return pSession_->UpdateTracking(trackedPids);
}

std::vector<std::shared_ptr<pwr::PowerTelemetryAdapter>> PresentMon::EnumerateAdapters()
{
	// Only the real time trace uses the control libary interface
	return pSession_->EnumerateAdapters();
}

PM_STATUS PresentMon::SelectAdapter(uint32_t adapter_id)
{
	// Only the real time trace uses the control libary interface
	return pSession_->SelectAdapter(adapter_id);
}

void PresentMon::StartPlayback()
{
	if (auto pPlaybackSession = dynamic_cast<MockPresentMonSession*>(pSession_.get())) {
		pPlaybackSession->StartPlayback();
	}
	else {
		pmlog_error("Bad call to start playback on a non-playback session");
	}
}

void PresentMon::StopPlayback()
{
	if (auto pPlaybackSession = dynamic_cast<MockPresentMonSession*>(pSession_.get())) {
		pPlaybackSession->StopPlayback();
	}
	else {
		pmlog_error("Bad call to stop playback on a non-playback session");
	}
}

void PresentMon::CheckTraceSessions()
{
	pSession_->CheckTraceSessions(false);
}

void PresentMon::StopTraceSessions()
{
	pSession_->CheckTraceSessions(true);
}
