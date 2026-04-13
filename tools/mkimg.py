import argparse
import math
import os


def main() -> int:
    parser = argparse.ArgumentParser(description="Create a raw floppy image from boot+kernel.")
    parser.add_argument("--boot", required=True, help="Path to 512-byte boot sector binary")
    parser.add_argument("--kernel", required=True, help="Path to kernel raw binary")
    parser.add_argument("--out", required=True, help="Output image path")
    parser.add_argument("--size", type=int, default=1474560, help="Image size in bytes (default 1.44MB)")
    args = parser.parse_args()

    with open(args.boot, "rb") as f:
        boot = f.read()
    if len(boot) != 512:
        raise SystemExit(f"boot sector must be exactly 512 bytes, got {len(boot)} bytes")

    with open(args.kernel, "rb") as f:
        kernel = f.read()

    sectors = int(math.ceil(len(kernel) / 512.0))
    padded_len = sectors * 512
    kernel_padded = kernel + (b"\x00" * (padded_len - len(kernel)))

    os.makedirs(os.path.dirname(os.path.abspath(args.out)) or ".", exist_ok=True)
    with open(args.out, "wb") as f:
        f.truncate(args.size)
        f.seek(0)
        f.write(boot)
        f.seek(512)
        f.write(kernel_padded)

    print(f"Wrote {args.out} (kernel: {len(kernel)} bytes, {sectors} sectors)")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

