from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon.common import BuildError, ensure_python_version
from pmon.discovery import generate_gtest_ctest, generate_vstest_ctest


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Generate CTest entries from built test binaries.")
    subparsers = parser.add_subparsers(dest="operation", required=True)

    gtest = subparsers.add_parser("gtest")
    gtest.add_argument("--test-executable", type=Path, required=True)
    gtest.add_argument("--output-file", type=Path, required=True)
    gtest.add_argument("--test-prefix", required=True)
    gtest.add_argument("--test-filter", required=True)
    gtest.add_argument("--presentmon-path", type=Path, required=True)
    gtest.add_argument("--gold-directory", type=Path, required=True)
    gtest.add_argument("--optional-test-directory", default="")
    gtest.add_argument("--working-directory", type=Path, required=True)
    gtest.add_argument("--labels", default="")
    gtest.add_argument("--fixtures-required", default="")
    gtest.add_argument("--resource-lock", default="")
    gtest.add_argument(
        "--allow-empty",
        action="store_true",
        help="Register no tests instead of failing when the group discovers none. "
             "For optional data-driven groups whose data directory may be absent.",
    )

    vstest = subparsers.add_parser("vstest")
    vstest.add_argument("--vstest-console", type=Path, required=True)
    vstest.add_argument("--test-container", type=Path, required=True)
    vstest.add_argument("--output-file", type=Path, required=True)
    vstest.add_argument("--test-prefix", required=True)
    vstest.add_argument("--working-directory", type=Path, required=True)
    vstest.add_argument("--base-labels", default="")
    vstest.add_argument("--default-labels", default="")
    vstest.add_argument("--environment-labels", default="")
    vstest.add_argument("--environment-name-regex", default="")
    vstest.add_argument("--test-environment", default="")
    vstest.add_argument("--fixtures-required", default="")
    vstest.add_argument("--resource-lock", default="")
    return parser.parse_args()


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        if arguments.operation == "gtest":
            generate_gtest_ctest(
                arguments.test_executable.resolve(),
                arguments.output_file.resolve(),
                arguments.test_prefix,
                arguments.test_filter,
                arguments.presentmon_path.resolve(),
                arguments.gold_directory.resolve(),
                Path(arguments.optional_test_directory).resolve()
                if arguments.optional_test_directory
                else None,
                arguments.working_directory.resolve(),
                arguments.labels,
                arguments.fixtures_required,
                arguments.resource_lock,
                arguments.allow_empty,
            )
        else:
            generate_vstest_ctest(
                arguments.vstest_console.resolve(),
                arguments.test_container.resolve(),
                arguments.output_file.resolve(),
                arguments.test_prefix,
                arguments.working_directory.resolve(),
                arguments.base_labels,
                arguments.default_labels,
                arguments.environment_labels,
                arguments.environment_name_regex,
                arguments.test_environment,
                arguments.fixtures_required,
                arguments.resource_lock,
            )
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
