from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon import web
from pmon.common import BuildError, ensure_python_version


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Prepare or build PresentMon web assets.")
    parser.add_argument("operation", choices=("ensure", "build"))
    parser.add_argument(
        "--repo-root",
        type=Path,
        default=Path(__file__).resolve().parents[1],
    )
    parser.add_argument("--dependency-root", type=Path)
    return parser.parse_args()


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        repo_root = arguments.repo_root.resolve()
        dependency_root = (
            arguments.dependency_root.resolve()
            if arguments.dependency_root
            else repo_root / "build" / "ThirdParty"
        )
        if arguments.operation == "ensure":
            web.ensure(repo_root, dependency_root)
        else:
            web.build(repo_root, dependency_root)
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
