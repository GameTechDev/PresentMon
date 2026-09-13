from __future__ import annotations

import argparse
import sys
import xml.etree.ElementTree as ElementTree
from pathlib import Path

from pmon import packaging
from pmon.common import BuildError, ensure_python_version


def _optional_path(value: str) -> Path | None:
    return Path(value).resolve() if value else None


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Prepare and protect PresentMon package inputs.")
    subparsers = parser.add_subparsers(dest="operation", required=True)

    stage_uci = subparsers.add_parser("stage-uci")
    stage_uci.add_argument("--enabled", required=True)
    stage_uci.add_argument("--sdk-root", default="")
    stage_uci.add_argument("--stage-dir", type=Path, required=True)
    stage_uci.add_argument("--manifest-path", type=Path, required=True)

    generate_uci = subparsers.add_parser("generate-uci")
    generate_uci.add_argument("--enabled", required=True)
    generate_uci.add_argument("--stage-dir", type=Path, required=True)
    generate_uci.add_argument("--output-path", type=Path, required=True)
    generate_uci.add_argument("--wix-root", type=Path, required=True)


    snapshot = subparsers.add_parser("native-snapshot")
    snapshot.add_argument("mode", choices=("record", "verify"))
    snapshot.add_argument("--snapshot-path", type=Path, required=True)
    snapshot.add_argument("--native-paths-list", required=True)

    production = subparsers.add_parser("verify-production-inputs")
    production.add_argument("--signtool", type=Path, required=True)
    production.add_argument("--native-paths-list", required=True)
    production.add_argument("--uci-enabled", required=True)
    production.add_argument("--uci-stage-dir", default="")
    production.add_argument("--uci-manifest-path", default="")
    production.add_argument("--skip-uci", action="store_true")

    link = subparsers.add_parser("verify-link-preflight")
    link.add_argument("--repo-root", type=Path, required=True)
    link.add_argument("--dependency-root", type=Path, required=True)
    link.add_argument("--toolset", required=True)
    link.add_argument("--payload-root", type=Path, required=True)
    link.add_argument("--deployment-profile", choices=("DEVELOPER", "PRODUCTION"), required=True)
    link.add_argument("--signtool", type=Path, required=True)
    link.add_argument("--native-paths-list", required=True)
    link.add_argument("--uci-enabled", required=True)
    link.add_argument("--uci-stage-dir", default="")
    link.add_argument("--uci-manifest-path", default="")
    link.add_argument("--skip-production-pe", action="store_true")

    require_signed = subparsers.add_parser("require-signed-payload")
    require_signed.add_argument("--stamp", type=Path, required=True)
    require_signed.add_argument("--signtool", type=Path, required=True)
    require_signed.add_argument("--native-paths-list", required=True)
    require_signed.add_argument("--uci-enabled", required=True)
    require_signed.add_argument("--uci-stage-dir", default="")
    require_signed.add_argument("--uci-manifest-path", default="")

    verify_msi = subparsers.add_parser("verify-msi")
    verify_msi.add_argument("--msi-path", type=Path, required=True)
    verify_msi.add_argument("--payload-root", type=Path, required=True)
    verify_msi.add_argument("--repo-root", type=Path, required=True)
    verify_msi.add_argument("--signtool", type=Path, required=True)
    verify_msi.add_argument("--native-paths-list", required=True)
    verify_msi.add_argument("--snapshot-path", type=Path, required=True)
    verify_msi.add_argument("--skip-msi-authenticode", action="store_true")

    sign = subparsers.add_parser("sign-installer")
    sign.add_argument("--msi-path", type=Path, required=True)
    sign.add_argument("--payload-root", type=Path, required=True)
    sign.add_argument("--repo-root", type=Path, required=True)
    sign.add_argument("--signtool", type=Path, required=True)
    sign.add_argument("--edss-script", type=Path, required=True)
    sign.add_argument("--native-paths-list", required=True)
    sign.add_argument("--snapshot-path", type=Path, required=True)
    sign.add_argument("--in-process-signing", action="store_true")
    sign.add_argument("--impersonated-signing", action="store_true")
    sign.add_argument("--skip-edss", action="store_true")

    return parser.parse_args()


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        operation = arguments.operation
        if operation == "stage-uci":
            packaging.stage_uci_runtime(
                arguments.enabled,
                _optional_path(arguments.sdk_root),
                arguments.stage_dir.resolve(),
                arguments.manifest_path.resolve(),
            )
        elif operation == "generate-uci":
            packaging.generate_uci_fragment(
                arguments.enabled,
                arguments.stage_dir.resolve(),
                arguments.output_path.resolve(),
                arguments.wix_root.resolve(),
            )
        elif operation == "native-snapshot":
            packaging.native_snapshot(
                arguments.mode,
                arguments.snapshot_path.resolve(),
                arguments.native_paths_list,
            )
        elif operation == "verify-production-inputs":
            packaging.verify_production_inputs(
                arguments.signtool.resolve(),
                arguments.native_paths_list,
                arguments.uci_enabled,
                _optional_path(arguments.uci_stage_dir),
                _optional_path(arguments.uci_manifest_path),
                arguments.skip_uci,
            )
        elif operation == "verify-link-preflight":
            packaging.verify_link_preflight(
                arguments.repo_root.resolve(),
                arguments.dependency_root.resolve(),
                arguments.toolset,
                arguments.payload_root.resolve(),
                arguments.deployment_profile,
                arguments.signtool.resolve(),
                arguments.native_paths_list,
                arguments.uci_enabled,
                _optional_path(arguments.uci_stage_dir),
                _optional_path(arguments.uci_manifest_path),
                arguments.skip_production_pe,
            )
        elif operation == "require-signed-payload":
            packaging.require_signed_payload(
                arguments.stamp.resolve(),
                arguments.signtool.resolve(),
                arguments.native_paths_list,
                arguments.uci_enabled,
                _optional_path(arguments.uci_stage_dir),
                _optional_path(arguments.uci_manifest_path),
            )
        elif operation == "verify-msi":
            packaging.verify_msi_payload(
                arguments.msi_path.resolve(),
                arguments.payload_root.resolve(),
                arguments.repo_root.resolve(),
                arguments.signtool.resolve(),
                arguments.native_paths_list,
                arguments.snapshot_path.resolve(),
                arguments.skip_msi_authenticode,
            )
        elif operation == "sign-installer":
            if arguments.in_process_signing and arguments.impersonated_signing:
                raise BuildError("Specify at most one of --in-process-signing and --impersonated-signing.")
            in_process = arguments.in_process_signing or not arguments.impersonated_signing
            packaging.sign_production_installer(
                arguments.msi_path.resolve(),
                arguments.payload_root.resolve(),
                arguments.repo_root.resolve(),
                arguments.signtool.resolve(),
                arguments.edss_script.resolve(),
                arguments.native_paths_list,
                arguments.snapshot_path.resolve(),
                in_process,
                arguments.skip_edss,
            )
        return 0
    except (BuildError, OSError, ElementTree.ParseError) as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
