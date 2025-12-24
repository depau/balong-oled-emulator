#!/usr/bin/env python3

import argparse
from pathlib import Path

import yaml


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("overrides", type=Path, help="Path to glyph_overrides.yaml")
    parser.add_argument("out", type=Path, help="Output header path")
    args = parser.parse_args()

    overrides_map = {}
    with args.overrides.open("r", encoding="utf-8") as f:
        overrides_data = yaml.safe_load(f)

    for ov in overrides_data.get("overrides", []):
        overrides_map[ov["target"]] = {
            "name": ov["name"],
        }

    with args.out.open("w", encoding="utf-8") as f:
        f.write("#pragma once\n\n")
        # Write glyphs sorted by code point
        for code in sorted(overrides_map.keys()):
            ov = overrides_map[code]
            f.write(f'#define GLYPH_{ov["name"]} "\\x{code:02x}"\n')

    print(f"Wrote {args.out}")


if __name__ == "__main__":
    main()
