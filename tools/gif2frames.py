#!/usr/bin/env python3
"""Convert GIF to RGB565 frame header for GP2040-CE displays."""

import argparse, os, sys
from PIL import Image

PRESETS = {"240x135": (240, 135), "160x80": (160, 80)}

def main():
    p = argparse.ArgumentParser(description="GIF -> RGB565 C header for GP2040-CE")
    p.add_argument("gif", help="Input GIF file")
    p.add_argument("-r", "--resolution", default="240x135",
                   help="Output resolution WxH or preset: 240x135, 160x80 (default: 240x135)")
    p.add_argument("-n", "--frames", type=int, default=3,
                   help="Max frames to extract (default: 3)")
    p.add_argument("-o", "--output", help="Output .h file (default: NyancatFrames.h in current dir)")
    p.add_argument("--name", default="nyancat", help="Variable name prefix (default: nyancat)")
    args = p.parse_args()

    if args.resolution in PRESETS:
        W, H = PRESETS[args.resolution]
    else:
        try:
            W, H = map(int, args.resolution.split("x"))
        except ValueError:
            sys.exit(f"Invalid resolution: {args.resolution}. Use WxH or preset: {', '.join(PRESETS)}")

    outfile = args.output or "NyancatFrames.h"

    gif = Image.open(args.gif)
    frames = []
    for i in range(args.frames):
        try:
            gif.seek(i)
            frame = gif.convert("RGB")
            fw, fh = frame.size
            scale = max(W / fw, H / fh)
            nw, nh = int(fw * scale), int(fh * scale)
            frame = frame.resize((nw, nh), Image.LANCZOS)
            left = (nw - W) // 2
            top = (nh - H) // 2
            frame = frame.crop((left, top, left + W, top + H))
            px = frame.load()
            rgb565 = [((px[x, y][0] >> 3) << 11) | ((px[x, y][1] >> 2) << 5) | (px[x, y][2] >> 3)
                      for y in range(H) for x in range(W)]
            frames.append(rgb565)
            print(f"  Frame {i+1}/{args.frames}: OK ({W}x{H}, {len(rgb565)} pixels)")
        except EOFError:
            break

    if not frames:
        sys.exit("No frames extracted!")

    name = args.name
    with open(outfile, "w") as f:
        f.write(f"// Auto-generated from: {os.path.basename(args.gif)}\n")
        f.write(f"// Resolution: {W}x{H}  Frames: {len(frames)}\n")
        f.write(f"#define {name.upper()}_FRAMES {len(frames)}\n")
        f.write(f"#define {name.upper()}_WIDTH {W}\n")
        f.write(f"#define {name.upper()}_HEIGHT {H}\n")
        f.write("#include <stdint.h>\n\n")
        for i, fr in enumerate(frames):
            f.write(f"static const uint16_t _{name}{i}[{W*H}] = {{\n    ")
            for j, p in enumerate(fr):
                f.write(f"0x{p:04X}")
                if j < len(fr) - 1:
                    f.write(", ")
                if (j + 1) % 16 == 0:
                    f.write("\n    ")
            f.write("\n};\n\n")
        f.write(f"const uint16_t* {name}_frames[{len(frames)}] = {{")
        for i in range(len(frames)):
            f.write(f"_{name}{i}")
            if i < len(frames) - 1:
                f.write(", ")
        f.write("};\n")

    sz = os.path.getsize(outfile)
    bin_sz = len(frames) * W * H * 2
    print(f"\nDone: {outfile} ({sz} bytes source, {bin_sz} bytes binary, {len(frames)} frames)")

if __name__ == "__main__":
    main()
