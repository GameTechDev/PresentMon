from __future__ import annotations

import json
import os
import re
import subprocess
import tempfile
import uuid
from pathlib import Path

from .common import BuildError, write_text_atomic


def _cmake_bracket(value: str) -> str:
    for equals_count in range(1, 17):
        equals = "=" * equals_count
        if f"]{equals}]" not in value:
            return f"[{equals}[{value}]{equals}]"
    raise BuildError("Unable to quote a value for generated CMake input.")


def _run_capture(arguments: list[str], cwd: Path | None = None) -> str:
    try:
        result = subprocess.run(
            arguments,
            cwd=cwd,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except FileNotFoundError as error:
        raise BuildError(f"Required command was not found: {arguments[0]}") from error
    if result.returncode:
        command = subprocess.list2cmdline(arguments)
        raise BuildError(
            f"Command failed with exit code {result.returncode}: {command}\n{result.stdout}"
        )
    return result.stdout


def generate_gtest_ctest(
    test_executable: Path,
    output_file: Path,
    test_prefix: str,
    test_filter: str,
    presentmon_path: Path,
    gold_directory: Path,
    optional_test_directory: Path | None,
    working_directory: Path,
    labels: str,
    fixtures_required: str,
    resource_lock: str,
    allow_empty: bool = False,
) -> None:
    if not test_executable.is_file():
        raise BuildError(f"Google Test executable was not built: {test_executable}")
    if not presentmon_path.is_file():
        raise BuildError(f"PresentMon executable was not built: {presentmon_path}")

    runner_arguments = [
        f"--presentmon={presentmon_path}",
        f"--golddir={gold_directory}",
    ]
    if optional_test_directory:
        runner_arguments.append(f"--opttestdir={optional_test_directory}")

    environment = os.environ.copy()
    environment.pop("PRESENTMON_ADDITIONAL_TEST_DIR", None)
    arguments = [
        str(test_executable),
        *runner_arguments,
        "--gtest_list_tests",
        f"--gtest_filter={test_filter}",
    ]
    try:
        result = subprocess.run(
            arguments,
            cwd=working_directory,
            env=environment,
            check=False,
            text=True,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
        )
    except FileNotFoundError as error:
        raise BuildError(f"Google Test executable was not found: {test_executable}") from error
    if result.returncode:
        raise BuildError(
            f"Google Test discovery failed for {test_executable}\n{result.stdout}"
        )

    test_names: list[str] = []
    current_suite = ""
    for line in result.stdout.splitlines():
        if not line:
            continue
        if not line[0].isspace():
            suite_name = line.split("#", 1)[0].strip()
            current_suite = suite_name if suite_name.endswith(".") else ""
        elif current_suite:
            test_name = line.split("#", 1)[0].strip()
            if test_name:
                test_names.append(f"{current_suite}{test_name}")
    if not test_names:
        if not allow_empty:
            raise BuildError(f"Google Test discovery found no tests matching {test_filter}.")
        # Optional data-driven group with no data present. Register nothing rather than
        # failing: this file is included on every CTest invocation, so raising here would
        # break the whole test tree, not just this group. Discovery re-runs each time, so
        # the tests appear as soon as the data directory is populated.
        write_text_atomic(
            output_file,
            "\r\n".join((
                "# Generated from Google Test discovery. Do not edit.",
                f"# No tests matched {test_filter}; the optional data directory is empty.",
            )) + "\r\n",
        )
        return
    if len(set(test_names)) != len(test_names):
        raise BuildError(f"Google Test discovery returned duplicate tests for {test_filter}.")

    ctest_labels = ";".join(value for value in labels.split(",") if value)
    lines = ["# Generated from Google Test discovery. Do not edit."]
    for test_name in test_names:
        ctest_name = f"{test_prefix}{test_name}"
        test_arguments = [f"--gtest_filter={test_name}", *runner_arguments]
        command = " ".join(_cmake_bracket(value) for value in test_arguments)
        lines.append(
            f"add_test({_cmake_bracket(ctest_name)} "
            f"{_cmake_bracket(str(test_executable))} {command})"
        )
        properties = [
            "WORKING_DIRECTORY",
            _cmake_bracket(str(working_directory)),
            "SKIP_REGULAR_EXPRESSION",
            _cmake_bracket(r"\[  SKIPPED \]"),
            "ENVIRONMENT",
            _cmake_bracket("PRESENTMON_ADDITIONAL_TEST_DIR="),
        ]
        for name, value in (
            ("LABELS", ctest_labels),
            ("FIXTURES_REQUIRED", fixtures_required),
            ("RESOURCE_LOCK", resource_lock),
        ):
            if value:
                properties.extend((name, _cmake_bracket(value)))
        if re.search(r"(^|\.)DISABLED_", test_name):
            properties.extend(("DISABLED", "TRUE"))
        lines.append(
            f"set_tests_properties({_cmake_bracket(ctest_name)} PROPERTIES "
            f"{' '.join(properties)})"
        )
    write_text_atomic(output_file, "\r\n".join(lines) + "\r\n")


def _add_discovered_tests(
    tests: dict[str, dict[str, object]],
    discovered: object,
) -> None:
    if discovered is None:
        return
    entries = discovered if isinstance(discovered, list) else [discovered]
    for entry in entries:
        if not isinstance(entry, dict):
            continue
        fully_qualified_name = str(entry.get("FullyQualifiedName", ""))
        if fully_qualified_name:
            tests[fully_qualified_name] = entry


def generate_vstest_ctest(
    vstest_console: Path,
    test_container: Path,
    output_file: Path,
    test_prefix: str,
    working_directory: Path,
    base_labels: str,
    default_labels: str,
    environment_labels: str,
    environment_name_regex: str,
    test_environment: str,
    fixtures_required: str,
    resource_lock: str,
) -> None:
    if not vstest_console.is_file():
        raise BuildError(f"vstest.console.exe was not found: {vstest_console}")
    if not test_container.is_file():
        raise BuildError(f"VSTest container was not built: {test_container}")

    diagnostic_file = Path(tempfile.gettempdir()) / f"pmon-vstest-{uuid.uuid4().hex}.log"
    try:
        _run_capture(
            [
                str(vstest_console),
                "/InIsolation",
                f"/ListTests:{test_container}",
                f"/Diag:{diagnostic_file}",
            ]
        )
        tests: dict[str, dict[str, object]] = {}
        expected_test_count: int | None = None
        marker = "Received message: "
        for line in diagnostic_file.read_text(encoding="utf-8-sig", errors="replace").splitlines():
            marker_index = line.find(marker)
            if marker_index < 0:
                continue
            try:
                message = json.loads(line[marker_index + len(marker) :])
            except json.JSONDecodeError:
                continue
            if not isinstance(message, dict):
                continue
            payload = message.get("Payload")
            if message.get("MessageType") == "TestDiscovery.TestFound":
                _add_discovered_tests(tests, payload)
            elif message.get("MessageType") == "TestDiscovery.Completed":
                if not isinstance(payload, dict):
                    raise BuildError("VSTest completion payload is malformed.")
                expected_test_count = int(payload.get("TotalTests", -1))
                _add_discovered_tests(tests, payload.get("LastDiscoveredTests"))
        if expected_test_count is None:
            raise BuildError(
                "VSTest diagnostic output did not contain a discovery completion message."
            )
        if len(tests) != expected_test_count:
            raise BuildError(
                f"VSTest reported {expected_test_count} tests but only {len(tests)} "
                "fully qualified names were captured."
            )

        split_labels = lambda value: [item for item in value.split(",") if item]
        base_label_list = split_labels(base_labels)
        default_label_list = split_labels(default_labels)
        environment_label_list = split_labels(environment_labels)
        name_pattern = re.compile(environment_name_regex) if environment_name_regex else None
        lines = ["# Generated from VSTest discovery. Do not edit."]
        for fully_qualified_name in sorted(tests):
            test_case = tests[fully_qualified_name]
            ctest_name = f"{test_prefix}{fully_qualified_name.replace('::', '.')}"
            filter_argument = f"/TestCaseFilter:FullyQualifiedName={fully_qualified_name}"
            display_name = str(test_case.get("DisplayName", ""))
            selected_labels = (
                environment_label_list
                if name_pattern and name_pattern.search(display_name)
                else default_label_list
            )
            labels = list(dict.fromkeys([*base_label_list, *selected_labels]))
            lines.append(
                f"add_test({_cmake_bracket(ctest_name)} "
                f"{_cmake_bracket(str(vstest_console))} /InIsolation "
                f"{_cmake_bracket(str(test_container))} "
                f"{_cmake_bracket(filter_argument)})"
            )
            properties = [
                "WORKING_DIRECTORY",
                _cmake_bracket(str(working_directory)),
            ]
            for name, value in (
                ("LABELS", ";".join(labels)),
                ("ENVIRONMENT", test_environment),
                ("FIXTURES_REQUIRED", fixtures_required),
                ("RESOURCE_LOCK", resource_lock),
            ):
                if value:
                    properties.extend((name, _cmake_bracket(value)))
            lines.append(
                f"set_tests_properties({_cmake_bracket(ctest_name)} PROPERTIES "
                f"{' '.join(properties)})"
            )
        write_text_atomic(output_file, "\r\n".join(lines) + "\r\n")
    finally:
        diagnostic_file.unlink(missing_ok=True)
