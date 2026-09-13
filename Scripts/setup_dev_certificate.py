from __future__ import annotations

import argparse
import ctypes
import subprocess
import sys
import tempfile
from ctypes import wintypes
from pathlib import Path

from pmon.common import BuildError, ensure_python_version, run


SUBJECT_NAME = "Test Certificate - For Internal Use Only"
CURRENT_USER = 0x00010000
LOCAL_MACHINE = 0x00020000
READ_ONLY = 0x00008000
X509_ASN_ENCODING = 0x00000001
PKCS_7_ASN_ENCODING = 0x00010000
FIND_SUBJECT_STRING = 0x00080007
SHA1_HASH_PROPERTY = 3
SYSTEM_STORE_PROVIDER = 10


class CertificateContext(ctypes.Structure):
    _fields_ = [
        ("dwCertEncodingType", wintypes.DWORD),
        ("pbCertEncoded", ctypes.POINTER(ctypes.c_ubyte)),
        ("cbCertEncoded", wintypes.DWORD),
        ("pCertInfo", ctypes.c_void_p),
        ("hCertStore", ctypes.c_void_p),
    ]


CertificateContextPointer = ctypes.POINTER(CertificateContext)


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Create and trust the PresentMon developer code-signing certificate."
    )
    parser.add_argument("--check", action="store_true", help="Check setup without changing stores.")
    parser.add_argument(
        "--certificate-output",
        type=Path,
        help="Public certificate location used by the manual trust procedure.",
    )
    return parser.parse_args()


def _crypt32() -> ctypes.WinDLL:
    if sys.platform != "win32":
        raise BuildError("Developer certificate setup is supported only on Windows.")
    library = ctypes.WinDLL("crypt32", use_last_error=True)
    library.CertOpenStore.argtypes = [
        ctypes.c_void_p,
        wintypes.DWORD,
        ctypes.c_void_p,
        wintypes.DWORD,
        ctypes.c_void_p,
    ]
    library.CertOpenStore.restype = ctypes.c_void_p
    library.CertFindCertificateInStore.argtypes = [
        ctypes.c_void_p,
        wintypes.DWORD,
        wintypes.DWORD,
        wintypes.DWORD,
        ctypes.c_void_p,
        CertificateContextPointer,
    ]
    library.CertFindCertificateInStore.restype = CertificateContextPointer
    library.CertEnumCertificatesInStore.argtypes = [
        ctypes.c_void_p,
        CertificateContextPointer,
    ]
    library.CertEnumCertificatesInStore.restype = CertificateContextPointer
    library.CertGetCertificateContextProperty.argtypes = [
        CertificateContextPointer,
        wintypes.DWORD,
        ctypes.c_void_p,
        ctypes.POINTER(wintypes.DWORD),
    ]
    library.CertGetCertificateContextProperty.restype = wintypes.BOOL
    library.CertFreeCertificateContext.argtypes = [CertificateContextPointer]
    library.CertFreeCertificateContext.restype = wintypes.BOOL
    library.CertCloseStore.argtypes = [ctypes.c_void_p, wintypes.DWORD]
    library.CertCloseStore.restype = wintypes.BOOL
    return library


def _open_store(library: ctypes.WinDLL, name: str, location: int) -> int:
    store_name = ctypes.c_wchar_p(name)
    store = library.CertOpenStore(
        ctypes.c_void_p(SYSTEM_STORE_PROVIDER),
        0,
        None,
        location | READ_ONLY,
        ctypes.cast(store_name, ctypes.c_void_p),
    )
    if not store:
        raise BuildError(f"Could not open the Windows certificate store {name}.")
    return store


def _thumbprint(library: ctypes.WinDLL, context: CertificateContextPointer) -> bytes:
    size = wintypes.DWORD()
    if not library.CertGetCertificateContextProperty(
        context, SHA1_HASH_PROPERTY, None, ctypes.byref(size)
    ):
        raise BuildError("Could not read a certificate thumbprint.")
    buffer = (ctypes.c_ubyte * size.value)()
    if not library.CertGetCertificateContextProperty(
        context, SHA1_HASH_PROPERTY, buffer, ctypes.byref(size)
    ):
        raise BuildError("Could not read a certificate thumbprint.")
    return bytes(buffer[: size.value])


