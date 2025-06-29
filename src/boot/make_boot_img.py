#!/usr/bin/env python3

import os
import sys
import subprocess
from math import ceil

PROGNAME = os.path.basename(sys.argv[0])

if len(sys.argv) != 6:
    print(f"usage: {PROGNAME} <STAGE1> <STAGE2> <KERNEL_BIN> <KERNEL_SYM> <OUTPUT_DIR>",
          file=sys.stderr)
    sys.exit(1)

stage_1 = sys.argv[1]
stage_2 = sys.argv[2]
kernel_bin = sys.argv[3]
kernel_sym = sys.argv[4]
output_dir = sys.argv[5]
boot_img = os.path.join(output_dir, "boot.img")

for file, path in {"stage 1": stage_1, "stage 2": stage_2, "kernel.bin": kernel_bin, "kernel.sym": kernel_sym}.items():
    if not os.path.exists(path):
        print(f"error: `{path}` ({file}) doesn't exist!", file=sys.stderr)
        sys.exit(2)

SECTOR_SIZE = 512
TOTAL_FLOPPY_SIZE = 2880 * SECTOR_SIZE
stage_1_size_in_sectors = 1
stage_2_size_in_sectors = ceil(os.path.getsize(stage_2) / SECTOR_SIZE)
num_reserved_sectors = stage_1_size_in_sectors + stage_2_size_in_sectors

# check the stage1/stage2/kernel bundle fits on our floppy
if TOTAL_FLOPPY_SIZE < sum(os.path.getsize(p) for p in [stage_1, stage_2, kernel_bin, kernel_sym]):
    print("error: floppy size < stage1 + stage2 + kernel!", file=sys.stderr)
    sys.exit(2)

# check we don't make a kernel larger than we can address in real mode (when we
# load it in)
kernel_size = os.path.getsize(kernel_bin)
max_size = 0x100 * 0x1000
if kernel_size > max_size:
    print(
        f"error: kernel too big! ({kernel_size} > {max_size})", file=sys.stderr)
    sys.exit(2)

def run(args):
    c = subprocess.run(args)
    args = " ".join(args)
    if c.returncode != 0:
        print(f"`{args}`: failed: {c.returncode}", file=sys.stderr)
        sys.exit(1)
    elif os.environ.get('VERBOSE') == '1':
        print("`" + args + "`: success")

# dd if=/dev/zero of=boot_img bs=512 count=2880
zeroes = b'\x00' * 512
with open(boot_img, "wb") as outf:
    for _ in range(2880):
        outf.write(zeroes)
assert os.path.getsize(boot_img) == 512 * 2880

# TODO: maybe I could do this in Python instead of relying on mtools?
run(["mformat", "-i", boot_img, "-f1440", "-B",
    stage_1, "-R", str(num_reserved_sectors), "::/"])

# dd if=stage_2 of=boot_img bs=512 skip=1 count=stage_1_size_in_sectors
with open(boot_img, "r+b") as outf, open(stage_2, "rb") as inf:
    outf.seek(512)
    for _ in range(stage_2_size_in_sectors):
        outf.write(inf.read(512))

run(["mcopy", "-i", boot_img, kernel_bin, "::/KERNEL"])
run(["mcopy", "-i", boot_img, kernel_sym, "::/KERNEL.SYM"])

assert os.path.getsize(boot_img) == 512 * 2880
