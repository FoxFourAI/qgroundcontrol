#!/usr/bin/env python3
"""Unpack the pages packed inside an MRF triplet (.mrf + .idx + .pjg/.ppg).

The index is a flat array of 16-byte records -- big-endian page offset, then big-endian page
length, with (0, 0) marking an absent page. Records run finest level first, row-major within
each level, and each level halves the page counts rounding up, so the level boundaries have to
be recomputed to know which record belongs where. Pages are stored as whole JPEG/PNG files, so
unpacking is a seek and a copy; nothing is re-encoded.

    ./mrf_unpack.py Tarasivka.mrf                 # every level -> ./Tarasivka_pages/
    ./mrf_unpack.py Tarasivka.mrf --level 0       # just the full-resolution level
    ./mrf_unpack.py Tarasivka.mrf --list          # report the layout, extract nothing
    ./mrf_unpack.py Tarasivka.mrf --mosaic        # also stitch each level into one image
"""

import argparse
import math
import os
import struct
import sys
import xml.etree.ElementTree as ET

RECORD = 16  # bytes per index entry
ORIGIN_SHIFT = 20037508.342789244  # half the EPSG:3857 world, in meters
DEFAULT_PAGE = 256

DATA_EXT = {"PNG": ".ppg", "JPEG": ".pjg"}


def parse_mrf(path):
    """Pull the page geometry and file names out of an MRF header."""
    root = ET.parse(path).getroot()

    raster = root.find("Raster")
    if raster is None:
        sys.exit(f"{path}: no <Raster> element -- not an MRF header")

    def dims(tag, default):
        el = raster.find(tag)
        if el is None:
            return default, default
        return int(el.get("x", default)), int(el.get("y", default))

    size_x, size_y = dims("Size", 0)
    # Page size is 256x256 unless the header says otherwise.
    page_x, page_y = dims("PageSize", DEFAULT_PAGE)
    if not size_x or not size_y:
        sys.exit(f"{path}: <Size> is missing or zero")

    compression = (raster.findtext("Compression") or "JPEG").strip().upper()

    base = os.path.splitext(path)[0]
    idx_path = root.findtext("IndexFile") or (base + ".idx")
    data_path = root.findtext("DataFile")
    if data_path is None:
        # GDAL derives the data file from the basename when <DataFile> is absent.
        data_path = base + DATA_EXT.get(compression, ".til")
        if not os.path.exists(data_path):
            for ext in DATA_EXT.values():
                if os.path.exists(base + ext):
                    data_path = base + ext
                    break

    # Paths in the header are relative to the header's own directory.
    here = os.path.dirname(os.path.abspath(path))
    idx_path = os.path.join(here, idx_path)
    data_path = os.path.join(here, data_path)

    info = {
        "compression": compression,
        "page_x": page_x,
        "page_y": page_y,
        # Pages across and down at full resolution, rounding up: a partly covered edge page
        # still occupies a whole page.
        "pages_x": (size_x + page_x - 1) // page_x,
        "pages_y": (size_y + page_y - 1) // page_y,
        "size_x": size_x,
        "size_y": size_y,
        "idx_path": idx_path,
        "data_path": data_path,
        "has_rsets": root.find("Rsets") is not None,
    }
    info.update(read_geotags(root, info))
    return info


def read_geotags(root, info):
    """Recover the zoom and the top-left tile index from the bounding box, when present.

    The bounding box is tile-aligned, so the ground size of one page gives the zoom directly and
    the box corners give the absolute XYZ indices of the full-resolution level. Only level 0 gets
    real tile coordinates: an overview page covers a 2x2 block of pages, which lines up with a
    real tile at the next zoom only when the origin happens to be even.
    """
    box = root.find("./GeoTags/BoundingBox")
    if box is None:
        return {}
    try:
        minx = float(box.get("minx"))
        maxx = float(box.get("maxx"))
        maxy = float(box.get("maxy"))
    except (TypeError, ValueError):
        return {}

    page_meters = ((maxx - minx) / info["size_x"]) * info["page_x"]
    if page_meters <= 0:
        return {}
    # One page spans the whole world at zoom 0, so the page count across the world is 2**zoom.
    zoom = round(math.log2((2.0 * ORIGIN_SHIFT) / page_meters))

    return {
        "zoom": zoom,
        "tile_x0": int(round((minx + ORIGIN_SHIFT) / page_meters)),
        "tile_y0": int(round((ORIGIN_SHIFT - maxy) / page_meters)),
    }


