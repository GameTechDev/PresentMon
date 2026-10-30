# Copyright (C) 2026 Intel Corporation
# SPDX-License-Identifier: MIT
from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon.common import BuildError, ensure_python_version, log
from pmon.generate_presentbench2_spirv import find_glslc, generate


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Regenerate PresentBench2 CubeShadersSpirv.h from CubeShaders.h"
    )
    parser.add_argument(
        "--glslc",
        type=Path,
        default=None,
        help="Path to glslc.exe (defaults to PATH)",
    )
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
        help="PresentMon repository root",
    )
    parser.add_argument(
        "--check",
        action="store_true",
        help="Fail if the checked-in header does not match the GLSL",
    )
    return parser.parse_args()


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        root = arguments.repo_root.resolve()
        glslc = find_glslc(arguments.glslc.resolve() if arguments.glslc else None)
        out = generate(glslc, root, check=arguments.check)
        log(f"Checked {out}" if arguments.check else f"Wrote {out}")
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
