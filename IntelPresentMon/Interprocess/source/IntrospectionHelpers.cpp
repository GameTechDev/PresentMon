#include "IntrospectionHelpers.h"
#include "metadata/EnumDataType.h"
#include "IntrospectionMacroHelpers.h"
#include "IntrospectionDataTypeMapping.h"


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

	size_t GetDataTypeAlignment(PM_DATA_TYPE dt)
	{
		return BridgeDataType<DataTypeAlignmentBridger>(dt);
	}
}
