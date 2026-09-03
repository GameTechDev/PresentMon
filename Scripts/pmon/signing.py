from __future__ import annotations

from pathlib import Path

from .common import BuildError, run

_DEVELOPER_CERTIFICATE_STORE_FALLBACKS: dict[str, tuple[str, ...]] = {
    "my": ("PrivateCertStore",),
    "privatecertstore": ("My",),
}


def _developer_certificate_stores(primary_store: str) -> list[str]:
    stores = [primary_store]
    normalized = primary_store.replace(" ", "").lower()
    for fallback in _DEVELOPER_CERTIFICATE_STORE_FALLBACKS.get(normalized, ()):
        if fallback not in stores:
            stores.append(fallback)
    return stores


def sign_developer_binary(
    configuration: str,
    signtool: Path,
    target: Path,
    certificate_store: str,
    certificate_name: str,
    timestamp_url: str,
) -> None:
    if configuration.lower() != "release":
        return
    if not signtool.is_file():
        raise BuildError(
            "SignTool was not found. Install the Windows SDK or set "
            "PMON_SIGNTOOL_EXECUTABLE."
        )
    if not target.is_file():
        raise BuildError(f"Signing target output was not found: {target}")
    stores = _developer_certificate_stores(certificate_store)
    sign_errors: list[BuildError] = []
    for store in stores:
        try:
            run(
                [
                    signtool,
                    "sign",
                    "/a",
                    "/v",
                    "/s",
                    store,
                    "/n",
                    certificate_name,
                    "/t",
                    timestamp_url,
                    "/fd",
                    "sha1",
                    target,
                ],
                capture_output=True,
            )
            sign_errors.clear()
            break
        except BuildError as error:
            sign_errors.append(error)
    if sign_errors:
        last_error = sign_errors[-1]
        raise BuildError(
            "Developer signing failed. Install the test certificate in PrivateCertStore "
            "(legacy Visual Studio / MSBuild setup) or run Scripts/setup_dev_certificate.py "
            f"for CurrentUser\\My. {last_error}"
        ) from last_error
    run([signtool, "verify", "/pa", "/v", target])