def level_shapes(pages_x, pages_y):
    """Page counts per level, finest first, halving and rounding up until a level is one page."""
    shapes = [(pages_x, pages_y)]
    while shapes[-1] != (1, 1):
        x, y = shapes[-1]
        shapes.append((max(1, (x + 1) // 2), max(1, (y + 1) // 2)))
    return shapes


def page_extension(blob):
    if blob[:3] == b"\xff\xd8\xff":
        return ".jpg"
    if blob[:8] == b"\x89PNG\r\n\x1a\n":
        return ".png"
    return ".bin"


def main():
    ap = argparse.ArgumentParser(description="Unpack the pages of an MRF file.")
    ap.add_argument("mrf", help="path to the .mrf header")
    ap.add_argument("-o", "--out", help="output directory (default: <basename>_pages)")
    ap.add_argument("--level", type=int, action="append",
                    help="only this level (0 = full resolution); repeatable")
    ap.add_argument("--list", action="store_true", help="report the layout without extracting")
    ap.add_argument("--mosaic", action="store_true",
                    help="also stitch each extracted level into one image (needs Pillow)")
    args = ap.parse_args()

    info = parse_mrf(args.mrf)
    shapes = level_shapes(info["pages_x"], info["pages_y"])

    for path in (info["idx_path"], info["data_path"]):
        if not os.path.exists(path):
            sys.exit(f"missing {path}")

    idx_size = os.path.getsize(info["idx_path"])
    expected = sum(x * y for x, y in shapes) * RECORD

    print(f"{args.mrf}: {info['size_x']}x{info['size_y']} px, "
          f"{info['page_x']}x{info['page_y']} pages, {info['compression']}")
    if "zoom" in info:
        print(f"  zoom {info['zoom']}, level 0 starts at tile "
              f"x={info['tile_x0']} y={info['tile_y0']}")
    print(f"  index {info['idx_path']} ({idx_size} bytes, {idx_size // RECORD} records)")
    print(f"  data  {info['data_path']} ({os.path.getsize(info['data_path'])} bytes)")

    if idx_size != expected:
        # A single-level file with no <Rsets> is legitimate; anything else is a real mismatch.
        levels_present = idx_size // RECORD == info["pages_x"] * info["pages_y"]
        note = "single level, no overviews" if levels_present else "LAYOUT MISMATCH"
        print(f"  expected {expected} bytes for {len(shapes)} levels -> {note}")
        if levels_present:
            shapes = shapes[:1]
        else:
            sys.exit("  refusing to guess at the level layout")

    with open(info["idx_path"], "rb") as f:
        index = f.read()

    out_root = args.out or (os.path.splitext(args.mrf)[0] + "_pages")
    wanted = set(args.level) if args.level else None

    record = 0
    for level, (px, py) in enumerate(shapes):
        count = px * py
        if wanted is not None and level not in wanted:
            record += count
            continue

        entries = [struct.unpack_from(">QQ", index, (record + i) * RECORD) for i in range(count)]
        record += count
        present = [e for e in entries if e[1]]
        print(f"  level {level}: {px}x{py} pages, {len(present)}/{count} present, "
              f"{sum(e[1] for e in present)} bytes")
        if args.list:
            continue

        level_dir = os.path.join(out_root, f"level{level}")
        os.makedirs(level_dir, exist_ok=True)

        written = 0
        with open(info["data_path"], "rb") as data:
            for i, (offset, size) in enumerate(entries):
                if not size:
                    continue
                data.seek(offset)
                blob = data.read(size)
                if len(blob) != size:
                    print(f"    page {i}: short read at {offset} "
                          f"({len(blob)} of {size} bytes)", file=sys.stderr)
                    continue
                col, row = i % px, i // px
                # Level 0 pages are real XYZ tiles, so name them that way; deeper levels are
                # only page coordinates.
                if level == 0 and "tile_x0" in info:
                    name = f"{info['tile_x0'] + col}_{info['tile_y0'] + row}"
                else:
                    name = f"{col}_{row}"
                with open(os.path.join(level_dir, name + page_extension(blob)), "wb") as out:
                    out.write(blob)
                written += 1

        print(f"    -> {written} files in {level_dir}")
        if args.mosaic:
            build_mosaic(level_dir, info, px, py, level)


def build_mosaic(level_dir, info, px, py, level):
    try:
        from PIL import Image
    except ImportError:
        print("    --mosaic needs Pillow (pip install Pillow)", file=sys.stderr)
        return

    page_x, page_y = info["page_x"], info["page_y"]
    canvas = Image.new("RGBA", (px * page_x, py * page_y))
    placed = 0
    for name in os.listdir(level_dir):
        stem, ext = os.path.splitext(name)
        if ext not in (".jpg", ".png"):
            continue
        a, _, b = stem.partition("_")
        col, row = int(a), int(b)
        if level == 0 and "tile_x0" in info:
            col -= info["tile_x0"]
            row -= info["tile_y0"]
        with Image.open(os.path.join(level_dir, name)) as tile:
            canvas.paste(tile.convert("RGBA"), (col * page_x, row * page_y))
        placed += 1

    path = os.path.join(os.path.dirname(level_dir), f"level{level}_mosaic.png")
    canvas.save(path)
    print(f"    -> mosaic of {placed} pages: {path}")


if __name__ == "__main__":
    main()
