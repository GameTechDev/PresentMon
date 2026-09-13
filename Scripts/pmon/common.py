from __future__ import annotations

import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
import urllib.request
from collections.abc import Iterable, Sequence
from contextlib import contextmanager
from pathlib import Path


class BuildError(RuntimeError):
    pass


def log(message: str) -> None:
    print(message, flush=True)


def run(
    arguments: Sequence[str | os.PathLike[str]],
    *,
    cwd: Path | None = None,
    env: dict[str, str] | None = None,
    capture_output: bool = False,
) -> subprocess.CompletedProcess[str]:
    command = [os.fspath(argument) for argument in arguments]
    try:
        return subprocess.run(
            command,
            cwd=cwd,
            env=env,
            check=True,
            text=True,
            capture_output=capture_output,
        )
    except FileNotFoundError as error:
        raise BuildError(f"Required command was not found: {command[0]}") from error
    except subprocess.CalledProcessError as error:
        command_text = subprocess.list2cmdline(command)
        details = ""
        if capture_output:
            details = f"\n{error.stdout or ''}{error.stderr or ''}".rstrip()
        raise BuildError(
            f"Command failed with exit code {error.returncode}: {command_text}{details}"
        ) from error


def sha256_file(path: Path, chunk_size: int = 1024 * 1024) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        while chunk := stream.read(chunk_size):
            digest.update(chunk)
    return digest.hexdigest()


def require_relative_path(value: str, description: str) -> Path:
    normalized = value.replace("\\", "/")
    candidate = Path(normalized)
    if (
        not normalized
        or candidate.is_absolute()
        or candidate.drive
        or ".." in candidate.parts
        or ";" in normalized
    ):
        raise BuildError(f"Unsafe {description}: {value}")
    return candidate


def download(uri: str, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    request = urllib.request.Request(uri, headers={"User-Agent": "PresentMon-build"})
    try:
        with urllib.request.urlopen(request) as response, destination.open("wb") as output:
            shutil.copyfileobj(response, output)
    except Exception as error:
        raise BuildError(f"Failed to download {uri}: {error}") from error


def read_json(path: Path) -> object:
    try:
        with path.open("r", encoding="utf-8-sig") as stream:
            return json.load(stream)
    except (OSError, json.JSONDecodeError) as error:
        raise BuildError(f"Failed to read JSON file {path}: {error}") from error


@contextmanager
def file_lock(path: Path, timeout_seconds: int = 3600) -> Iterable[None]:
    if os.name != "nt":
        raise BuildError("PresentMon dependency locking currently requires Windows.")

    import msvcrt

    path.parent.mkdir(parents=True, exist_ok=True)
    stream = path.open("a+b")
    stream.seek(0, os.SEEK_END)
    if stream.tell() == 0:
        stream.write(b"0")
        stream.flush()
    deadline = time.monotonic() + timeout_seconds
    try:
        while True:
            try:
                stream.seek(0)
                msvcrt.locking(stream.fileno(), msvcrt.LK_NBLCK, 1)
                break
            except OSError:
                if time.monotonic() >= deadline:
                    raise BuildError(f"Timed out waiting for dependency lock: {path}")
                time.sleep(0.25)
        yield
    finally:
        try:
            stream.seek(0)
            msvcrt.locking(stream.fileno(), msvcrt.LK_UNLCK, 1)
        finally:
            stream.close()


@contextmanager
def temporary_directory(parent: Path, prefix: str) -> Iterable[Path]:
    parent.mkdir(parents=True, exist_ok=True)
    directory = Path(tempfile.mkdtemp(prefix=prefix, dir=parent))
    try:
        yield directory
    finally:
        shutil.rmtree(directory, ignore_errors=True)


def replace_directory(source: Path, destination: Path) -> None:
    destination.parent.mkdir(parents=True, exist_ok=True)
    backup = destination.with_name(f"{destination.name}.previous-{os.getpid()}")
    if backup.exists():
        shutil.rmtree(backup)
    if destination.exists():
        destination.rename(backup)
    try:
        source.rename(destination)
    except Exception:
        if backup.exists() and not destination.exists():
            backup.rename(destination)
        raise
    finally:
        if backup.exists():
            shutil.rmtree(backup)


def write_text_atomic(path: Path, content: str) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    descriptor, temporary_name = tempfile.mkstemp(
        prefix=f"{path.name}.",
        suffix=".tmp",
        dir=path.parent,
    )
    temporary = Path(temporary_name)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="") as stream:
            stream.write(content)
        temporary.replace(path)
    finally:
        temporary.unlink(missing_ok=True)


def ensure_python_version(minimum: tuple[int, int] = (3, 10)) -> None:
    if sys.version_info < minimum:
        version = ".".join(str(part) for part in minimum)
        raise BuildError(f"PresentMon build scripts require Python {version} or newer.")
