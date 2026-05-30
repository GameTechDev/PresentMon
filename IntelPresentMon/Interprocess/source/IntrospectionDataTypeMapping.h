#pragma once
#include "../../PresentMonAPI2/PresentMonAPI.h"
#include "metadata/EnumDataType.h"
#include "metadata/MasterEnumList.h"
#include <utility>
#include <type_traits>

namespace pmon::ipc::intro {
	// static mapping of datatype enum to static type
	template<PM_DATA_TYPE T, PM_ENUM E = PM_ENUM_NULL_ENUM>
	struct DataTypeToStaticType;

	// basic types
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_DOUBLE, PM_ENUM_NULL_ENUM> { using type = double; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_INT32, PM_ENUM_NULL_ENUM> { using type = int32_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_UINT32, PM_ENUM_NULL_ENUM> { using type = uint32_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_STRING, PM_ENUM_NULL_ENUM> { using type = char[260]; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_UINT64, PM_ENUM_NULL_ENUM> { using type = uint64_t; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_BOOL, PM_ENUM_NULL_ENUM> { using type = bool; };
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_VOID, PM_ENUM_NULL_ENUM> { using type = void; };

	// enum specific types
#define HANDLE_ENUM_TYPE(enumFrag, keyFrag, name, shortName, description) \
	template<> struct DataTypeToStaticType<PM_DATA_TYPE_ENUM, PM_ENUM_##keyFrag> { \
		using type = typename std::conditional<PM_ENUM_##keyFrag == PM_ENUM_NULL_ENUM, int, PM_##keyFrag>::type; };

	ENUM_KEY_LIST_ENUM(HANDLE_ENUM_TYPE)

#undef HANDLE_ENUM_TYPE

	// TODO: check completeness of lookup and maybe check the typename matches shortName

	template<PM_DATA_TYPE T>
	using DataTypeToStaticType_t = typename DataTypeToStaticType<T>::type;

	template<PM_DATA_TYPE T>
	constexpr size_t DataTypeToStaticType_sz = sizeof(DataTypeToStaticType_t<T>);
	template<>
	constexpr size_t DataTypeToStaticType_sz<PM_DATA_TYPE::PM_DATA_TYPE_VOID> = 0ull;

	template<PM_DATA_TYPE T>
	constexpr size_t DataTypeToStaticType_align = alignof(DataTypeToStaticType_t<T>);
	template<>
	constexpr size_t DataTypeToStaticType_align<PM_DATA_TYPE::PM_DATA_TYPE_VOID> = 1ull;

	template<template<PM_DATA_TYPE> class F, typename...P>
	auto BridgeDataType(PM_DATA_TYPE dataType, P&&...args)
	{
		switch (dataType) {
#define HANDLE_DATA_TYPE_KEY(enumFrag, keyFrag, name, shortName, description) \
		case PM_DATA_TYPE_##keyFrag: return F<PM_DATA_TYPE_##keyFrag>::Invoke(std::forward<P>(args)...);

		ENUM_KEY_LIST_DATA_TYPE(HANDLE_DATA_TYPE_KEY)

#undef HANDLE_DATA_TYPE_KEY
		}
		return F<PM_DATA_TYPE_VOID>::Default(std::forward<P>(args)...);
	}

	namespace
	{
		template<template<PM_DATA_TYPE, PM_ENUM> class F, typename...P>
		auto BridgeEnum_(PM_ENUM enumId, P&&...args)
		{
			switch (enumId) {
#define HANDLE_ENUM(enumFrag, keyFrag, name, shortName, description) case PM_ENUM_##keyFrag: return F<PM_DATA_TYPE_ENUM, PM_ENUM_##keyFrag>::Invoke(enumId, std::forward<P>(args)...);

				ENUM_KEY_LIST_ENUM(HANDLE_ENUM)

#undef HANDLE_ENUM
			}
			return F<PM_DATA_TYPE_ENUM, PM_ENUM_NULL_ENUM>::Invoke(enumId, std::forward<P>(args)...);
		}
	}

	// behavior: if datatype is not found, Default is invoked with void/null
	// if datatype is enum, we switch on enumId, id not found => invoke enum/null
	// first parameter of Invoke() must be PM_ENUM type
	template<template<PM_DATA_TYPE, PM_ENUM> class F, typename...P>
	auto BridgeDataTypeWithEnum(PM_DATA_TYPE dataType, PM_ENUM enumId, P&&...args)
	{
		switch (dataType) {
#define HANDLE_DATA_TYPE_KEY(enumFrag, keyFrag, name, shortName, description) \
		case PM_DATA_TYPE_##keyFrag: if constexpr (PM_DATA_TYPE_##keyFrag == PM_DATA_TYPE_ENUM) {\
				return BridgeEnum_<F>(enumId, std::forward<P>(args)...); \
			} \
			return F<PM_DATA_TYPE_##keyFrag, PM_ENUM_NULL_ENUM>::Invoke(enumId, std::forward<P>(args)...);

			ENUM_KEY_LIST_DATA_TYPE(HANDLE_DATA_TYPE_KEY)

#undef HANDLE_DATA_TYPE_KEY
		}
		return F<PM_DATA_TYPE_VOID, PM_ENUM_NULL_ENUM>::Default(std::forward<P>(args)...);
	}
}
