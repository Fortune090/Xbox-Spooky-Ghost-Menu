#!/usr/bin/env python3
"""Check XEX2 files: header hash and page-hash chain (formulas in xexrepack.py). usage: xexhash.py <file.xex> ..."""
import os
import sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from xexrepack import verify  # noqa: E402
for f in sys.argv[1:]:
    print(os.path.basename(f), verify(f))
