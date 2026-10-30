// Copyright (C) 2026 Intel Corporation
// SPDX-License-Identifier: MIT
#pragma once

namespace pb2::shaders
{
#define PB2_HLSL_SHADE_POINT R"(
float3 ShadePoint(float3 worldPos, float3 N, float3 lightPos, float4 lightColor)
{
	float3 L = lightPos - worldPos;
	float dist = length(L);
	L = L / max(dist, 1e-4);
	float ndotl = saturate(dot(N, L));
	float atten = 1.0 / (1.0 + 0.08 * dist + 0.02 * dist * dist);
	return lightColor.rgb * lightColor.a * ndotl * atten;
}
)"

	// Unified HLSL for D3D11/D3D12 (vs_5_0 / ps_5_0). Compile with MOTION_VECTORS=1
	// for the DX12 dual-RT path that writes NDC motion vectors.
	inline constexpr const char* kHlslSm5 = R"_pb2_hlsl(
cbuffer FrameConstants : register(b0)
{
	float4x4 worldViewProj;
	float4x4 world;
	float4 cameraPos;
	float4 materialColor;
	float4 ambientColor;
	float4 light0Pos;
	float4 light0Color;
	float4 light1Pos;
	float4 light1Color;
#if MOTION_VECTORS
	float4x4 previousWorldViewProj;
#endif
};

struct VSIn
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VSOut
{
	float4 position : SV_POSITION;
	float3 worldPos : TEXCOORD0;
	float3 worldNormal : TEXCOORD1;
#if MOTION_VECTORS
	float4 currentClip : TEXCOORD2;
	float4 previousClip : TEXCOORD3;
#endif
};

VSOut VSMain(VSIn input)
{
	VSOut o;
	float4 pos = float4(input.position, 1.0);
#if MOTION_VECTORS
	o.currentClip = mul(worldViewProj, pos);
	o.previousClip = mul(previousWorldViewProj, pos);
	o.position = o.currentClip;
#else
	o.position = mul(worldViewProj, pos);
#endif
	o.worldPos = mul(world, pos).xyz;
	o.worldNormal = normalize(mul((float3x3)world, input.normal));
	return o;
}
)_pb2_hlsl" PB2_HLSL_SHADE_POINT R"_pb2_hlsl(
#if MOTION_VECTORS
struct PSOut
{
	float4 color : SV_TARGET0;
	float2 motion : SV_TARGET1;
};

PSOut PSMain(VSOut input)
{
	PSOut o;
	float3 N = normalize(input.worldNormal);
	float3 lit = ambientColor.rgb * ambientColor.a;
	lit += ShadePoint(input.worldPos, N, light0Pos.xyz, light0Color);
	lit += ShadePoint(input.worldPos, N, light1Pos.xyz, light1Color);
	o.color = float4(materialColor.rgb * lit, materialColor.a);
	float2 currentNdc = input.currentClip.xy / input.currentClip.w;
	float2 previousNdc = input.previousClip.xy / input.previousClip.w;
	o.motion = previousNdc - currentNdc;
	return o;
}
#else
float4 PSMain(VSOut input) : SV_TARGET
{
	float3 N = normalize(input.worldNormal);
	float3 ambient = ambientColor.rgb * ambientColor.a;
	float3 lit = ambient;
	lit += ShadePoint(input.worldPos, N, light0Pos.xyz, light0Color);
	lit += ShadePoint(input.worldPos, N, light1Pos.xyz, light1Color);
	return float4(materialColor.rgb * lit, materialColor.a);
}
#endif
)_pb2_hlsl";

	// SM3 path for D3D9: pack constants into float4 registers.
	inline constexpr const char* kHlslSm3Vs = R"(
float4x4 worldViewProj : register(c0);
float4x4 world : register(c4);

struct VSIn
{
	float3 position : POSITION;
	float3 normal : NORMAL;
};

