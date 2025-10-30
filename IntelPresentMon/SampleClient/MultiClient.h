#pragma once
#include <memory>
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"

// mode used in conjuction with multi-client unit test suite
int MultiClientTest(std::unique_ptr<pmapi::Session> pSession);