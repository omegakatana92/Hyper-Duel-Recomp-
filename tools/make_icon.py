#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Draw the project's original 256x256 icon (no game art)."""
import math
import struct
import sys
import zlib

S = 256


def blend(dst, src, a):
    return tuple(int(dst[i] * (1 - a) + src[i] * a) for i in range(4))


def in_round_rect(x, y, x0, y0, x1, y1, r):
    if x < x0 or x > x1 or y < y0 or y > y1:
        return False
    cx = min(max(x, x0 + r), x1 - r)
    cy = min(max(y, y0 + r), y1 - r)
    return (x - cx) ** 2 + (y - cy) ** 2 <= r * r


def main(path):
    px = [[(0, 0, 0, 0) for _ in range(S)] for _ in range(S)]
    # dark navy rounded panel
    for y in range(S):
        for x in range(S):
            if in_round_rect(x, y, 8, 8, 247, 247, 42):
                px[y][x] = (10, 18, 48, 255)
    # orbit ring
    for y in range(S):
        for x in range(S):
            d = math.hypot(x - 128, y - 118)
            if 66 <= d <= 73:
                px[y][x] = (226, 177, 58, 255)
    # stars
    for sx, sy, r in ((52, 58, 3), (205, 70, 2), (70, 188, 2), (196, 196, 3), (44, 128, 2)):
        for y in range(sy - r, sy + r + 1):
            for x in range(sx - r, sx + r + 1):
                if 0 <= x < S and 0 <= y < S and (x - sx) ** 2 + (y - sy) ** 2 <= r * r:
                    px[y][x] = (220, 235, 255, 255)
    # ship: a white arrow with gold leading edges
    ship = [(92, 118), (190, 118), (154, 94), (154, 142)]
    for y in range(88, 150):
        for x in range(86, 196):
            # point-in-triangle tests for two swept wings
            def inside(a, b, c):
                def sign(p, q, r):
                    return (p[0] - r[0]) * (q[1] - r[1]) - (q[0] - r[0]) * (p[1] - r[1])
                d1 = sign((x, y), a, b)
                d2 = sign((x, y), b, c)
                d3 = sign((x, y), c, a)
                neg = d1 < 0 or d2 < 0 or d3 < 0
                pos = d1 > 0 or d2 > 0 or d3 > 0
                return not (neg and pos)
            if inside((92, 118), (190, 118), (154, 94)) or inside((92, 118), (190, 118), (154, 142)):
                px[y][x] = (238, 244, 255, 255)
    # gold cockpit
    for y in range(110, 127):
        for x in range(132, 158):
            if (x - 145) ** 2 / 169 + (y - 118) ** 2 / 49 <= 1:
                px[y][x] = (226, 177, 58, 255)
    # gold tail bar
    for y in range(114, 123):
        for x in range(96, 112):
            px[y][x] = (226, 177, 58, 255)

    raw = bytearray()
    for y in range(S):
        raw.append(0)
        for x in range(S):
            raw.extend(px[y][x])
    def chunk(t, data):
        return struct.pack(">I", len(data)) + t + data + struct.pack(">I", zlib.crc32(t + data) & 0xFFFFFFFF)
    png = b"\x89PNG\r\n\x1a\n"
    png += chunk(b"IHDR", struct.pack(">IIBBBBB", S, S, 8, 6, 0, 0, 0))
    png += chunk(b"IDAT", zlib.compress(bytes(raw), 9))
    png += chunk(b"IEND", b"")
    with open(path, "wb") as f:
        f.write(png)


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "hyper-duel.png")
