#!/usr/bin/env python3

# Converts ppm files to png if you can't open ppm filez
from __future__ import annotations

import argparse
import struct
import zlib
from pathlib import Path


def _read_token(data: bytes, index: int) -> tuple[bytes, int]:
    length = len(data)
    while index < length:
        if data[index:index + 1] == b"#":
            while index < length and data[index:index + 1] not in (b"\n", b"\r"):
                index += 1
        elif data[index:index + 1].isspace():
            index += 1
        else:
            break

    start = index
    while index < length and not data[index:index + 1].isspace():
        index += 1
    return data[start:index], index


def read_ppm(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    index = 0

    magic, index = _read_token(data, index)
    width_token, index = _read_token(data, index)
    height_token, index = _read_token(data, index)
    max_token, index = _read_token(data, index)

    if magic not in (b"P3", b"P6"):
        raise ValueError(f"Unsupported PPM format {magic!r} in {path}")

    width = int(width_token)
    height = int(height_token)
    maximum = int(max_token)
    if maximum <= 0 or maximum > 255:
        raise ValueError(f"Only 8-bit PPM files are supported: {path}")

    if magic == b"P3":
        values: list[int] = []
        while len(values) < width * height * 3:
            token, index = _read_token(data, index)
            if not token:
                break
            values.append(int(token))
        if len(values) != width * height * 3:
            raise ValueError(f"Incomplete P3 pixel data in {path}")
        if maximum != 255:
            values = [round(value * 255 / maximum) for value in values]
        pixels = bytes(values)
    else:
        while index < len(data) and data[index:index + 1].isspace():
            index += 1
        expected = width * height * 3
        pixels = data[index:index + expected]
        if len(pixels) != expected:
            raise ValueError(f"Incomplete P6 pixel data in {path}")
        if maximum != 255:
            pixels = bytes(round(value * 255 / maximum) for value in pixels)

    return width, height, pixels


def png_chunk(chunk_type: bytes, payload: bytes) -> bytes:
    return (
        struct.pack(">I", len(payload))
        + chunk_type
        + payload
        + struct.pack(">I", zlib.crc32(chunk_type + payload) & 0xFFFFFFFF)
    )


def write_png(path: Path, width: int, height: int, pixels: bytes) -> None:
    stride = width * 3
    rows = []
    for y in range(height):
        start = y * stride
        rows.append(b"\x00" + pixels[start:start + stride])

    header = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    output = b"\x89PNG\r\n\x1a\n"
    output += png_chunk(b"IHDR", header)
    output += png_chunk(b"IDAT", zlib.compress(b"".join(rows), level=9))
    output += png_chunk(b"IEND", b"")
    path.write_bytes(output)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument(
        "directory",
        nargs="?",
        type=Path,
        default=Path(__file__).resolve().parents[1] / "renders",
        help="Folder containing PPM files (default: project renders folder)",
    )
    args = parser.parse_args()

    directory = args.directory.resolve()
    if not directory.exists():
        raise SystemExit(f"Render directory does not exist: {directory}")

    ppm_files = sorted(directory.glob("*.ppm"))
    if not ppm_files:
        raise SystemExit(f"No PPM files found in {directory}")

    for ppm_path in ppm_files:
        width, height, pixels = read_ppm(ppm_path)
        png_path = ppm_path.with_suffix(".png")
        write_png(png_path, width, height, pixels)
        print(f"Converted {ppm_path.name} -> {png_path.name}")

    return 0


if __name__ == "__main__":
    raise SystemExit(main())