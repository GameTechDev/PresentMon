from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon import cef
from pmon.common import BuildError, ensure_python_version


# Each operation names the selections it needs. Compiling the wrapper libraries
# needs the full Visual Studio selection; validating a stage needs only the
# toolset that qualifies it; validating the published runtime payload needs
# neither, because that payload is toolset-independent.
REQUIRED_ARGUMENTS = {
    "ensure": ("generator", "platform", "toolset", "generator_instance"),
    "restore": ("generator", "platform", "toolset", "generator_instance"),
    "verify-stage": ("toolset",),
    "verify-output": ("output_root",),
}


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Restore or validate PresentMon CEF inputs.")
    parser.add_argument("operation", choices=tuple(REQUIRED_ARGUMENTS))
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--dependency-root", type=Path, required=True)
    parser.add_argument("--output-root", type=Path)
    parser.add_argument("--generator")
    parser.add_argument("--platform")
    parser.add_argument("--toolset")
    parser.add_argument("--generator-instance")
    parser.add_argument("--source")
    arguments = parser.parse_args()
    missing = [
        "--" + name.replace("_", "-")
        for name in REQUIRED_ARGUMENTS[arguments.operation]
        if not getattr(arguments, name)
    ]
    if missing:
        parser.error(f"{arguments.operation} requires {', '.join(missing)}.")
    return arguments


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        repo_root = arguments.repo_root.resolve()
        dependency_root = arguments.dependency_root.resolve()
        if arguments.operation == "ensure":
            cef.ensure(
                repo_root,
                dependency_root,
                arguments.generator,
                arguments.platform,
                arguments.toolset,
                arguments.generator_instance,
                arguments.source,
            )
        elif arguments.operation == "restore":
            cef.restore(
                repo_root,
                dependency_root,
                arguments.generator,
                arguments.platform,
                arguments.toolset,
                arguments.generator_instance,
                arguments.source,
            )
        elif arguments.operation == "verify-stage":
            errors = cef.validate_stage(repo_root, dependency_root, arguments.toolset)
            if errors:
                raise BuildError("CEF stage validation failed:\n" + "\n".join(errors))
            print("CEF stage matches the lock and contains all build inputs.")
        else:
            errors = cef.validate_output(repo_root, arguments.output_root.resolve())
            if errors:
                raise BuildError("CEF output validation failed:\n" + "\n".join(errors))
            print("CEF output payload matches the lock.")
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
