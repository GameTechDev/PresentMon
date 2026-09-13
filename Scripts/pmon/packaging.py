from __future__ import annotations

import hashlib
import os
import re
import shutil
import subprocess
import tempfile
import uuid
import xml.etree.ElementTree as ElementTree
from pathlib import Path

from .cef import validate_output, validate_stage
from .common import BuildError, read_json, run, sha256_file, write_text_atomic


WIX_NAMESPACE = "http://schemas.microsoft.com/wix/2006/wi"
REPARSE_POINT = 0x0400


def _is_true(value: str | bool) -> bool:
    if isinstance(value, bool):
        return value
    return value.strip().lower() in ("true", "1", "on", "yes")


def _split_paths(value: str) -> list[Path]:
    return [Path(path).resolve() for path in value.split("|") if path]


def _is_reparse_point(path: Path) -> bool:
    stat = path.lstat()
    return path.is_symlink() or bool(getattr(stat, "st_file_attributes", 0) & REPARSE_POINT)


def _clear_directory(path: Path) -> None:
    resolved = path.resolve()
    if resolved == Path(resolved.anchor) or len(resolved.parts) < 3:
        raise BuildError(f"Refusing to clear unsafe staging directory: {resolved}")
    resolved.mkdir(parents=True, exist_ok=True)
    for child in resolved.iterdir():
        if child.is_symlink() or child.is_file():
            child.unlink()
        else:
            shutil.rmtree(child)


def stage_uci_runtime(
    enabled: str | bool,
    sdk_root: Path | None,
    stage_dir: Path,
    manifest_path: Path,
) -> None:
    _clear_directory(stage_dir)
    if not _is_true(enabled):
        write_text_atomic(manifest_path, "")
        return
    if sdk_root is None or not sdk_root.is_dir():
        raise BuildError(f"UCI is enabled but its SDK root is missing: {sdk_root}")
    sdk_root = sdk_root.resolve()
    runtime_dll = sdk_root / "unified-collector-interface.dll"
    collectors = sdk_root / "collectors"
    if not runtime_dll.is_file():
        raise BuildError(f"Enabled UCI runtime is missing required file: {runtime_dll}")
    if not collectors.is_dir():
        raise BuildError(f"Enabled UCI runtime is missing required directory: {collectors}")
    collector_files = [path for path in collectors.rglob("*") if path.is_file()]
    if not collector_files:
        raise BuildError(f"Enabled UCI runtime requires a file under: {collectors}")

    relative_files: list[str] = []
    for root_name, directory_names, file_names in os.walk(sdk_root, followlinks=False):
        root = Path(root_name)
        for directory_name in directory_names:
            directory = root / directory_name
            if _is_reparse_point(directory):
                raise BuildError(f"Reparse point is not allowed in the UCI SDK: {directory}")
        for file_name in file_names:
            source = root / file_name
            if _is_reparse_point(source):
                raise BuildError(f"Reparse point is not allowed in the UCI SDK: {source}")
            relative = source.relative_to(sdk_root)
            destination = stage_dir / relative
            destination.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(source, destination)
            relative_files.append(relative.as_posix())
    if not relative_files:
        raise BuildError(f"Enabled UCI SDK contains no files: {sdk_root}")
    write_text_atomic(manifest_path, "".join(f"{value}\r\n" for value in sorted(relative_files)))


def _empty_uci_fragment(output_path: Path) -> None:
    content = (
        '<?xml version="1.0" encoding="utf-8"?>\r\n'
        f'<Wix xmlns="{WIX_NAMESPACE}">\r\n'
        "    <Fragment>\r\n"
        '        <ComponentGroup Id="uci_dist_files" />\r\n'
        "    </Fragment>\r\n"
        "</Wix>\r\n"
    )
    write_text_atomic(output_path, content)


def _deterministic_guid(name: str) -> str:
    value = bytearray(hashlib.md5(name.lower().encode("utf-8")).digest())
    value[6] = (value[6] & 0x0F) | 0x30
    value[8] = (value[8] & 0x3F) | 0x80
    return str(uuid.UUID(bytes_le=bytes(value))).upper()


