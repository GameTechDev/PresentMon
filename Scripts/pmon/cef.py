from __future__ import annotations

import json
import os
import re
import shutil
import tempfile
import urllib.parse
from pathlib import Path

from .common import (
    BuildError,
    download,
    file_lock,
    log,
    read_json,
    replace_directory,
    require_relative_path,
    run,
    sha256_file,
)


BUILD_INPUTS = (
    "Include/include/base/cef_callback.h",
    "Include/include/cef_app.h",
    "Include/include/cef_client.h",
    "Include/include/cef_parser.h",
    "Include/include/cef_scheme.h",
    "Include/include/cef_task.h",
    "Include/include/cef_v8.h",
    "Include/include/cef_version.h",
    "Include/include/internal/cef_types.h",
    "Include/include/wrapper/cef_closure_task.h",
    "Include/include/wrapper/cef_helpers.h",
    "Lib/Debug/libcef.lib",
    "Lib/Debug/libcef_dll_wrapper.lib",
    "Lib/Release/libcef.lib",
    "Lib/Release/libcef_dll_wrapper.lib",
)

VERSION_NAMES = (
    "CEF_VERSION",
    "CEF_VERSION_MAJOR",
    "CEF_VERSION_MINOR",
    "CEF_VERSION_PATCH",
    "CEF_COMMIT_NUMBER",
    "CEF_COMMIT_HASH",
    "CHROME_VERSION_MAJOR",
    "CHROME_VERSION_MINOR",
    "CHROME_VERSION_BUILD",
    "CHROME_VERSION_PATCH",
)


def _lock_path(repo_root: Path) -> Path:
    return repo_root / "IntelPresentMon" / "AppCef" / "cef-lock.json"


def _load_lock(repo_root: Path) -> dict[str, object]:
    path = _lock_path(repo_root)
    value = read_json(path)
    if not isinstance(value, dict):
        raise BuildError(f"CEF lock is not an object: {path}")
    if value.get("schemaVersion") != 1:
        raise BuildError(f"Unsupported CEF lock schema in {path}")
    payload = value.get("payload")
    if not isinstance(payload, list) or not payload:
        raise BuildError(f"CEF lock payload is empty: {path}")
    return value


# Compiled CEF stages are qualified by platform toolset family so that a v143
# and a v145 build tree never share wrapper libraries. The toolset becomes a
# single path component, and an empty or compound value would silently resolve
# to the shared parent, so the family form is required here.
def _require_toolset(toolset: str) -> str:
    value = (toolset or "").strip()
    if not re.fullmatch(r"v[0-9]+", value):
        raise BuildError(
            "CEF operations require a platform toolset family such as v143 or "
            f"v145; received: {toolset!r}"
        )
    return value


def _stage_root(dependency_root: Path, toolset: str) -> Path:
    return dependency_root / "cef" / _require_toolset(toolset)


def _version_metadata(stage_root: Path) -> dict[str, object]:
    version_path = stage_root / "Include" / "include" / "cef_version.h"
    if not version_path.is_file():
        version_path = stage_root / "include" / "cef_version.h"
    if not version_path.is_file():
        raise BuildError(f"CEF version header was not found beneath {stage_root}")
    text = version_path.read_text(encoding="utf-8", errors="replace")
    metadata: dict[str, object] = {}
    for name in VERSION_NAMES:
        match = re.search(rf"^#define\s+{re.escape(name)}\s+(.+?)\s*$", text, re.MULTILINE)
        if not match:
            continue
        value = match.group(1).strip().strip('"')
        try:
            metadata[name] = int(value)
        except ValueError:
            metadata[name] = value
    if "CEF_VERSION" not in metadata:
        raise BuildError(f"CEF_VERSION was not found in {version_path}")
    return metadata


def _validate_build_inputs(stage_root: Path) -> list[str]:
    errors: list[str] = []
    for relative in BUILD_INPUTS:
        path = stage_root / Path(relative)
        if not path.is_file():
            errors.append(f"Missing CEF build input: {relative}")
        elif path.stat().st_size == 0:
            errors.append(f"Empty CEF build input: {relative}")
    return errors


