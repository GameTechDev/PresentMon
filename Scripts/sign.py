from __future__ import annotations

import argparse
import sys
from pathlib import Path

from pmon.common import BuildError, ensure_python_version
from pmon.signing import sign_developer_binary


def parse_arguments() -> argparse.Namespace:
    parser = argparse.ArgumentParser(description="Run PresentMon signing steps.")
    subparsers = parser.add_subparsers(dest="operation", required=True)
    developer = subparsers.add_parser("developer")
    developer.add_argument("--configuration", required=True)
    developer.add_argument("--signtool", type=Path, required=True)
    developer.add_argument("--target", type=Path, required=True)
    developer.add_argument("--certificate-store", required=True)
    developer.add_argument("--certificate-name", required=True)
    developer.add_argument("--timestamp-url", required=True)
    return parser.parse_args()


def main() -> int:
    try:
        ensure_python_version()
        arguments = parse_arguments()
        sign_developer_binary(
            arguments.configuration,
            arguments.signtool.resolve(),
            arguments.target.resolve(),
            arguments.certificate_store,
            arguments.certificate_name,
            arguments.timestamp_url,
        )
        return 0
    except BuildError as error:
        print(f"error: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
