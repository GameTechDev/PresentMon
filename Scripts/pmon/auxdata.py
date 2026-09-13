from __future__ import annotations

import json
import shutil
import tarfile
import urllib.request
from pathlib import Path

from .common import BuildError, download, file_lock, log, read_json, run, sha256_file


OWNER = "planetchili"
REPOSITORY = "IPMAuxTestData"


def _git_output(arguments: list[str], cwd: Path) -> str:
    result = run(["git", *arguments], cwd=cwd, capture_output=True)
    return result.stdout.strip()


def _load_lock(repo_root: Path) -> str:
    lock_path = repo_root / "Tests" / "aux-data.lock.json"
    lock = read_json(lock_path)
    if not isinstance(lock, dict):
        raise BuildError(f"Auxiliary data lock is not an object: {lock_path}")
    pinned_commit = str(lock.get("pinnedCommitHash", "")).strip()
    if not pinned_commit:
        raise BuildError(f"Auxiliary data lock has no pinnedCommitHash: {lock_path}")
    return pinned_commit


def _load_manifest_from_commit(aux_root: Path, pinned_commit: str) -> dict[str, object]:
    text = _git_output(["show", f"{pinned_commit}:etl-manifest.json"], aux_root)
    try:
        manifest = json.loads(text)
    except json.JSONDecodeError as error:
        raise BuildError(f"Invalid auxiliary data manifest at {pinned_commit}: {error}") from error
    if not isinstance(manifest, dict):
        raise BuildError("Auxiliary data manifest is not an object.")
    return manifest


def validate(repo_root: Path) -> list[str]:
    pinned_commit = _load_lock(repo_root)
    aux_root = repo_root / "Tests" / "AuxData"
    errors: list[str] = []
    if not (aux_root / ".git").is_dir():
        return [f"Auxiliary data repository is missing: {aux_root}"]

    try:
        current_commit = _git_output(["rev-parse", "HEAD"], aux_root)
    except BuildError as error:
        return [str(error)]
    if current_commit.lower() != pinned_commit.lower():
        errors.append(
            f"Auxiliary data commit mismatch; expected {pinned_commit}, found {current_commit}."
        )
        return errors

    try:
        manifest = _load_manifest_from_commit(aux_root, pinned_commit)
    except BuildError as error:
        return [str(error)]
    entries = manifest.get("etlFiles")
    if not isinstance(entries, list) or not entries:
        return ["Auxiliary data manifest has no etlFiles entries."]

    data_root = aux_root / "Data"
    for entry in entries:
        if not isinstance(entry, dict):
            errors.append("Auxiliary data manifest contains a malformed entry.")
            continue
        name = str(entry.get("name", "")).strip()
        expected_hash = str(entry.get("hash", "")).strip().lower()
        if not name or not expected_hash:
            errors.append("Auxiliary data manifest entry is missing name or hash.")
            continue
        path = data_root / name
        if not path.is_file():
            errors.append(f"Missing auxiliary test file: {name}")
        elif sha256_file(path) != expected_hash:
            errors.append(f"Hash mismatch for auxiliary test file: {name}")
    return errors


def _release(tag: str) -> dict[str, object]:
    uri = f"https://api.github.com/repos/{OWNER}/{REPOSITORY}/releases/tags/{tag}"
    request = urllib.request.Request(
        uri,
        headers={
            "User-Agent": "PresentMon-auxdata",
            "Accept": "application/vnd.github+json",
        },
    )
    try:
        with urllib.request.urlopen(request) as response:
            value = json.load(response)
    except Exception as error:
        raise BuildError(f"Failed to read auxiliary data release {tag}: {error}") from error
    if not isinstance(value, dict):
        raise BuildError(f"Unexpected GitHub release response for tag {tag}.")
    return value


def _extract_single_file(archive: Path, data_root: Path, expected_name: str) -> None:
    try:
        with tarfile.open(archive, "r:gz") as bundle:
            members = [member for member in bundle.getmembers() if member.isfile()]
            matching = [member for member in members if Path(member.name).name == expected_name]
            if len(matching) != 1:
                raise BuildError(
                    f"Archive {archive.name} does not contain exactly one {expected_name} file."
                )
            member = matching[0]
            source = bundle.extractfile(member)
            if source is None:
                raise BuildError(f"Failed to read {expected_name} from {archive.name}.")
            destination = data_root / expected_name
            temporary = destination.with_suffix(f"{destination.suffix}.incoming")
            with source, temporary.open("wb") as output:
                shutil.copyfileobj(source, output)
            temporary.replace(destination)
    except (OSError, tarfile.TarError) as error:
        raise BuildError(f"Failed to extract {archive}: {error}") from error