def _deterministic_wix_id(name: str, prefix: str) -> str:
    digest = hashlib.md5(name.lower().encode("utf-8")).hexdigest()
    return f"{prefix}{digest[:8]}"


def _resolve_heat(wix_root: Path) -> Path:
    heat = wix_root / "bin" / "heat.exe"
    if not heat.is_file():
        raise BuildError(f"WiX heat.exe was not found: {heat}")
    return heat


def _write_xml(document: ElementTree.ElementTree, output_path: Path) -> None:
    ElementTree.register_namespace("", WIX_NAMESPACE)
    ElementTree.indent(document, space="    ")
    temporary = output_path.with_name(f"{output_path.name}.incoming-{os.getpid()}")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    try:
        document.write(temporary, encoding="utf-8", xml_declaration=True)
        content = temporary.read_text(encoding="utf-8").replace("\n", "\r\n") + "\r\n"
        write_text_atomic(output_path, content)
    finally:
        temporary.unlink(missing_ok=True)


def generate_uci_fragment(
    enabled: str | bool,
    stage_dir: Path,
    output_path: Path,
    wix_root: Path,
) -> None:
    if not _is_true(enabled):
        _empty_uci_fragment(output_path)
        return
    stage_dir = stage_dir.resolve()
    if not (stage_dir / "unified-collector-interface.dll").is_file():
        raise BuildError("Enabled UCI stage is missing unified-collector-interface.dll.")
    collectors = stage_dir / "collectors"
    if not collectors.is_dir() or not any(path.is_file() for path in collectors.rglob("*")):
        raise BuildError("Enabled UCI stage collectors directory is missing or empty.")
    output_path.parent.mkdir(parents=True, exist_ok=True)
    run(
        [
            _resolve_heat(wix_root),
            "dir",
            stage_dir,
            "-srd",
            "-sreg",
            "-scom",
            "-dr",
            "service_folder",
            "-cg",
            "uci_dist_files",
            "-var",
            "var.UciDistDir",
            "-suid",
            "-g1",
            "-ag",
            "-out",
            output_path,
        ]
    )

    document = ElementTree.parse(output_path)
    namespace = {"w": WIX_NAMESPACE}
    stage_prefix = f"{stage_dir}{os.sep}".lower()
    for file_node in document.findall(".//w:File", namespace):
        source = file_node.get("Source", "")
        if not source or ".." in Path(source.replace("\\", "/")).parts:
            raise BuildError(f"Unsafe harvested UCI source: {source}")
        normalized = source.replace("/", "\\")
        variable = "$(var.UciDistDir)"
        if normalized.lower().startswith(variable.lower()):
            relative = normalized[len(variable) :].lstrip("\\")
            resolved = (stage_dir / relative).resolve()
        elif re.match(r"^[A-Za-z]:\\", normalized):
            resolved = Path(normalized).resolve()
        else:
            resolved = (stage_dir / normalized.lstrip("\\")).resolve()
        if not f"{resolved}".lower().startswith(stage_prefix) or not resolved.is_file():
            raise BuildError(f"Harvested source is outside or missing from the UCI stage: {source}")
        if _is_reparse_point(resolved):
            raise BuildError(f"Harvested source is a reparse point: {resolved}")

    for component in document.findall(".//w:Component", namespace):
        file_node = component.find("w:File", namespace)
        if file_node is None:
            raise BuildError("Harvested UCI component does not contain a File element.")
        source = file_node.get("Source", "")
        component.set("Win64", "yes")
        component.set("Guid", f"{{{_deterministic_guid(source)}}}")
        component.set("Id", _deterministic_wix_id(f"cmp:{source}", "c"))
        file_node.set("Id", _deterministic_wix_id(f"file:{source}", "f"))
    _write_xml(document, output_path)


def _native_snapshot_line(path: Path) -> str:
    if not path.is_file():
        raise BuildError(f"Native packaging input is missing: {path}")
    stat = path.stat()
    windows_ticks = stat.st_mtime_ns // 100 + 621355968000000000
    return f"{path}|{stat.st_size}|{windows_ticks}|{sha256_file(path).upper()}"


