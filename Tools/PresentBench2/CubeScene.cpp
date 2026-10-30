// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#include "CubeScene.h"
#include <cmath>

namespace pb2
{
	namespace
	{
		constexpr float kPi = 3.14159265358979323846f;

		void SetColumn(Mat4& m, int col, float x, float y, float z, float w)
		{
			m.m[col * 4 + 0] = x;
			m.m[col * 4 + 1] = y;
			m.m[col * 4 + 2] = z;
			m.m[col * 4 + 3] = w;
		}

		void Normalize3(float& x, float& y, float& z)
		{
			const float len = std::sqrt(x * x + y * y + z * z);
			if (len > 1e-8f) {
				x /= len;
				y /= len;
				z /= len;
			}
		}

		void Cross3(float ax, float ay, float az, float bx, float by, float bz,
			float& ox, float& oy, float& oz)
		{
			ox = ay * bz - az * by;
			oy = az * bx - ax * bz;
			oz = ax * by - ay * bx;
		}
	}

	const std::array<CubeVertex, kCubeVertexCount>& CubeVertices()
	{
		static const std::array<CubeVertex, kCubeVertexCount> kVerts{ {
			// +Z
			{ { -1.f, -1.f,  1.f }, { 0.f, 0.f, 1.f } },
			{ {  1.f, -1.f,  1.f }, { 0.f, 0.f, 1.f } },
			{ {  1.f,  1.f,  1.f }, { 0.f, 0.f, 1.f } },
			{ { -1.f,  1.f,  1.f }, { 0.f, 0.f, 1.f } },
			// -Z
			{ {  1.f, -1.f, -1.f }, { 0.f, 0.f, -1.f } },
			{ { -1.f, -1.f, -1.f }, { 0.f, 0.f, -1.f } },
			{ { -1.f,  1.f, -1.f }, { 0.f, 0.f, -1.f } },
			{ {  1.f,  1.f, -1.f }, { 0.f, 0.f, -1.f } },
			// +X
			{ {  1.f, -1.f,  1.f }, { 1.f, 0.f, 0.f } },
			{ {  1.f, -1.f, -1.f }, { 1.f, 0.f, 0.f } },
			{ {  1.f,  1.f, -1.f }, { 1.f, 0.f, 0.f } },
			{ {  1.f,  1.f,  1.f }, { 1.f, 0.f, 0.f } },
			// -X
			{ { -1.f, -1.f, -1.f }, { -1.f, 0.f, 0.f } },
			{ { -1.f, -1.f,  1.f }, { -1.f, 0.f, 0.f } },
			{ { -1.f,  1.f,  1.f }, { -1.f, 0.f, 0.f } },
			{ { -1.f,  1.f, -1.f }, { -1.f, 0.f, 0.f } },
			// +Y
			{ { -1.f,  1.f,  1.f }, { 0.f, 1.f, 0.f } },
			{ {  1.f,  1.f,  1.f }, { 0.f, 1.f, 0.f } },
			{ {  1.f,  1.f, -1.f }, { 0.f, 1.f, 0.f } },
			{ { -1.f,  1.f, -1.f }, { 0.f, 1.f, 0.f } },
			// -Y
			{ { -1.f, -1.f, -1.f }, { 0.f, -1.f, 0.f } },
			{ {  1.f, -1.f, -1.f }, { 0.f, -1.f, 0.f } },
			{ {  1.f, -1.f,  1.f }, { 0.f, -1.f, 0.f } },
			{ { -1.f, -1.f,  1.f }, { 0.f, -1.f, 0.f } },
		} };
		return kVerts;
	}

	const std::array<uint16_t, kCubeIndexCount>& CubeIndices()
	{
		// Clockwise winding for D3D default front faces (FrontCounterClockwise = false).
		static const std::array<uint16_t, kCubeIndexCount> kIdx{ {
			0, 1, 2, 0, 2, 3,
			4, 5, 6, 4, 6, 7,
			8, 9, 10, 8, 10, 11,
			12, 13, 14, 12, 14, 15,
			16, 17, 18, 16, 18, 19,
			20, 21, 22, 20, 22, 23,
		} };
		return kIdx;
	}

	Mat4 MatIdentity()
	{
		Mat4 m{};
		m.m[0] = m.m[5] = m.m[10] = m.m[15] = 1.f;
		return m;
	}

	Mat4 MatMultiply(const Mat4& a, const Mat4& b)
	{
		Mat4 out{};
		for (int col = 0; col < 4; ++col) {
			for (int row = 0; row < 4; ++row) {
				out.m[col * 4 + row] =
					a.m[0 * 4 + row] * b.m[col * 4 + 0] +
					a.m[1 * 4 + row] * b.m[col * 4 + 1] +
					a.m[2 * 4 + row] * b.m[col * 4 + 2] +
					a.m[3 * 4 + row] * b.m[col * 4 + 3];
			}
		}
		return out;
	}

	Mat4 MatScale(float s)
	{
		Mat4 m = MatIdentity();
		m.m[0] = m.m[5] = m.m[10] = s;
		return m;
	}

	Mat4 MatRotateX(float radians)
	{
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		Mat4 m = MatIdentity();
		m.m[5] = c;
		m.m[6] = s;
		m.m[9] = -s;
		m.m[10] = c;
		return m;
	}

	Mat4 MatRotateY(float radians)
	{
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		Mat4 m = MatIdentity();
		m.m[0] = c;
		m.m[2] = -s;
		m.m[8] = s;
		m.m[10] = c;
		return m;
	}