def validate_stage(repo_root: Path, dependency_root: Path, toolset: str) -> list[str]:
    lock = _load_lock(repo_root)
    stage_root = _stage_root(dependency_root, toolset)
    if not stage_root.is_dir():
        return [f"CEF stage directory is missing: {stage_root}"]

    errors: list[str] = []
    try:
        actual_metadata = _version_metadata(stage_root)
    except BuildError as error:
        errors.append(str(error))
        actual_metadata = {}
    expected_metadata = lock.get("cef")
    if not isinstance(expected_metadata, dict):
        errors.append("CEF lock metadata is missing.")
    else:
        for name, expected in expected_metadata.items():
            actual = actual_metadata.get(name)
            if str(actual) != str(expected):
                errors.append(
                    f"CEF metadata mismatch for {name}; expected {expected}, found {actual}."
                )

    expected_paths: set[str] = set()
    payload = lock["payload"]
    assert isinstance(payload, list)
    for raw_entry in payload:
        if not isinstance(raw_entry, dict):
            errors.append("CEF lock contains a malformed payload entry.")
            continue
        relative_value = str(raw_entry.get("stagePath", ""))
        try:
            relative = require_relative_path(relative_value, "CEF stage path")
        except BuildError as error:
            errors.append(str(error))
            continue
        expected_paths.add(relative.as_posix().lower())
        path = stage_root / relative
        if not path.is_file():
            errors.append(f"Missing CEF file: {relative.as_posix()}")
            continue
        expected_size = int(raw_entry.get("size", -1))
        if path.stat().st_size != expected_size:
            errors.append(
                f"Size mismatch for {relative.as_posix()}; expected {expected_size}, "
                f"found {path.stat().st_size}."
            )
        expected_hash = str(raw_entry.get("sha256", "")).lower()
        actual_hash = sha256_file(path)
        if actual_hash != expected_hash:
            errors.append(
                f"Hash mismatch for {relative.as_posix()}; expected {expected_hash}, "
                f"found {actual_hash}."
            )

    for group in ("Bin", "Resources"):
        root = stage_root / group
        if not root.is_dir():
            errors.append(f"CEF staged {group} directory is missing: {root}")
            continue
        for path in root.rglob("*"):
            if path.is_file():
                relative = path.relative_to(stage_root).as_posix().lower()
                if relative not in expected_paths:
                    errors.append(f"Unexpected CEF file: {path.relative_to(stage_root).as_posix()}")
    errors.extend(_validate_build_inputs(stage_root))
    return errors


def validate_output(repo_root: Path, output_root: Path) -> list[str]:
    lock = _load_lock(repo_root)
    if not output_root.is_dir():
        return [f"CEF output directory is missing: {output_root}"]
    errors: list[str] = []
    payload = lock["payload"]
    assert isinstance(payload, list)
    for raw_entry in payload:
        if not isinstance(raw_entry, dict):
            errors.append("CEF lock contains a malformed payload entry.")
            continue
        relative_value = str(raw_entry.get("path", ""))
        try:
            relative = require_relative_path(relative_value, "CEF output path")
        except BuildError as error:
            errors.append(str(error))
            continue
        path = output_root / relative
        if not path.is_file():
            errors.append(f"Missing CEF file: {relative.as_posix()}")
            continue
        expected_size = int(raw_entry.get("size", -1))
        if path.stat().st_size != expected_size:
            errors.append(f"Size mismatch for {relative.as_posix()}")
        expected_hash = str(raw_entry.get("sha256", "")).lower()
        if sha256_file(path) != expected_hash:
            errors.append(f"Hash mismatch for {relative.as_posix()}")
    return errors


def _work_parent() -> Path:
    configured = os.environ.get("PRESENTMON_CEF_WORK_ROOT")
    if configured:
        root = Path(configured)
    else:
        system_drive = os.environ.get("SystemDrive")
        root = Path(f"{system_drive}\\pcef") if system_drive else Path(tempfile.gettempdir()) / "pcef"
    root.mkdir(parents=True, exist_ok=True)
    return root


