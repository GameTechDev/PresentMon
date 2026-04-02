#pragma once
#include <memory>
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"

int PacedFramePlaybackTest(std::unique_ptr<pmapi::Session> pSession);
