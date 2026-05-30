#include "IntrospectionHelpers.h"
#include "metadata/EnumDataType.h"
#include "IntrospectionMacroHelpers.h"
#include "IntrospectionDataTypeMapping.h"
#include "PmStatusError.h"
#include "../../CommonUtilities/Exception.h"
#include "../../CommonUtilities/log/Log.h"
#include <cassert>


namespace pmon::ipc::intro
{
	template<PM_DATA_TYPE dt> struct DataTypeSizeBridger {
		static size_t Invoke() { return DataTypeToStaticType_sz<dt>; }
		static size_t Default() { return 0ull; }
	};

	template<PM_DATA_TYPE dt> struct DataTypeAlignmentBridger {
		static size_t Invoke() { return DataTypeToStaticType_align<dt>; }
		static size_t Default() { return 1ull; }
	};

	size_t GetDataTypeSize(PM_DATA_TYPE dt)
	{
		return BridgeDataType<DataTypeSizeBridger>(dt);
	}

	size_t GetDataTypeSizeChecked(PM_DATA_TYPE dt)
	{
		const auto dataSize = GetDataTypeSize(dt);
		assert(dataSize != 0);
		if (dataSize == 0) {
			pmlog_error("Unsupported query data type").pmwatch((int)dt).code(PM_STATUS_QUERY_MALFORMED).diag();
			throw util::Except<PmStatusError>(PM_STATUS_QUERY_MALFORMED, "Unsupported query data type");
		}
		return dataSize;
	}

	size_t GetDataTypeAlignment(PM_DATA_TYPE dt)
	{
		return BridgeDataType<DataTypeAlignmentBridger>(dt);
	}
}
