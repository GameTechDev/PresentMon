# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT
"""Regenerate Tools/PresentBench2/CubeShadersSpirv.h from CubeShaders.h GLSL."""

from __future__ import annotations

import re
import struct
import tempfile
from pathlib import Path
from shutil import which

from .common import BuildError, run


_GLSL_PATTERN = re.compile(
    r"inline constexpr const char\* (?P<name>kGlslVert|kGlslFrag) = R\"\((?P<body>.*?)\)\";",
    re.DOTALL,
)


def find_glslc(explicit: Path | None) -> Path:
    if explicit is not None:
        if not explicit.is_file():
            raise BuildError(f"glslc not found: {explicit}")
        return explicit
    found = which("glslc")
    if found:
        return Path(found)
    raise BuildError(
        "glslc was not found on PATH. Install the LunarG Vulkan SDK, or pass "
        "--glslc path\\to\\glslc.exe"
    )


def _extract_glsl(cube_shaders: Path) -> dict[str, str]:
    text = cube_shaders.read_text(encoding="utf-8")
    matches = {m.group("name"): m.group("body") for m in _GLSL_PATTERN.finditer(text)}
    missing = [name for name in ("kGlslVert", "kGlslFrag") if name not in matches]
    if missing:
        raise BuildError(
            f"Could not extract GLSL from {cube_shaders}: missing {', '.join(missing)}"
        )
    return matches


def _compile(glslc: Path, stage: str, source: str, out_spv: Path) -> None:
    with tempfile.TemporaryDirectory(prefix="pb2-spirv-") as temp_dir:
        src = Path(temp_dir) / f"cube.{stage}"
        src.write_text(source, encoding="ascii", newline="\n")
        run(
            [
                glslc,
                f"-fshader-stage={stage}",
                src,
                "-o",
                out_spv,
            ]
        )


def _emit_array(name: str, spv: Path) -> str:
    data = spv.read_bytes()
    if len(data) % 4 != 0:
        raise BuildError(f"SPIR-V size is not a multiple of 4: {spv}")
    words = struct.unpack("<" + ("I" * (len(data) // 4)), data)
    lines = [f"\tinline constexpr uint32_t {name}[] = {{"]
    row: list[str] = []
    for index, word in enumerate(words):
        row.append(f"0x{word:08x}u")
        if len(row) == 8 or index == len(words) - 1:
            suffix = "," if index != len(words) - 1 else ""
            lines.append("\t\t" + ", ".join(row) + suffix)
            row = []
    lines.append("\t};")
    lines.append(
        f"\tinline constexpr size_t {name}Count = sizeof({name}) / sizeof({name}[0]);"
    )
    return "\n".join(lines)


def generate(glslc: Path, repo: Path, check: bool = False) -> Path:
    cube_shaders = repo / "Tools" / "PresentBench2" / "CubeShaders.h"
    out_header = repo / "Tools" / "PresentBench2" / "CubeShadersSpirv.h"
    sources = _extract_glsl(cube_shaders)
    with tempfile.TemporaryDirectory(prefix="pb2-spirv-out-") as temp_dir:
        vert_spv = Path(temp_dir) / "cube.vert.spv"
        frag_spv = Path(temp_dir) / "cube.frag.spv"
        _compile(glslc, "vertex", sources["kGlslVert"], vert_spv)
        _compile(glslc, "fragment", sources["kGlslFrag"], frag_spv)
        body = "\n\n".join(
            [
                _emit_array("kSpirvVert", vert_spv),
                _emit_array("kSpirvFrag", frag_spv),
            ]
        )
    header = (
        "// Copyright (C) 2026 Intel Corporation\n"
        "// SPDX-License-Identifier: MIT\n"
        "// Generated SPIR-V for PresentBench2 Vulkan cube shaders.\n"
        "// Source GLSL: CubeShaders.h (kGlslVert / kGlslFrag). Regenerate with:\n"
        "//   py -3 Scripts/generate_presentbench2_spirv.py\n"
        "#pragma once\n"
        "#include <cstddef>\n"
        "#include <cstdint>\n"
        "\n"
        "namespace pb2::shaders\n"
        "{\n"
        f"{body}\n"
        "}\n"
    )
    if check:
        if not out_header.is_file():
            raise BuildError(f"Generated SPIR-V header is missing: {out_header}")
        existing = out_header.read_text(encoding="utf-8")
        if existing.replace("\r\n", "\n") != header:
            raise BuildError(
                f"{out_header} is stale; run Scripts/generate_presentbench2_spirv.py"
            )
    else:
        out_header.write_text(header, encoding="utf-8", newline="\r\n")
    return out_header