def native_snapshot(mode: str, snapshot_path: Path, paths_list: str) -> None:
    paths = _split_paths(paths_list)
    if not paths:
        raise BuildError("Native paths list must contain at least one path.")
    current = [_native_snapshot_line(path) for path in paths]
    if mode == "record":
        write_text_atomic(snapshot_path, "".join(f"{line}\r\n" for line in current))
        return
    if not snapshot_path.is_file():
        raise BuildError(f"Native payload snapshot is missing: {snapshot_path}")
    expected = snapshot_path.read_text(encoding="ascii").splitlines()
    if current != expected:
        mismatches = [str(path) for path, old, new in zip(paths, expected, current) if old != new]
        raise BuildError(f"Native payload changed during the WiX build: {', '.join(mismatches)}")


def verify_authenticode(signtool: Path, path: Path) -> None:
    if not signtool.is_file():
        raise BuildError("Production packaging requires SignTool for Authenticode verification.")
    if not path.is_file():
        raise BuildError(f"Packaged PE input is missing: {path}")
    run([signtool, "verify", "/pa", path], capture_output=True)


def verify_production_inputs(
    signtool: Path,
    native_paths_list: str,
    uci_enabled: str | bool,
    uci_stage_dir: Path | None,
    uci_manifest_path: Path | None,
    skip_uci: bool,
) -> None:
    for path in _split_paths(native_paths_list):
        print(f"Verifying Authenticode packaging input: {path}")
        verify_authenticode(signtool, path)
    if _is_true(uci_enabled) and not skip_uci:
        if uci_stage_dir is None or uci_manifest_path is None or not uci_manifest_path.is_file():
            raise BuildError("UCI production verification requires its stage and manifest.")
        pe_count = 0
        for relative in uci_manifest_path.read_text(encoding="utf-8-sig").splitlines():
            if not relative.strip():
                continue
            path = uci_stage_dir / relative
            if not path.is_file():
                raise BuildError(f"UCI staged file is missing: {path}")
            if path.suffix.lower() in (".exe", ".dll"):
                pe_count += 1
        print(f"Verified UCI staged runtime manifest ({pe_count} PE files).")


def verify_link_preflight(
    repo_root: Path,
    dependency_root: Path,
    toolset: str,
    payload_root: Path,
    deployment_profile: str,
    signtool: Path,
    native_paths_list: str,
    uci_enabled: str | bool,
    uci_stage_dir: Path | None,
    uci_manifest_path: Path | None,
    skip_production_pe: bool,
) -> None:
    stage_errors = validate_stage(repo_root, dependency_root, toolset)
    if stage_errors:
        raise BuildError("CEF shared stage validation failed:\n" + "\n".join(stage_errors))
    cef_errors = validate_output(repo_root, payload_root)
    if cef_errors:
        raise BuildError("CEF packaging input validation failed:\n" + "\n".join(cef_errors))
    verify_cef_installer_fragments(repo_root)
    if deployment_profile.upper() == "PRODUCTION" and not skip_production_pe:
        verify_production_inputs(
            signtool,
            native_paths_list,
            uci_enabled,
            uci_stage_dir,
            uci_manifest_path,
            False,
        )


def _stable_cef_wix_id(prefix: str, value: str) -> str:
    safe = re.sub(r"[^A-Za-z0-9_.]", "_", value)
    if len(safe) > 45:
        safe = safe[-45:]
    digest = hashlib.sha256(value.encode("utf-8")).hexdigest()[:8]
    return f"{prefix}_{safe}_{digest}"


def _cef_fragment_entries(repo_root: Path, group: str) -> list[str]:
    lock = read_json(repo_root / "IntelPresentMon" / "AppCef" / "cef-lock.json")
    if not isinstance(lock, dict) or not isinstance(lock.get("payload"), list):
        raise BuildError("CEF lock payload is missing or malformed.")
    entries: list[str] = []
    for entry in lock["payload"]:
        if isinstance(entry, dict) and entry.get("group") == group:
            relative = str(entry.get("path", "")).replace("\\", "/")
            if not relative or relative.startswith("/") or ".." in Path(relative).parts:
                raise BuildError(f"Unsafe CEF installer path in the lock: {relative}")
            entries.append(relative)
    if not entries:
        raise BuildError(f"CEF lock contains no {group} installer entries.")
    return sorted(entries)


