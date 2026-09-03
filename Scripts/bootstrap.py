from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon import auxdata, cef, web
from pmon.common import BuildError, ensure_python_version, log


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Prepare PresentMon build dependencies.")
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--dependency-root", type=Path, required=True)
    parser.add_argument("--architecture", choices=("x64", "Win32"), required=True)
    parser.add_argument("--with-cef", action="store_true")
    parser.add_argument("--with-auxdata", action="store_true")
    parser.add_argument("--with-web", action="store_true")
    parser.add_argument("--generator")
    parser.add_argument("--platform")
    parser.add_argument("--toolset")
    parser.add_argument("--generator-instance")
    parser.add_argument("--cef-source")
    arguments = parser.parse_args()
    # Only the CEF stage compiles wrapper libraries, so only it needs the full
    # Visual Studio selection. The other dependencies are toolset-independent.
    if arguments.with_cef:
        missing = [
            "--" + name.replace("_", "-")
            for name in ("generator", "platform", "toolset", "generator_instance")
            if not getattr(arguments, name)
        ]
        if missing:
            parser.error(f"--with-cef requires {', '.join(missing)}.")
    return arguments


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        repo_root = arguments.repo_root.resolve()
        dependency_root = arguments.dependency_root.resolve()
        if not (repo_root / "CMakeLists.txt").is_file():
            raise BuildError(f"Repository root does not contain CMakeLists.txt: {repo_root}")
        dependency_root.mkdir(parents=True, exist_ok=True)

        log(f"Python: {sys.executable}")
        log(f"Shared dependency root: {dependency_root}")
        log(f"Target architecture: {arguments.architecture}")
        if arguments.with_cef:
            cef.ensure(
                repo_root,
                dependency_root,
                arguments.generator,
                arguments.platform,
                arguments.toolset,
                arguments.generator_instance,
                arguments.cef_source,
            )
        if arguments.with_auxdata:
            auxdata.ensure(repo_root, dependency_root)
        if arguments.with_web:
            web.ensure(repo_root, dependency_root)
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
