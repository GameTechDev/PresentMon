#pragma once
#include "../CommonUtilities/Exception.h"

namespace pmon::tel
{
    PM_DEFINE_EX(TelementryException);

    PM_DEFINE_EX_FROM(TelementryException, TelemetrySubsystemAbsent);
    PM_DEFINE_EX_FROM(TelementryException, TelemetryCoordinatorException);
    PM_DEFINE_EX_FROM(TelementryException, AdlException);
    PM_DEFINE_EX_FROM(TelementryException, IgclException);
    PM_DEFINE_EX_FROM(TelementryException, NvapiException);
    PM_DEFINE_EX_FROM(TelementryException, NvmlException);
    PM_DEFINE_EX_FROM(TelementryException, UciException);
    PM_DEFINE_EX_FROM(TelementryException, WmiException);

    using ::pmon::util::Except;
}