struct VSOut
{
	float4 position : POSITION;
	float3 worldPos : TEXCOORD0;
	float3 worldNormal : TEXCOORD1;
};

VSOut main(VSIn input)
{
	VSOut o;
	float4 pos = float4(input.position, 1.0);
	o.position = mul(worldViewProj, pos);
	o.worldPos = mul(world, pos).xyz;
	o.worldNormal = normalize(mul((float3x3)world, input.normal));
	return o;
}
)";

	inline constexpr const char* kHlslSm3Ps = R"_pb2_hlsl(
float4 materialColor : register(c0);
float4 ambientColor : register(c1);
float4 light0Pos : register(c2);
float4 light0Color : register(c3);
float4 light1Pos : register(c4);
float4 light1Color : register(c5);

struct PSIn
{
	float3 worldPos : TEXCOORD0;
	float3 worldNormal : TEXCOORD1;
};
)_pb2_hlsl" PB2_HLSL_SHADE_POINT R"_pb2_hlsl(
float4 main(PSIn input) : COLOR0
{
	float3 N = normalize(input.worldNormal);
	float3 ambient = ambientColor.rgb * ambientColor.a;
	float3 lit = ambient;
	lit += ShadePoint(input.worldPos, N, light0Pos.xyz, light0Color);
	lit += ShadePoint(input.worldPos, N, light1Pos.xyz, light1Color);
	return float4(materialColor.rgb * lit, materialColor.a);
}
)_pb2_hlsl";

	// GLSL sources for the Vulkan path. Checked-in SPIR-V lives in
	// CubeShadersSpirv.h; regenerate with:
	//   py -3 Scripts/generate_presentbench2_spirv.py
	inline constexpr const char* kGlslVert = R"(#version 450
layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(set = 0, binding = 0, std140) uniform FrameConstants {
	mat4 worldViewProj;
	mat4 world;
	vec4 cameraPos;
	vec4 materialColor;
	vec4 ambientColor;
	vec4 light0Pos;
	vec4 light0Color;
	vec4 light1Pos;
	vec4 light1Color;
} ubo;

layout(location = 0) out vec3 vWorldPos;
layout(location = 1) out vec3 vWorldNormal;

void main() {
	vec4 pos = vec4(inPosition, 1.0);
	gl_Position = ubo.worldViewProj * pos;
	vWorldPos = (ubo.world * pos).xyz;
	vWorldNormal = normalize(mat3(ubo.world) * inNormal);
}
)";

	inline constexpr const char* kGlslFrag = R"(#version 450
layout(location = 0) in vec3 vWorldPos;
layout(location = 1) in vec3 vWorldNormal;
layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0, std140) uniform FrameConstants {
	mat4 worldViewProj;
	mat4 world;
	vec4 cameraPos;
	vec4 materialColor;
	vec4 ambientColor;
	vec4 light0Pos;
	vec4 light0Color;
	vec4 light1Pos;
	vec4 light1Color;
} ubo;

vec3 ShadePoint(vec3 worldPos, vec3 N, vec3 lightPos, vec4 lightColor) {
	vec3 L = lightPos - worldPos;
	float dist = length(L);
	L = L / max(dist, 1e-4);
	float ndotl = max(dot(N, L), 0.0);
	float atten = 1.0 / (1.0 + 0.08 * dist + 0.02 * dist * dist);
	return lightColor.rgb * lightColor.a * ndotl * atten;
}

void main() {
	vec3 N = normalize(vWorldNormal);
	vec3 ambient = ubo.ambientColor.rgb * ubo.ambientColor.a;
	vec3 lit = ambient;
	lit += ShadePoint(vWorldPos, N, ubo.light0Pos.xyz, ubo.light0Color);
	lit += ShadePoint(vWorldPos, N, ubo.light1Pos.xyz, ubo.light1Color);
	outColor = vec4(ubo.materialColor.rgb * lit, ubo.materialColor.a);
}
)";

#undef PB2_HLSL_SHADE_POINT
}