def _find_developer_certificate() -> tuple[bytes, bytes] | None:
    library = _crypt32()
    store = _open_store(library, "MY", CURRENT_USER)
    context = CertificateContextPointer()
    try:
        subject = ctypes.c_wchar_p(SUBJECT_NAME)
        context = library.CertFindCertificateInStore(
            store,
            X509_ASN_ENCODING | PKCS_7_ASN_ENCODING,
            0,
            FIND_SUBJECT_STRING,
            ctypes.cast(subject, ctypes.c_void_p),
            None,
        )
        if not context:
            return None
        encoded = ctypes.string_at(
            context.contents.pbCertEncoded,
            context.contents.cbCertEncoded,
        )
        return _thumbprint(library, context), encoded
    finally:
        if context:
            library.CertFreeCertificateContext(context)
        library.CertCloseStore(store, 0)


def _root_contains(thumbprint: bytes) -> bool:
    library = _crypt32()
    store = _open_store(library, "ROOT", LOCAL_MACHINE)
    context = CertificateContextPointer()
    try:
        while True:
            context = library.CertEnumCertificatesInStore(store, context)
            if not context:
                return False
            if _thumbprint(library, context) == thumbprint:
                return True
    finally:
        library.CertCloseStore(store, 0)


def _is_elevated() -> bool:
    return bool(ctypes.windll.shell32.IsUserAnAdmin())


def _create_certificate() -> None:
    policy = "\r\n".join(
        (
            "[Version]",
            'Signature="$Windows NT$"',
            "",
            "[NewRequest]",
            f'Subject="CN={SUBJECT_NAME}"',
            "KeyLength=2048",
            "KeySpec=2",
            "Exportable=TRUE",
            "MachineKeySet=FALSE",
            "RequestType=Cert",
            "HashAlgorithm=sha256",
            "KeyUsage=0x80",
            'ProviderName="Microsoft Enhanced RSA and AES Cryptographic Provider"',
            "ProviderType=24",
            "",
            "[EnhancedKeyUsageExtension]",
            "OID=1.3.6.1.5.5.7.3.3",
            "",
        )
    )
    with tempfile.TemporaryDirectory(prefix="pmon-cert-") as temporary_name:
        temporary = Path(temporary_name)
        policy_path = temporary / "certificate.inf"
        certificate_path = temporary / "certificate.cer"
        policy_path.write_text(policy, encoding="ascii", newline="")
        run(["certreq.exe", "-q", "-new", "-user", policy_path, certificate_path])
        if _find_developer_certificate() is not None:
            return
        try:
            run(["certreq.exe", "-q", "-accept", "-user", certificate_path])
        except BuildError:
            # Self-signed dev certs are often installed by certreq -new; -accept can
            # fail with CERT_E_UNTRUSTEDROOT until the same cert is trusted in Root.
            if _find_developer_certificate() is not None:
                return
            run(["certutil.exe", "-user", "-addstore", "My", certificate_path])


def configure(certificate_output: Path, check_only: bool) -> None:
    certificate = _find_developer_certificate()
    if certificate is None:
        if check_only:
            raise BuildError(
                f"Developer certificate is missing from CurrentUser\\My: {SUBJECT_NAME}"
            )
        _create_certificate()
        certificate = _find_developer_certificate()
        if certificate is None:
            raise BuildError("Certificate creation completed but it was not installed.")
        print("Created the developer certificate in CurrentUser\\My.")
    else:
        print("Developer certificate is present in CurrentUser\\My.")

    thumbprint, encoded = certificate
    if _root_contains(thumbprint):
        print("Developer certificate is trusted in LocalMachine\\Root.")
        return
    if check_only:
        raise BuildError("Developer certificate is not trusted in LocalMachine\\Root.")

    certificate_output.parent.mkdir(parents=True, exist_ok=True)
    certificate_output.write_bytes(encoded)
    if not _is_elevated():
        command = subprocess.list2cmdline(
            ["certutil.exe", "-addstore", "-f", "Root", str(certificate_output)]
        )
        raise BuildError(
            "Administrator rights are required for LocalMachine\\Root trust. "
            f"Run this command from an elevated Command Prompt: {command}"
        )
    run(["certutil.exe", "-addstore", "-f", "Root", certificate_output])
    if not _root_contains(thumbprint):
        raise BuildError("certutil completed but LocalMachine\\Root trust was not found.")
    print("Trusted the developer certificate in LocalMachine\\Root.")


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        output = arguments.certificate_output
        if output is None:
            output = (
                Path(__file__).resolve().parents[1]
                / "build"
                / "ThirdParty"
                / "setup"
                / "presentmon-testcert.cer"
            )
        configure(output.resolve(), arguments.check)
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
