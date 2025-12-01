#pragma once
#include <memory>
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"

// mode used in conjuction with polled paced playback unit test suite
int PacedPlaybackTest(std::unique_ptr<pmapi::Session> pSession);