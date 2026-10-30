// Copyright (C) 2022 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <CommonUtilities/win/WinAPI.h>
#include <include/cef_v8.h>
#include <type_traits>
#include <string>
#include <algorithm>
#include <iterator>
#include "Logging.h"
#include <CommonUtilities/str/String.h>
#include <CommonUtilities/Meta.h>
#include <CommonUtilities/Exception.h>
#include <string_view>
#include <CommonUtilities/ref/WrapReflect.h>
#include <ranges>
#include <array>
#include <cmath>

namespace p2c::client::util
{
	using namespace ::pmon::util;
	namespace refl = reflect;
	namespace vi = std::views;

	PM_DEFINE_EX(BadV8Traversal);

	inline CefString ToCefString(std::string_view sv)
	{
		return CefString(sv.data(), sv.size());
	}

	// specialize this to enable custom conversion behavior for specific types
	template<class T>
	struct CustomV8Conversion {};

	template<class T>
	concept HasCustomFromV8 = requires(CefV8Value& v8, T& out) {
		CustomV8Conversion<T>::FromV8(v8, out);
	};

	template<class T>
	void FromV8(CefV8Value& v8, T& out)
	{
		if constexpr (HasCustomFromV8<T>) {
			CustomV8Conversion<T>::FromV8(v8, out);
		}
		else if constexpr (IsContainer<std::optional, T>) {
			// Missing object properties are undefined in V8; treat them like null so
			// field-patch payloads do not invent empty engaged optionals.
			if (v8.IsNull() || v8.IsUndefined()) {
				out.reset();
			}
			else {
				out.emplace();
				FromV8(v8, *out);
			}
		}
		else if constexpr (std::same_as<std::string, T>) {
			out = v8.GetStringValue();
		}
		else if constexpr (std::same_as<bool, T>) {
			out = v8.GetBoolValue();
		}
		else if constexpr (IsContainer<std::vector, T>) {
			out.clear();
			for (int i = 0; i < v8.GetArrayLength(); i++) {
				out.emplace_back();
				FromV8(*v8.GetValue(i), out.back());
			}
		}
		else if constexpr (IsStdArray<T>) {
			for (int i = 0; i < v8.GetArrayLength(); i++) {
				FromV8(*v8.GetValue(i), out[i]);
			}
		}
		else if constexpr (std::is_floating_point_v<T>) {
			out = T(v8.GetDoubleValue());
		}
		else if constexpr (std::is_integral_v<T> && sizeof(T) > 4) {
			// 64-bit values travel as JS numbers (doubles), exact up to 2^53
			// TODO: Use a lossless encoding before values can exceed 2^53; the current
			// verbose-module mask uses only 15 bits, so precision loss is latent.
			// non-finite or out-of-range values become 0 instead of undefined conversion
			const auto value = v8.GetDoubleValue();
			out = std::isfinite(value) && std::abs(value) < 9.2e18 ? T(int64_t(value)) : T(0);
		}
		else if constexpr (std::is_integral_v<T>) {
			if (v8.IsUInt()) {
				out = T(v8.GetUIntValue());
			}
			else {
				out = T(v8.GetIntValue());
			}
		}
		else if constexpr (std::is_enum_v<T>) {
			int keyVal = 0;
			FromV8(v8, keyVal);
			out = T(keyVal);
		}
		else if constexpr (std::is_class_v<T>) {
			refl::for_each([&](const auto I) {
				FromV8(*v8.GetValue(ToCefString(refl::member_name<I>(out))), refl::get<I>(out));
			}, out);
		}
	}

	template<class T>
	T FromV8(CefV8Value& v8)
	{
		T out;
		FromV8(v8, out);
		return out;
	}

	template<class T>
	void ToV8(const T& in, CefRefPtr<CefV8Value>& pV8)
	{
		if constexpr (IsContainer<std::optional, T>) {
			if (!in) {
				pV8 = CefV8Value::CreateNull();
			}
			else {
				ToV8(*in, pV8);
			}
		}
		else if constexpr (std::same_as<std::string, T>) {
			pV8 = CefV8Value::CreateString(in);
		}
		else if constexpr (std::same_as<bool, T>) {
			pV8 = CefV8Value::CreateBool(in);
		}
		else if constexpr (IsContainer<std::vector, T>) {
			pV8 = CefV8Value::CreateArray((int)in.size());
			for (auto&&[i, el] : in | vi::enumerate) {
				CefRefPtr<CefV8Value> pNewV8;
				ToV8(el, pNewV8);
				pV8->SetValue((int)i, std::move(pNewV8));
			}
		}
		else if constexpr (IsStdArray<T>) {
			pV8 = CefV8Value::CreateArray((int)in.size());
			for (auto&& [i, el] : in | vi::enumerate) {
				CefRefPtr<CefV8Value> pNewV8;
				ToV8(el, pNewV8);
				pV8->SetValue((int)i, std::move(pNewV8));
			}
		}
		else if constexpr (std::is_floating_point_v<T>) {
			pV8 = CefV8Value::CreateDouble(in);
		}
		else if constexpr (std::is_integral_v<T> && sizeof(T) > 4) {
			pV8 = CefV8Value::CreateDouble((double)in);
		}
		else if constexpr (std::is_integral_v<T>) {
			pV8 = CefV8Value::CreateInt(in);
		}
		else if constexpr (std::is_enum_v<T>) {
			pV8 = CefV8Value::CreateInt(int(in));
		}
		else if constexpr (std::is_class_v<T>) {
			pV8 = CefV8Value::CreateObject(nullptr, nullptr);
			refl::for_each([&](const auto I) {
				CefRefPtr<CefV8Value> pNewV8;
				ToV8(refl::get<I>(in), pNewV8);
				pV8->SetValue(ToCefString(refl::member_name<I>(in)), std::move(pNewV8),
					CefV8Value::PropertyAttribute::V8_PROPERTY_ATTRIBUTE_NONE);
			}, in);
		}
	}

	template<class T>
	CefRefPtr<CefV8Value> ToV8(const T& in)
	{
		CefRefPtr<CefV8Value> pV8;
		ToV8(in, pV8);
		return pV8;
	}
}