def _verify_cef_installer_fragment(
    path: Path,
    component_group: str,
    entries: list[str],
) -> None:
    if not path.is_file():
        raise BuildError(f"Installer CEF fragment is missing: {path}")
    document = ElementTree.parse(path)
    namespace = {"w": WIX_NAMESPACE}
    directory_refs = document.findall(".//w:DirectoryRef", namespace)
    if len(directory_refs) != 1 or directory_refs[0].get("Id") != "pm_app_folder":
        raise BuildError(f"Installer CEF fragment has an invalid DirectoryRef: {path}")

    expected_directories = sorted(
        {
            relative.rsplit("/", 1)[0]
            for relative in entries
            if "/" in relative
        }
    )
    if any("/" in value for value in expected_directories):
        raise BuildError("Nested CEF installer directories deeper than one level are unsupported.")
    actual_directories = {
        node.get("Name", ""): node.get("Id", "")
        for node in directory_refs[0].findall("w:Directory", namespace)
    }
    expected_directory_map = {
        value: _stable_cef_wix_id("cef_dir", value) for value in expected_directories
    }
    if actual_directories != expected_directory_map:
        raise BuildError(f"Installer CEF directory entries are stale: {path}")

    groups = [
        node
        for node in document.findall(".//w:ComponentGroup", namespace)
        if node.get("Id") == component_group
    ]
    if len(groups) != 1:
        raise BuildError(f"Installer CEF component group is missing or duplicated: {path}")
    actual_components = {
        node.get("Id", ""): node for node in groups[0].findall("w:Component", namespace)
    }
    expected_component_ids = {
        _stable_cef_wix_id("cef_cmp", relative) for relative in entries
    }
    if set(actual_components) != expected_component_ids:
        raise BuildError(f"Installer CEF component entries are stale: {path}")

    for relative in entries:
        component_id = _stable_cef_wix_id("cef_cmp", relative)
        component = actual_components[component_id]
        directory = relative.rsplit("/", 1)[0] if "/" in relative else ""
        expected_directory = (
            _stable_cef_wix_id("cef_dir", directory) if directory else "pm_app_folder"
        )
        if component.get("Directory") != expected_directory or component.get("Guid") != "*":
            raise BuildError(f"Installer CEF component metadata is stale: {path}")
        files = component.findall("w:File", namespace)
        leaf = relative.rsplit("/", 1)[-1]
        if len(files) != 1:
            raise BuildError(f"Installer CEF component must contain one file: {path}")
        file_node = files[0]
        expected_source = "$(var.PresentMon.TargetDir)\\" + relative.replace("/", "\\")
        expected_file_id = _stable_cef_wix_id("cef_file", relative)
        if (
            file_node.get("Id") != expected_file_id
            or file_node.get("Name") != leaf
            or file_node.get("KeyPath") != "yes"
            or file_node.get("Source") != expected_source
        ):
            raise BuildError(f"Installer CEF file metadata is stale: {path}")


def verify_cef_installer_fragments(repo_root: Path) -> None:
    installer_root = repo_root / "IntelPresentMon" / "PMInstaller"
    for file_name, component_group, lock_group in (
        ("CefBinaries.wxs", "CefBinaries", "Bin"),
        ("CefResources.wxs", "CefResources", "Resources"),
    ):
        _verify_cef_installer_fragment(
            installer_root / file_name,
            component_group,
            _cef_fragment_entries(repo_root, lock_group),
        )


def require_signed_payload(
    stamp: Path,
    signtool: Path,
    native_paths_list: str,
    uci_enabled: str | bool,
    uci_stage_dir: Path | None,
    uci_manifest_path: Path | None,
) -> None:
    if not stamp.is_file():
        raise BuildError(
            f"Production payload sign stamp is missing: {stamp}. "
            "Run pmon_sign_production_payload first."
        )
    verify_production_inputs(
        signtool,
        native_paths_list,
        uci_enabled,
        uci_stage_dir,
        uci_manifest_path,
        True,
    )


