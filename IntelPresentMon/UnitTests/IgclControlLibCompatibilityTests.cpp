// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <ControlLib/igcl/IgclControlLibCompatibility.h>
#include <array>
#include <span>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace IgclControlLibCompatibilityTests
{
    using pmon::tel::igcl::ControlLibFileVersion;
    using pmon::tel::igcl::IsControlLibVersionMismatch;
    using pmon::tel::igcl::IsPreHotfixControlLibVersion;
    using pmon::tel::igcl::IsZesInitOnlyControlLibVersion;

    TEST_CLASS(ControlLibVersionPredicateTests)
    {
    public:
        TEST_METHOD(PreHotfixBelowOneTwoTwoSixNine)
        {
            Assert::IsTrue(IsPreHotfixControlLibVersion({ 1, 2, 268, 999 }));
            Assert::IsFalse(IsPreHotfixControlLibVersion({ 1, 2, 269, 269 }));
            Assert::IsFalse(IsPreHotfixControlLibVersion({ 1, 2, 270, 0 }));
        }

        TEST_METHOD(ZesInitOnlyAtOneThree)
        {
            Assert::IsTrue(IsZesInitOnlyControlLibVersion({ 1, 3, 0, 0 }));
            Assert::IsFalse(IsZesInitOnlyControlLibVersion({ 1, 2, 999, 999 }));
        }

        TEST_METHOD(MismatchRequiresBothBuckets)
        {
            const ControlLibFileVersion preHotfix{ 1, 2, 100, 0 };
            const ControlLibFileVersion zesInit{ 1, 3, 0, 0 };
            const ControlLibFileVersion hotfixNeutral{ 1, 2, 269, 269 };

            Assert::IsTrue(IsControlLibVersionMismatch(std::array{ preHotfix, zesInit }));
            Assert::IsFalse(IsControlLibVersionMismatch(std::array{ preHotfix, hotfixNeutral }));
            Assert::IsFalse(IsControlLibVersionMismatch(std::array{ zesInit, hotfixNeutral }));
            Assert::IsFalse(IsControlLibVersionMismatch(std::span<const ControlLibFileVersion>{}));
        }
    };
}
