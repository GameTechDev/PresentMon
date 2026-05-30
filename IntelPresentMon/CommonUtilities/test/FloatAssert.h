// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

#include <CppUnitTest.h>

#include <cmath>
#include <sstream>
#include <string>
#include <type_traits>

namespace pmon::util::test
{
    template<typename ExpectedT, typename ActualT, typename ToleranceT>
    void AssertAreEqualWithinTolerance(
        ExpectedT expected,
        ActualT actual,
        ToleranceT tolerance,
        const wchar_t* message = nullptr)
    {
        using ComparisonT = std::common_type_t<ExpectedT, ActualT, ToleranceT>;
        static_assert(std::is_floating_point_v<ComparisonT>);

        const auto expectedValue = static_cast<ComparisonT>(expected);
        const auto actualValue = static_cast<ComparisonT>(actual);
        const auto toleranceValue = static_cast<ComparisonT>(tolerance);

        if (std::isnan(expectedValue) || std::isnan(actualValue) || std::isnan(toleranceValue) ||
            std::abs(expectedValue - actualValue) > toleranceValue) {
            std::wstringstream ss;
            if (message != nullptr) {
                ss << message << L" ";
            }
            ss << L"Expected: <" << expectedValue
               << L"> Actual: <" << actualValue
               << L"> Tolerance: <" << toleranceValue << L">";

            Microsoft::VisualStudio::CppUnitTestFramework::Assert::Fail(ss.str().c_str());
        }
    }
}
