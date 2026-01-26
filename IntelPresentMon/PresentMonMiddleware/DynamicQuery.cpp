#include "DynamicQuery.h"

namespace pmon::mid::todo
{
	PM_DYNAMIC_QUERY::PM_DYNAMIC_QUERY(std::span<PM_QUERY_ELEMENT> qels, ipc::MiddlewareComms& comms)
	{

	}

	size_t PM_DYNAMIC_QUERY::GetBlobSize() const
	{
		return blobSize_;
	}


	void PM_DYNAMIC_QUERY::Poll(uint8_t* pBlobBase, ipc::MiddlewareComms& comms, std::optional<uint32_t> pid) const
	{

	}
}