#pragma once
#include <cmath>
#include <limits>
#include <algorithm>

namespace pmon::util
{
	template<typename T>
	T CommonEpsilonStrict(T a, T b)
	{
		return std::numeric_limits<T>::epsilon() * std::max(std::abs(a), std::abs(b));
	}

	template<typename T>
	T EpsilonEqual(T a, T b)
	{
		return std::abs(a - b) <= CommonEpsilonStrict(a, b);
	}

	template<typename T>
	T EpsilonLessThan(T a, T b)
	{
		return (a - b) < CommonEpsilon(a, b);
	}
	enum class MagnitudePrefix
	{
		Base,
		Kilo,
		Kibi,
		Mega,
		Mebi,
		Giga,
		Gibi,
	};
	constexpr double GetMagnitudeFactor(MagnitudePrefix prefix)
	{
		switch (prefix) {
		case MagnitudePrefix::Base: return 1.;
		case MagnitudePrefix::Kilo: return 1'000.;
		case MagnitudePrefix::Kibi: return 1'024.;
		case MagnitudePrefix::Mega: return 1'000'000.;
		case MagnitudePrefix::Mebi: return 1'048'576.;
		case MagnitudePrefix::Giga: return 1'000'000'000.;
		case MagnitudePrefix::Gibi: return 1'073'741'824.;
		default: return 0.;
		}
	}
	template<typename From, typename To = From>
	To ConvertMagnitudePrefix(From from, MagnitudePrefix fromPrefix, MagnitudePrefix toPrefix)
	{
		auto fromExtended = double(from);
		const auto srcFactor = GetMagnitudeFactor(fromPrefix);
		const auto dstFactor = GetMagnitudeFactor(toPrefix);
		const auto conversionFactor = srcFactor / dstFactor;
		return To(fromExtended * conversionFactor);
	}
	template<typename T>
	T CalculateEma(T prevEma, T newSample, T alpha)
	{
		return prevEma + alpha * (newSample - prevEma);
	}
}