#pragma once
#include <memory>
#include "../PresentMonAPIWrapper/PresentMonAPIWrapper.h"

// mode used in conjuction with etl logging unit test suite
int EtlLoggerTest(std::unique_ptr<pmapi::Session> pSession);