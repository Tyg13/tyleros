#!/usr/bin/env python3
import argparse
import os
import shutil
import subprocess
import sys


def checked_run(*args, **kwargs):
    kwargs["check"] = True
    try:
        subprocess.run(*args, **kwargs)
    except subprocess.CalledProcessError:
        print("`{}`: failed".format(' '.join(*args)), file=sys.stderr)
        sys.exit(1)


def rmdir_if_exists(dir_: str):
    try:
        shutil.rmtree(dir_)
    except FileNotFoundError:
        pass


def build(args):
    if args.clean:
        rmdir_if_exists("build")
        rmdir_if_exists("bin")
        rmdir_if_exists("sysroot")

    os.makedirs("build", exist_ok=True)

    checked_run([
        "cmake",
        "-B", "build",
        "-G", "Ninja",
        "-DCMAKE_EXPORT_COMPILE_COMMANDS=1",
        "-DCMAKE_TOOLCHAIN_FILE=toolchain/x86_64.cmake",
        "-DCMAKE_BUILD_TYPE=" + args.buildtype,
    ])
    checked_run(["cmake", "--build", "build"] +
                (["--", args.target] if args.target and not args.test else []))

    if args.test:
        os.makedirs("build/unit-tests", exist_ok=True)
        checked_run([
            "cmake",
            "-S", "tests",
            "-B", "build/unit-tests",
            "-G", "Ninja",
            "-DCMAKE_EXPORT_COMPILE_COMMANDS=1",
            "-DCMAKE_BUILD_TYPE=" + args.buildtype,
        ])
        checked_run(["cmake", "--build", "build/unit-tests"])
        checked_run(["build/unit-tests/test_harness"] +
                    ([f"--gtest_filter={args.target}"] if args.target and args.test else []))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--buildtype", default="Debug")
    parser.add_argument("--clean", action="store_true")
    parser.add_argument("--test", action="store_true")
    parser.add_argument("target", nargs="?")
    build(parser.parse_args())


if __name__ == "__main__":
    main()