def _cached_archive(repo_root: Path, dependency_root: Path, source: str | None) -> Path:
    lock = _load_lock(repo_root)
    source_lock = lock.get("source")
    if not isinstance(source_lock, dict):
        raise BuildError("CEF lock source is missing.")
    locked_uri = str(source_lock.get("uri", ""))
    expected_hash = str(source_lock.get("sha256", "")).lower()
    expected_size = int(source_lock.get("size", -1))
    file_name = str(source_lock.get("fileName", "")).strip()
    if not expected_hash or not file_name:
        raise BuildError("CEF lock must contain source fileName and sha256 values.")

    if source:
        parsed = urllib.parse.urlparse(source)
        if parsed.scheme in ("http", "https"):
            if source != locked_uri:
                raise BuildError(f"CEF accepts only the URI recorded in the lock: {locked_uri}")
            uri = source
            local_source = None
        else:
            local_source = Path(source).resolve()
            if not local_source.is_file():
                raise BuildError(f"CEF source archive was not found: {local_source}")
            uri = ""
    else:
        local_source = None
        uri = locked_uri
    if not local_source and not uri:
        raise BuildError("CEF lock does not define a source URI.")

    archive = dependency_root / "downloads" / "cef" / file_name
    archive.parent.mkdir(parents=True, exist_ok=True)
    if local_source:
        actual_hash = sha256_file(local_source)
        if actual_hash != expected_hash:
            raise BuildError(
                f"CEF archive hash does not match lock; expected {expected_hash}, found {actual_hash}."
            )
        if local_source != archive:
            temporary = archive.with_name(f"{archive.name}.incoming-{os.getpid()}")
            shutil.copy2(local_source, temporary)
            temporary.replace(archive)
        return archive

    if archive.is_file():
        if archive.stat().st_size == expected_size and sha256_file(archive) == expected_hash:
            return archive
        archive.unlink()
    temporary = archive.with_name(f"{archive.name}.incoming-{os.getpid()}")
    log(f"Downloading locked CEF archive from {uri}")
    download(uri, temporary)
    actual_hash = sha256_file(temporary)
    if actual_hash != expected_hash:
        temporary.unlink(missing_ok=True)
        raise BuildError(
            f"CEF archive hash does not match lock; expected {expected_hash}, found {actual_hash}."
        )
    if temporary.stat().st_size != expected_size:
        temporary.unlink(missing_ok=True)
        raise BuildError("CEF archive size does not match the lock.")
    temporary.replace(archive)
    return archive


def _extract_distribution(archive: Path, work_root: Path) -> Path:
    extraction_root = work_root / "extract"
    extraction_root.mkdir()
    log(f"Extracting CEF archive into {extraction_root}")
    run(["cmake", "-E", "tar", "xjf", archive], cwd=extraction_root)
    candidates = [extraction_root, *[path for path in extraction_root.iterdir() if path.is_dir()]]
    for candidate in candidates:
        if all(
            path.exists()
            for path in (
                candidate / "CMakeLists.txt",
                candidate / "Release",
                candidate / "Resources",
                candidate / "include",
            )
        ):
            return candidate
    raise BuildError(f"Could not locate a CEF distribution root beneath {extraction_root}")


def _build_wrapper(
    distribution_root: Path,
    generator: str,
    platform: str,
    toolset: str,
    generator_instance: str,
) -> None:
    # The wrapper libraries must be produced by the same Visual Studio
    # installation and toolset as the parent build, so none of these selections
    # may be inferred here.
    for description, value in (
        ("generator", generator),
        ("platform", platform),
        ("toolset", toolset),
        ("generator instance", generator_instance),
    ):
        if not value:
            raise BuildError(f"Building the CEF wrapper requires an explicit {description}.")
    build_root = distribution_root / "build"
    if build_root.exists():
        shutil.rmtree(build_root)
    arguments = [
        "cmake",
        "-S",
        distribution_root,
        "-B",
        build_root,
        "-G",
        generator,
        "-A",
        platform,
        "-T",
        toolset,
        f"-DCMAKE_GENERATOR_INSTANCE={generator_instance}",
        "-DUSE_SANDBOX=OFF",
    ]
    environment = os.environ.copy()
    environment.setdefault("CL_MPCount", "4")
    log("Configuring the CEF wrapper.")
    run(arguments, env=environment)
    for configuration in ("Release", "Debug"):
        log(f"Building the CEF wrapper for {configuration}.")
        run(
            ["cmake", "--build", build_root, "--config", configuration],
            env=environment,
        )
        output = build_root / "libcef_dll_wrapper" / configuration / "libcef_dll_wrapper.lib"
        if not output.is_file():
            raise BuildError(f"CEF wrapper output was not produced: {output}")