def _source_path_for_leaf(repo_root: Path, payload_root: Path, leaf: str) -> Path | None:
    lock_path = repo_root / "IntelPresentMon" / "AppCef" / "cef-lock.json"
    lock = read_json(lock_path)
    if isinstance(lock, dict):
        payload = lock.get("payload", [])
        if isinstance(payload, list):
            for entry in payload:
                if isinstance(entry, dict):
                    relative = Path(str(entry.get("path", "")))
                    if relative.name == leaf:
                        return payload_root / relative
    direct = payload_root / leaf
    return direct if direct.is_file() else None


def _expected_native_hashes(snapshot_path: Path, native_paths: list[Path]) -> dict[str, str]:
    if not snapshot_path.is_file():
        raise BuildError(f"Native payload snapshot is missing: {snapshot_path}")
    leaves = {path.name for path in native_paths}
    expected: dict[str, str] = {}
    for line in snapshot_path.read_text(encoding="ascii").splitlines():
        parts = line.split("|")
        if len(parts) < 4:
            raise BuildError(f"Native snapshot line is missing SHA256: {line}")
        leaf = Path(parts[0]).name
        if leaf in leaves:
            expected[leaf] = parts[3].upper()
    missing = leaves - expected.keys()
    if missing:
        raise BuildError(f"Native snapshot is missing entries: {', '.join(sorted(missing))}")
    return expected


def verify_msi_payload(
    msi_path: Path,
    payload_root: Path,
    repo_root: Path,
    signtool: Path,
    native_paths_list: str,
    snapshot_path: Path,
    skip_msi_authenticode: bool,
) -> None:
    msi_path = msi_path.resolve()
    if not skip_msi_authenticode:
        verify_authenticode(signtool, msi_path)
    native_paths = _split_paths(native_paths_list)
    native_by_leaf = {path.name: path for path in native_paths}
    expected = _expected_native_hashes(snapshot_path, native_paths)
    with tempfile.TemporaryDirectory(prefix="pmon-msi-verify-") as temporary_name:
        extract_root = Path(temporary_name) / "admin"
        extract_root.mkdir()
        msiexec = Path(os.environ.get("WINDIR", r"C:\Windows")) / "System32" / "msiexec.exe"
        run(
            [
                msiexec,
                "/a",
                msi_path,
                "/qn",
                f"TARGETDIR={extract_root.resolve()}",
                "/norestart",
            ],
            capture_output=True,
        )
        seen: set[str] = set()
        for extracted in extract_root.rglob("*"):
            if not extracted.is_file() or extracted.suffix.lower() not in (".exe", ".dll"):
                continue
            leaf = extracted.name
            extracted_hash = sha256_file(extracted).upper()
            if leaf in native_by_leaf:
                seen.add(leaf)
                if not skip_msi_authenticode:
                    verify_authenticode(signtool, extracted)
                if extracted_hash != expected[leaf]:
                    raise BuildError(f"Embedded MSI native payload hash mismatch for {leaf}.")
                continue
            source = _source_path_for_leaf(repo_root, payload_root, leaf)
            if source and source.is_file() and sha256_file(source).upper() != extracted_hash:
                raise BuildError(f"Embedded MSI payload hash mismatch for {leaf}.")
        missing = native_by_leaf.keys() - seen
        if missing:
            raise BuildError(f"Embedded MSI is missing native files: {', '.join(sorted(missing))}")


def sign_production_installer(
    msi_path: Path,
    payload_root: Path,
    repo_root: Path,
    signtool: Path,
    edss_script: Path,
    native_paths_list: str,
    snapshot_path: Path,
    in_process_signing: bool,
    skip_edss: bool,
) -> None:
    if not skip_edss:
        if not edss_script.is_file():
            raise BuildError(f"External EDSS signing script was not found: {edss_script}")
        arguments: list[str | Path] = [
            "powershell.exe",
            "-NoLogo",
            "-NoProfile",
            "-NonInteractive",
            "-ExecutionPolicy",
            "Bypass",
            "-File",
            edss_script,
            "-OutputRoot",
            payload_root,
            "-RepoRoot",
            repo_root,
            "-SignMsiOnly",
            "-Verify",
        ]
        if in_process_signing:
            arguments.append("-InProcessSigning")
        else:
            arguments.append("-ImpersonatedSigning")
        run(arguments)
    verify_msi_payload(
        msi_path,
        payload_root,
        repo_root,
        signtool,
        native_paths_list,
        snapshot_path,
        False,
    )
