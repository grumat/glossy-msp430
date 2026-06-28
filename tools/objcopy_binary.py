#!/usr/bin/env python3
"""Convert a binary file to an object file with controlled symbol names.

Runs arm-none-eabi-objcopy from a temporary working directory so that the
generated _binary_* symbols are based on a short, predictable filename
rather than an absolute path.

Usage:
    objcopy_binary.py <input.bin> <output.o> <symbol_filename>
"""

import os
import shutil
import subprocess
import sys
import tempfile


def main():
    if len(sys.argv) < 4:
        print('Usage: objcopy_binary.py <input.bin> <output.o> <symbol_filename>')
        sys.exit(1)

    input_bin = os.path.abspath(sys.argv[1])
    output_o = os.path.abspath(sys.argv[2])
    symbol_name = sys.argv[3]

    tmpdir = tempfile.mkdtemp(prefix='objcopy_')
    try:
        staged_bin = os.path.join(tmpdir, symbol_name)
        shutil.copy2(input_bin, staged_bin)

        subprocess.run(
            [
                'arm-none-eabi-objcopy',
                '-I', 'binary',
                '-O', 'elf32-littlearm',
                '-B', 'arm',
                symbol_name,
                output_o,
            ],
            cwd=tmpdir,
            check=True,
        )
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


if __name__ == '__main__':
    main()