def _copy_directory(source: Path, destination: Path, excluded_names: set[str] | None = None) -> None:
    if not source.is_dir():
        raise BuildError(f"CEF source directory was not found: {source}")
    destination.mkdir(parents=True, exist_ok=True)
    excluded = {name.lower() for name in (excluded_names or set())}
    for path in source.iterdir():
        if path.is_file() and path.name.lower() in excluded:
            continue
        target = destination / path.name
        if path.is_dir():
            shutil.copytree(path, target, dirs_exist_ok=True)
        else:
            shutil.copy2(path, target)


def _create_stage(distribution_root: Path, destination: Path) -> None:
    for relative in ("Bin", "Lib/Debug", "Lib/Release", "Include", "Resources"):
        (destination / relative).mkdir(parents=True, exist_ok=True)
    _copy_directory(
        distribution_root / "Release",
        destination / "Bin",
        {"cef_sandbox.lib", "libcef.lib"},
    )
    _copy_directory(distribution_root / "include", destination / "Include" / "include")
    _copy_directory(distribution_root / "Resources", destination / "Resources")
    shutil.copy2(
        distribution_root / "Release" / "libcef.lib",
        destination / "Lib" / "Debug" / "libcef.lib",
    )
    shutil.copy2(
        distribution_root / "Release" / "libcef.lib",
        destination / "Lib" / "Release" / "libcef.lib",
    )
    _copy_directory(
        distribution_root / "build" / "libcef_dll_wrapper" / "Debug",
        destination / "Lib" / "Debug",
    )
    _copy_directory(
        distribution_root / "build" / "libcef_dll_wrapper" / "Release",
        destination / "Lib" / "Release",
    )
    locales = destination / "Resources" / "locales"
    if locales.is_dir():
        for path in locales.iterdir():
            if path.is_file() and path.name.lower() != "en-us.pak":
                path.unlink()


def restore(
    repo_root: Path,
    dependency_root: Path,
    generator: str,
    platform: str,
    toolset: str,
    generator_instance: str,
    source: str | None = None,
    skip_if_valid: bool = False,
) -> None:
    toolset = _require_toolset(toolset)
    # The archive cache and this lock stay shared across toolsets, so a v143 and
    # a v145 restore serialize instead of publishing concurrently.
    lock_file = dependency_root / "locks" / "cef.lock"
    with file_lock(lock_file):
        if skip_if_valid:
            errors = validate_stage(repo_root, dependency_root, toolset)
            if not errors:
                log("Locked CEF stage is present and valid.")
                return
            log("CEF stage is missing, stale, or invalid; restoring it.")
        archive = _cached_archive(repo_root, dependency_root, source)
        work_root = Path(tempfile.mkdtemp(prefix="restore-", dir=_work_parent()))
        keep_work = os.environ.get("PRESENTMON_CEF_KEEP_WORK", "").lower() in (
            "1",
            "true",
            "yes",
        )
        success = False
        try:
            distribution_root = _extract_distribution(archive, work_root)
            _build_wrapper(distribution_root, generator, platform, toolset, generator_instance)
            staged = work_root / "stage"
            staged.mkdir()
            _create_stage(distribution_root, staged)
            publish_root = dependency_root / f".cef-publish-{os.getpid()}"
            if publish_root.exists():
                shutil.rmtree(publish_root)
            try:
                # Publication replaces only the selected toolset stage. The
                # shared cef/ parent and every other stage beneath it, including
                # an older unqualified one, are never renamed or removed.
                shutil.copytree(staged, publish_root / "cef" / toolset)
                errors = validate_stage(repo_root, publish_root, toolset)
                if errors:
                    raise BuildError("Restored CEF stage did not validate:\n" + "\n".join(errors))
                replace_directory(
                    publish_root / "cef" / toolset,
                    _stage_root(dependency_root, toolset),
                )
            finally:
                shutil.rmtree(publish_root, ignore_errors=True)
            success = True
            log(f"Published locked CEF stage at {_stage_root(dependency_root, toolset)}")
        finally:
            if success and not keep_work:
                shutil.rmtree(work_root, ignore_errors=True)
            elif not success:
                log(f"CEF restore work directory retained after failure: {work_root}")


def ensure(
    repo_root: Path,
    dependency_root: Path,
    generator: str,
    platform: str,
    toolset: str,
    generator_instance: str,
    source: str | None = None,
    force: bool = False,
) -> None:
    restore(
        repo_root,
        dependency_root,
        generator,
        platform,
        toolset,
        generator_instance,
        source,
        skip_if_valid=not force,
    )
