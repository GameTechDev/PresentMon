// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once
#include <array>
#include <cstdint>
#include <cstddef>

namespace pb2
{
	struct CubeVertex
	{
		float position[3];
		float normal[3];
	};

	// GPU-aligned frame constants shared by D3D11/12 (cbuffer b0) and Vulkan (std140 UBO).
	struct alignas(16) FrameConstants
	{
		float worldViewProj[16];
		float world[16];
		float cameraPos[3];
		float _pad0;
		float materialColor[4];
		float ambientColor[4]; // rgb + strength in w
		float light0Pos[4];    // xyz, w unused
		float light0Color[4];  // rgb + intensity in w
		float light1Pos[4];
		float light1Color[4];
	};
	static_assert(sizeof(FrameConstants) == 240, "FrameConstants size must match shader packing");
	static_assert(alignof(FrameConstants) == 16, "FrameConstants must be 16-byte aligned");

	struct Mat4
	{
		float m[16]{}; // column-major
	};

	constexpr uint32_t kCubeVertexCount = 24;
	constexpr uint32_t kCubeIndexCount = 36;
	constexpr float kCubeHalfExtent = 0.55f;

	const std::array<CubeVertex, kCubeVertexCount>& CubeVertices();
	const std::array<uint16_t, kCubeIndexCount>& CubeIndices();

	Mat4 MatIdentity();
	Mat4 MatMultiply(const Mat4& a, const Mat4& b);
	Mat4 MatScale(float s);
	Mat4 MatRotateX(float radians);
	Mat4 MatRotateY(float radians);
	Mat4 MatRotateZ(float radians);
	Mat4 MatLookAtLh(float eyeX, float eyeY, float eyeZ,
		float atX, float atY, float atZ,
		float upX, float upY, float upZ);
	Mat4 MatPerspectiveFovLh(float fovYRadians, float aspect, float nearZ, float farZ);
	// Same as MatPerspectiveFovLh but with clip Y flipped for Vulkan NDC.
	Mat4 MatPerspectiveFovLhVulkan(float fovYRadians, float aspect, float nearZ, float farZ);

	void MatTranspose(const Mat4& in, float out[16]);

	struct CameraMatrices
	{
		Mat4 view;
		Mat4 projection;
		float eyeX = 0.f;
		float eyeY = 0.35f;
		float eyeZ = -2.55f;
	};

	// Shared camera for all backends and frame-generation matrix tagging.
	CameraMatrices BuildCamera(uint32_t width, uint32_t height, bool flipYForVulkan = false);

	// Build CPU-side constants for a lit rotating cube. flipYForVulkan selects Vulkan clip Y.
	FrameConstants BuildFrameConstants(float sceneTimeSec, uint32_t width, uint32_t height,
		bool flipYForVulkan = false);

	// Scene tuning constants (also useful for tests/docs).
	constexpr float kRotXRadPerSec = 0.55f;
	constexpr float kRotYRadPerSec = 0.95f;
	constexpr float kRotZRadPerSec = 0.35f;
	constexpr float kClearR = 0.045f;
	constexpr float kClearG = 0.050f;
	constexpr float kClearB = 0.065f;
}
