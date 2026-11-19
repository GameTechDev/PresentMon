// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "../CubeScene.h"
#include <cmath>
#include <gtest/gtest.h>

TEST(CubeScene, MeshIndexBoundsAndNormals)
{
	const auto& verts = pb2::CubeVertices();
	const auto& indices = pb2::CubeIndices();
	ASSERT_EQ(verts.size(), pb2::kCubeVertexCount);
	ASSERT_EQ(indices.size(), pb2::kCubeIndexCount);
	for (const uint16_t idx : indices) {
		EXPECT_LT(idx, pb2::kCubeVertexCount);
	}
	for (const auto& v : verts) {
		const float len = std::sqrt(
			v.normal[0] * v.normal[0] +
			v.normal[1] * v.normal[1] +
			v.normal[2] * v.normal[2]);
		EXPECT_NEAR(len, 1.f, 1e-5f);
	}
}

TEST(CubeScene, FrameConstantsAreFinite)
{
	const auto c = pb2::BuildFrameConstants(1.25f, 1280, 720, false);
	for (int i = 0; i < 16; ++i) {
		EXPECT_TRUE(std::isfinite(c.worldViewProj[i]));
		EXPECT_TRUE(std::isfinite(c.world[i]));
	}
	EXPECT_GT(c.ambientColor[3], 0.f);
	EXPECT_GT(c.light0Color[3], 0.f);
	EXPECT_GT(c.light1Color[3], 0.f);
}

TEST(CubeScene, AspectAffectsProjection)
{
	const auto wide = pb2::BuildFrameConstants(0.f, 1920, 1080, false);
	const auto tall = pb2::BuildFrameConstants(0.f, 720, 1280, false);
	EXPECT_NE(wide.worldViewProj[0], tall.worldViewProj[0]);
}

TEST(CubeScene, MultiAxisRotationChangesPose)
{
	const auto a = pb2::BuildFrameConstants(0.0f, 1280, 720, false);
	const auto b = pb2::BuildFrameConstants(0.7f, 1280, 720, false);
	const auto c = pb2::BuildFrameConstants(1.4f, 1280, 720, false);
	EXPECT_NE(a.world[0], b.world[0]);
	EXPECT_NE(b.world[5], c.world[5]);
	EXPECT_NE(a.world[10], c.world[10]);
}

TEST(CubeScene, VulkanYFlipDiffers)
{
	const auto d3d = pb2::BuildFrameConstants(0.f, 1280, 720, false);
	const auto vk = pb2::BuildFrameConstants(0.f, 1280, 720, true);
	EXPECT_NE(d3d.worldViewProj[5], vk.worldViewProj[5]);
}

TEST(CubeScene, CameraMatricesArePopulated)
{
	const auto cam = pb2::BuildCamera(1280, 720, false);
	EXPECT_NE(cam.view.m[0], 0.f);
	EXPECT_NE(cam.projection.m[0], 0.f);
}