	Mat4 MatRotateZ(float radians)
	{
		const float c = std::cos(radians);
		const float s = std::sin(radians);
		Mat4 m = MatIdentity();
		m.m[0] = c;
		m.m[1] = s;
		m.m[4] = -s;
		m.m[5] = c;
		return m;
	}

	Mat4 MatLookAtLh(float eyeX, float eyeY, float eyeZ,
		float atX, float atY, float atZ,
		float upX, float upY, float upZ)
	{
		float zx = atX - eyeX;
		float zy = atY - eyeY;
		float zz = atZ - eyeZ;
		Normalize3(zx, zy, zz);

		float xx, xy, xz;
		Cross3(upX, upY, upZ, zx, zy, zz, xx, xy, xz);
		Normalize3(xx, xy, xz);

		float yx, yy, yz;
		Cross3(zx, zy, zz, xx, xy, xz, yx, yy, yz);

		Mat4 m{};
		SetColumn(m, 0, xx, yx, zx, 0.f);
		SetColumn(m, 1, xy, yy, zy, 0.f);
		SetColumn(m, 2, xz, yz, zz, 0.f);
		SetColumn(m, 3,
			-(xx * eyeX + xy * eyeY + xz * eyeZ),
			-(yx * eyeX + yy * eyeY + yz * eyeZ),
			-(zx * eyeX + zy * eyeY + zz * eyeZ),
			1.f);
		return m;
	}

	Mat4 MatPerspectiveFovLh(float fovYRadians, float aspect, float nearZ, float farZ)
	{
		const float yScale = 1.f / std::tan(fovYRadians * 0.5f);
		const float xScale = yScale / aspect;
		Mat4 m{};
		m.m[0] = xScale;
		m.m[5] = yScale;
		m.m[10] = farZ / (farZ - nearZ);
		m.m[11] = 1.f;
		m.m[14] = (-nearZ * farZ) / (farZ - nearZ);
		return m;
	}

	Mat4 MatPerspectiveFovLhVulkan(float fovYRadians, float aspect, float nearZ, float farZ)
	{
		Mat4 m = MatPerspectiveFovLh(fovYRadians, aspect, nearZ, farZ);
		m.m[5] = -m.m[5];
		return m;
	}

	void MatTranspose(const Mat4& in, float out[16])
	{
		for (int row = 0; row < 4; ++row) {
			for (int col = 0; col < 4; ++col) {
				out[row * 4 + col] = in.m[col * 4 + row];
			}
		}
	}

	CameraMatrices BuildCamera(uint32_t width, uint32_t height, bool flipYForVulkan)
	{
		CameraMatrices cam{};
		const float aspect = (height > 0) ? ((float)width / (float)height) : 1.f;
		cam.view = MatLookAtLh(cam.eyeX, cam.eyeY, cam.eyeZ, 0.f, 0.f, 0.f, 0.f, 1.f, 0.f);
		const float fov = 50.f * (kPi / 180.f);
		cam.projection = flipYForVulkan
			? MatPerspectiveFovLhVulkan(fov, aspect, 0.1f, 100.f)
			: MatPerspectiveFovLh(fov, aspect, 0.1f, 100.f);
		return cam;
	}

	FrameConstants BuildFrameConstants(float sceneTimeSec, uint32_t width, uint32_t height,
		bool flipYForVulkan)
	{
		const Mat4 scale = MatScale(kCubeHalfExtent);
		const Mat4 rx = MatRotateX(sceneTimeSec * kRotXRadPerSec);
		const Mat4 ry = MatRotateY(sceneTimeSec * kRotYRadPerSec);
		const Mat4 rz = MatRotateZ(sceneTimeSec * kRotZRadPerSec);
		const Mat4 world = MatMultiply(MatMultiply(MatMultiply(scale, rx), ry), rz);

		const CameraMatrices cam = BuildCamera(width, height, flipYForVulkan);
		const Mat4 viewProj = MatMultiply(cam.projection, cam.view);
		const Mat4 worldViewProj = MatMultiply(viewProj, world);

		FrameConstants c{};
		for (int i = 0; i < 16; ++i) {
			c.worldViewProj[i] = worldViewProj.m[i];
			c.world[i] = world.m[i];
		}
		c.cameraPos[0] = cam.eyeX;
		c.cameraPos[1] = cam.eyeY;
		c.cameraPos[2] = cam.eyeZ;
		c.materialColor[0] = 0.86f;
		c.materialColor[1] = 0.84f;
		c.materialColor[2] = 0.80f;
		c.materialColor[3] = 1.f;
		c.ambientColor[0] = 0.55f;
		c.ambientColor[1] = 0.58f;
		c.ambientColor[2] = 0.62f;
		c.ambientColor[3] = 0.18f; // strength
		// Warm key light
		c.light0Pos[0] = 1.8f;
		c.light0Pos[1] = 1.6f;
		c.light0Pos[2] = -1.2f;
		c.light0Pos[3] = 1.f;
		c.light0Color[0] = 1.00f;
		c.light0Color[1] = 0.72f;
		c.light0Color[2] = 0.48f;
		c.light0Color[3] = 0.85f;
		// Cool fill light
		c.light1Pos[0] = -1.7f;
		c.light1Pos[1] = 0.4f;
		c.light1Pos[2] = 1.5f;
		c.light1Pos[3] = 1.f;
		c.light1Color[0] = 0.42f;
		c.light1Color[1] = 0.62f;
		c.light1Color[2] = 0.95f;
		c.light1Color[3] = 0.55f;
		return c;
	}
}
