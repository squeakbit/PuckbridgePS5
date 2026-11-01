#!/usr/bin/env python3
"""Generate a C translation unit for the locally supplied adapter firmware."""

import hashlib
import pathlib
import sys

EXPECTED_SHA256 = "48084d9fa53b9bb04358f3bb127b7495dc8f7bb0b3ca1437bd24ef2b6eabdf66"


def main() -> None:
    if len(sys.argv) != 3:
        raise SystemExit("usage: embed_xbox_firmware.py INPUT OUTPUT")

    source = pathlib.Path(sys.argv[1])
    output = pathlib.Path(sys.argv[2])
    firmware = source.read_bytes()
    digest = hashlib.sha256(firmware).hexdigest()
    if digest != EXPECTED_SHA256:
        raise SystemExit(
            f"{source}: SHA-256 mismatch: expected {EXPECTED_SHA256}, got {digest}"
        )

    output.parent.mkdir(parents=True, exist_ok=True)
    lines = [
        '#include "xbox_adapter_firmware.h"',
        "",
        "const unsigned char xbox_wireless_firmware[] = {",
    ]
    for offset in range(0, len(firmware), 12):
        chunk = firmware[offset : offset + 12]
        lines.append("    " + ", ".join(f"0x{byte:02x}" for byte in chunk) + ",")
    lines.extend(
        [
            "};",
            "",
            "const size_t xbox_wireless_firmware_size =",
            "    sizeof xbox_wireless_firmware;",
            "",
        ]
    )
    output.write_text("\n".join(lines), encoding="ascii")


if __name__ == "__main__":
    main()
