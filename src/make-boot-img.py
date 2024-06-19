import os, sys, subprocess
from math import ceil

if len(sys.argv) < 5:
    print("usage: get-stage2.py <STAGE1> <STAGE2> <KERNEL> <OUTPUT_DIR>", file=sys.stderr )
    sys.exit(1)

stage_1 = sys.argv[1]
stage_2 = sys.argv[2]
kernel = sys.argv[3]
output_dir = sys.argv[4]
boot_img = os.path.join(output_dir, "boot.img")

for f in [stage_1, stage_2, kernel]:
    if not os.path.exists(f):
        print("error: " + f + " doesn't exist!", file=sys.stderr)
        exit(2)

sector_size = 512
total_floppy_size = 2880 * sector_size
stage_1_size_in_sectors = 1
stage_2_size_in_sectors = ceil(os.path.getsize(stage_2) / sector_size)
num_reserved_sectors = stage_1_size_in_sectors + stage_2_size_in_sectors

total_size = sum(os.path.getsize(p) for p in [stage_1, stage_2, kernel])
if total_size > total_floppy_size:
    print("error: stage1 + stage2 + kernel > floppy size!", file=sys.stderr)
    exit(2)

def run(args):
    c = subprocess.run(args)
    args = " ".join(args)
    if c.returncode != 0:
        print("`" + args + "`: failed: " + str(c.returncode), file=sys.stderr)
        exit(1)
    elif os.environ.get('VERBOSE') == '1':
        print("`" + args + "`: success")

# TODO: maybe I could do this in Python instead of relying on mtools and dd?
run(["dd", "status=none", "if=/dev/zero", "of="+boot_img, "bs=512", "count=2880"])
run(["dd", "status=none", "if="+stage_1, "of="+boot_img, "bs=512", "count=1", "conv=notrunc"])
run(["mformat", "-i", boot_img, "-f1440", "-k", "-R", str(num_reserved_sectors), "::/"])
run(["dd", "status=none", "if="+stage_2, "of="+boot_img, "bs=512", "seek=1", "count="+str(stage_2_size_in_sectors), "conv=notrunc"])
run(["mcopy", "-i", boot_img, kernel, "::/KERNEL"])
