#!/usr/bin/env python3
"""make_bundle.py - packs run_test.py, hang_probe.py, boot_trace.py, xefu_probe.py, spooky_live.py and xex.py into one
self-contained h2test.py. The modules are embedded as source and served to `import` by a small in-memory finder, so
they keep importing each other by their usual names. Usage of the result:

  python h2test.py run   --label r48 [--xex xefu1_1.xex] [--boot "HDD:\\Games\\Halo 2\\default.xbe"]   (the usual one-run test)
  python h2test.py probe --label r48          (hang_probe.py)
  python h2test.py trace --label r48          (boot_trace.py)
  python h2test.py xefu  --mem128 --label r48 (xefu_probe.py)
  python h2test.py live  192.168.0.178 ...    (spooky_live.py)
  python h2test.py run ...  is also what a bare  python h2test.py ...  means.
"""
import sys, os, base64, zlib, datetime

MODULES = ['xefu_probe', 'hang_probe', 'boot_trace', 'run_test', 'spooky_live', 'xex', 'pages', 'hashmaps']
here = os.path.dirname(os.path.abspath(__file__))
blob = {}
for m in MODULES:
    blob[m] = open(os.path.join(here, m + '.py'), 'rb').read()

import pickle
packed = base64.b64encode(zlib.compress(pickle.dumps(blob), 9)).decode('ascii')
stamp = datetime.datetime.now().strftime('%Y-%m-%d %H:%M')

out = '''#!/usr/bin/env python3
"""h2test - the Halo 2 / xefu 360-emulator test tools in one file (built %s).

  python h2test.py run   --label r48 [--xex xefu1_1.xex] [--boot "HDD:\\\\Games\\\\Halo 2\\\\default.xbe"]
        one run: send the xbe / emulator, boot the game, watch, follow the mod's log, verdict, hang probe - never a reboot (run_test)
  python h2test.py probe --label r48            the hang probe on a stalled game (hang_probe)
  python h2test.py trace --label r48            the boot tracer (boot_trace)
  python h2test.py xefu  --mem128 --label r48   the emulator probe (xefu_probe)
  python h2test.py pages --label r48            the page map of the 128 MB while the game runs (pages)
  python h2test.py hashmaps --label r48 [--baseline]   SHA-1 of the game's map files over FTP against a baseline (hashmaps)
  python h2test.py live  192.168.0.178 ...      spooky_live (deploy / find-xbe / logs)
  A bare  python h2test.py --label ...  means  run.
Default console IP 192.168.0.178. Only the standard library is needed; pycryptodome only for comparing a local
xex with the one on the console (optional).
"""
import sys, os, base64, zlib, pickle, importlib.abc, importlib.util

_SRC = pickle.loads(zlib.decompress(base64.b64decode(%r)))


class _Finder(importlib.abc.MetaPathFinder, importlib.abc.Loader):
    def find_spec(self, name, path=None, target=None):
        if name in _SRC:
            return importlib.util.spec_from_loader(name, self, origin='h2test:' + name)
        return None

    def create_module(self, spec):
        return None

    def exec_module(self, module):
        module.__file__ = os.path.abspath(sys.argv[0])
        exec(compile(_SRC[module.__name__], 'h2test:' + module.__name__ + '.py', 'exec'), module.__dict__)


sys.meta_path.insert(0, _Finder())

TOOLS = {'run': 'run_test', 'probe': 'hang_probe', 'trace': 'boot_trace', 'xefu': 'xefu_probe', 'live': 'spooky_live', 'pages': 'pages', 'hashmaps': 'hashmaps'}


def main():
    args = sys.argv[1:]
    tool = 'run'
    if args and args[0] in TOOLS:
        tool = args.pop(0)
    elif args and args[0] in ('-h', '--help') and len(args) == 1:
        print(__doc__)
        return
    sys.argv = ['%%s (%%s)' %% (os.path.basename(sys.argv[0]), tool)] + args
    mod = importlib.import_module(TOOLS[tool])
    if tool == 'live':
        sys.exit(mod.main(sys.argv))
    mod.main()


if __name__ == '__main__':
    main()
''' % (stamp, packed)
open(os.path.join(here, 'h2test.py'), 'w').write(out)
print('h2test.py: %d bytes, modules %s' % (len(out), ', '.join('%s %d' % (m, len(blob[m])) for m in MODULES)))
