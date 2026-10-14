// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include <CppUnitTest.h>
#include <ControlLib/TelemetryDeviceFingerprint.h>
#include <PresentMonAPI2/PresentMonAPI.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;
using pmon::tel::TelemetryDeviceFingerprint;
using pmon::tel::TelemetryDeviceFingerprintMatches;
using pmon::tel::MergeTelemetryDeviceFingerprint;

namespace TelemetryDeviceFingerprintTests
{
    namespace
    {
        TelemetryDeviceFingerprint MakeGpu_(PM_DEVICE_VENDOR vendor, const char* name)
        {
            TelemetryDeviceFingerprint fp{};
            fp.deviceType = PM_DEVICE_TYPE_GRAPHICS_ADAPTER;
            fp.vendor = vendor;
            fp.deviceName = name;
            return fp;
        }
    }

    TEST_CLASS(FingerprintMatchTests)
    {
    public:
        TEST_METHOD(IdenticalNvidiaCardsOnDifferentBusesDoNotMatch)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "NVIDIA GeForce RTX 4090");
            a.pciDeviceId = 0x2684;
            a.pciSubSystemId = 0x16F310DE;
            a.pciBusId = 1;
            auto b = a;
            b.pciBusId = 2;

            Assert::IsFalse(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(NvapiAndNvmlSameCardMatchByBus)
        {
            auto nvapi = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "NVIDIA GeForce RTX 4090");
            nvapi.pciDeviceId = 0x2684;
            nvapi.pciSubSystemId = 0x16F310DE;
            nvapi.pciBusId = 2;
            auto nvml = nvapi;
            nvml.deviceName = "NVIDIA GeForce RTX 4090 ";

            Assert::IsTrue(TelemetryDeviceFingerprintMatches(nvapi, nvml));
        }

        TEST_METHOD(IdenticalAmdCardsOnDifferentBusesDoNotMatchByName)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_AMD, "AMD Radeon RX 7900 XTX");
            a.pciBusId = 3;
            auto b = a;
            b.pciBusId = 4;

            Assert::IsFalse(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(DifferingLuidsDoNotPreventBusMatch)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "NVIDIA GeForce RTX 4090");
            a.pciBusId = 2;
            a.luid = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            auto b = a;
            b.luid = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            Assert::IsTrue(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(SameLuidDoesNotOverrideBusMismatch)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "NVIDIA GeForce RTX 4090");
            a.pciBusId = 1;
            a.luid = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            auto b = a;
            b.pciBusId = 2;

            Assert::IsFalse(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(MergeKeepsFirstLuid)
        {
            auto dst = MakeGpu_(PM_DEVICE_VENDOR_INTEL, "Intel(R) Arc(TM) B580 Graphics");
            dst.luid = { 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
            auto src = dst;
            src.luid = { 0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

            MergeTelemetryDeviceFingerprint(dst, src);
            Assert::IsTrue(dst.luid == std::vector<uint8_t>{ 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 });

            auto empty = MakeGpu_(PM_DEVICE_VENDOR_INTEL, "Intel(R) Arc(TM) B580 Graphics");
            MergeTelemetryDeviceFingerprint(empty, src);
            Assert::IsTrue(empty.luid == src.luid);
        }

        TEST_METHOD(PciIdsMatchWhenBusUnavailable)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "NVIDIA GeForce RTX 4090");
            a.pciDeviceId = 0x2684;
            a.pciSubSystemId = 0x16F310DE;
            auto b = a;
            b.pciBusId = 2;

            Assert::IsTrue(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(NameFallbackWhenNoIdentifiers)
        {
            const auto a = MakeGpu_(PM_DEVICE_VENDOR_AMD, "AMD Radeon RX 7900 XTX");
            const auto b = MakeGpu_(PM_DEVICE_VENDOR_AMD, "amd radeon rx-7900 xtx");

            Assert::IsTrue(TelemetryDeviceFingerprintMatches(a, b));
        }

        TEST_METHOD(DifferentVendorsNeverMatch)
        {
            auto a = MakeGpu_(PM_DEVICE_VENDOR_NVIDIA, "GPU");
            a.pciBusId = 1;
            auto b = MakeGpu_(PM_DEVICE_VENDOR_AMD, "GPU");
            b.pciBusId = 1;

            Assert::IsFalse(TelemetryDeviceFingerprintMatches(a, b));
        }
    };
}
