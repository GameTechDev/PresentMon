#pragma once
#include "../CommonUtilities/Exception.h"

namespace pmon::tel
{
    PM_DEFINE_EX(TelementryException);

    PM_DEFINE_EX_FROM(TelementryException, TelemetrySubsystemAbsent);

    using ::pmon::util::Except;
}
