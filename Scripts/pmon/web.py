from __future__ import annotations

import hashlib
import shutil
from pathlib import Path

from .common import BuildError, file_lock, log, run, sha256_file


def _fingerprint(web_root: Path) -> str:
    digest = hashlib.sha256()
    for name in ("package.json", "package-lock.json"):
        path = web_root / name
        if not path.is_file():
            raise BuildError(f"Web dependency input is missing: {path}")
        digest.update(name.encode("ascii"))
        digest.update(sha256_file(path).encode("ascii"))
    return digest.hexdigest()


def ensure(repo_root: Path, dependency_root: Path, force: bool = False) -> None:
    web_root = repo_root / "IntelPresentMon" / "AppCef" / "ipm-ui-vue"
    node_modules = web_root / "node_modules"
    stamp = node_modules / ".pmon-dependencies.sha256"
    expected = _fingerprint(web_root)
    with file_lock(dependency_root / "locks" / "web.lock", timeout_seconds=1800):
        npm = shutil.which("npm.cmd") or shutil.which("npm")
        if not npm:
            raise BuildError("npm was not found. Install Node.js before configuring PresentMon.")
        if not force and node_modules.is_dir() and stamp.is_file():
            if stamp.read_text(encoding="ascii").strip() == expected:
                try:
                    run([npm, "ls", "--depth=0", "--silent"], cwd=web_root, capture_output=True)
                    log("Web dependencies are present and current.")
                    return
                except BuildError:
                    log("Web dependencies are incomplete or invalid; reinstalling them.")
        log("Installing locked web dependencies with npm ci.")
        run([npm, "ci"], cwd=web_root)
        stamp.write_text(expected + "\n", encoding="ascii")


def build(repo_root: Path, dependency_root: Path) -> None:
    ensure(repo_root, dependency_root)
    web_root = repo_root / "IntelPresentMon" / "AppCef" / "ipm-ui-vue"
    npm = shutil.which("npm.cmd") or shutil.which("npm")
    if not npm:
        raise BuildError("npm was not found. Install Node.js before building PresentMonUI.")
    run([npm, "run", "build"], cwd=web_root)