def restore(repo_root: Path, dependency_root: Path) -> None:
    pinned_commit = _load_lock(repo_root)
    aux_root = repo_root / "Tests" / "AuxData"
    remote_uri = f"https://github.com/{OWNER}/{REPOSITORY}.git"
    if not aux_root.exists():
        log(f"Cloning auxiliary data repository into {aux_root}")
        run(["git", "clone", remote_uri, aux_root])
    if not (aux_root / ".git").is_dir():
        raise BuildError(f"Auxiliary data path is not a Git repository: {aux_root}")

    current_remote = _git_output(["remote", "get-url", "origin"], aux_root)
    if current_remote.startswith("git@github.com:"):
        run(["git", "remote", "set-url", "origin", remote_uri], cwd=aux_root)
        run(["git", "remote", "set-url", "--push", "origin", remote_uri], cwd=aux_root)

    log("Fetching the pinned auxiliary data commit.")
    run(["git", "fetch", "origin", "--tags"], cwd=aux_root)
    run(["git", "checkout", "--detach", pinned_commit], cwd=aux_root)
    manifest = _load_manifest_from_commit(aux_root, pinned_commit)
    entries = manifest.get("etlFiles")
    if not isinstance(entries, list) or not entries:
        raise BuildError("Auxiliary data manifest has no etlFiles entries.")

    data_root = aux_root / "Data"
    data_root.mkdir(parents=True, exist_ok=True)
    cache_root = dependency_root / "downloads" / "auxdata"
    releases: dict[str, dict[str, object]] = {}
    for entry in entries:
        if not isinstance(entry, dict):
            raise BuildError("Auxiliary data manifest contains a malformed entry.")
        name = str(entry.get("name", "")).strip()
        expected_hash = str(entry.get("hash", "")).strip().lower()
        tag = str(entry.get("releaseTag", "")).strip()
        if not name or not expected_hash or not tag:
            raise BuildError("Auxiliary data manifest entry is missing name, hash, or releaseTag.")
        destination = data_root / name
        if destination.is_file() and sha256_file(destination) == expected_hash:
            continue

        if tag not in releases:
            releases[tag] = _release(tag)
        release = releases[tag]
        assets = release.get("assets")
        if not isinstance(assets, list):
            raise BuildError(f"Auxiliary data release {tag} has no assets.")
        archive_name = f"{name}.tar.gz"
        asset = next(
            (
                item
                for item in assets
                if isinstance(item, dict) and str(item.get("name", "")).lower() == archive_name.lower()
            ),
            None,
        )
        if asset is None:
            raise BuildError(f"Asset {archive_name} was not found in release {tag}.")
        uri = str(asset.get("browser_download_url", ""))
        if not uri:
            raise BuildError(f"Asset {archive_name} has no download URL.")
        archive = cache_root / tag / archive_name
        for attempt in range(2):
            if not archive.is_file():
                temporary = archive.with_suffix(f"{archive.suffix}.incoming")
                log(f"Downloading auxiliary test data {archive_name}.")
                download(uri, temporary)
                temporary.replace(archive)
            try:
                _extract_single_file(archive, data_root, name)
            except BuildError:
                archive.unlink(missing_ok=True)
                if attempt == 0:
                    log(f"Cached auxiliary archive is invalid; downloading {archive_name} again.")
                    continue
                raise
            actual_hash = sha256_file(destination)
            if actual_hash == expected_hash:
                break
            archive.unlink(missing_ok=True)
            destination.unlink(missing_ok=True)
            if attempt == 0:
                log(f"Cached auxiliary payload hash is invalid; downloading {archive_name} again.")
                continue
            raise BuildError(
                f"Hash mismatch after restoring {name}; expected {expected_hash}, found {actual_hash}."
            )


def ensure(repo_root: Path, dependency_root: Path, force: bool = False) -> None:
    lock_path = dependency_root / "locks" / "auxdata.lock"
    with file_lock(lock_path, timeout_seconds=1800):
        errors = validate(repo_root)
        if not force and not errors:
            log("Pinned auxiliary test data is present and valid.")
            return
        if errors:
            log("Auxiliary test data is missing, stale, or invalid; restoring it.")
        restore(repo_root, dependency_root)
        errors = validate(repo_root)
        if errors:
            raise BuildError("Auxiliary data restore did not validate:\n" + "\n".join(errors))
        log("Pinned auxiliary test data was restored and validated.")
