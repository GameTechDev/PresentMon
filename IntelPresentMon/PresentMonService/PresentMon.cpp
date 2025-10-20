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

PresentMon::PresentMon(bool isRealtime)
	:
	etwLogger_{ util::win::WeAreElevated() }
{
	if (isRealtime) {
		pSession_ = std::make_unique<RealtimePresentMonSession>();
	}
	else {
		pSession_ = std::make_unique<MockPresentMonSession>();
	}
}

PresentMon::~PresentMon()
{
	pSession_->CheckTraceSessions(true);
}

PM_STATUS PresentMon::StartStreaming(uint32_t client_process_id, uint32_t target_process_id,
	std::string& nsm_file_name)
{
	return pSession_->StartStreaming(client_process_id, target_process_id, nsm_file_name);
}

void PresentMon::StopStreaming(uint32_t client_process_id, uint32_t target_process_id)
{
	return pSession_->StopStreaming(client_process_id, target_process_id);
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
