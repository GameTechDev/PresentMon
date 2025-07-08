#include "../IntelPresentMon/CommonUtilities/log/Log.h"
namespace pmon::util::log
{
    std::shared_ptr<class IChannel> GetDefaultChannel() noexcept
    {
        return {};
    }
}