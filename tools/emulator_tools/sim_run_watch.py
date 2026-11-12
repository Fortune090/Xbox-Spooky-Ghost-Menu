import sys, types, os, time as realtime
sys.path.insert(0, '/home/claude/h5/emulator/tools')
os.chdir('/tmp/simrun') if os.path.isdir('/tmp/simrun') else (os.makedirs('/tmp/simrun'), os.chdir('/tmp/simrun'))
import xefu_probe as xp, run_test as rt
clock = [1000.0]
rt.time.time = lambda: clock[0]
def fsleep(s): clock[0] += s
rt.time.sleep = fsleep
MODE = sys.argv[1]                       # 'dead' (stops answering at 15 s), 'ok', 'quit' (title gone at 70 s) or 'freeze' (frames stop at 70 s)
T0 = [None]
class FakeX:
    def __init__(self, ip=None, port=None, timeout=None):
        if T0[0] is not None and MODE == 'dead' and clock[0] - T0[0] > 15: clock[0] += 5; raise OSError('timed out')
    def cmd(self, c, *a, **k):
        if T0[0] is not None and MODE == 'dead' and clock[0] - T0[0] > 15: clock[0] += 5; raise IOError('timed out')
        if c == 'modules': return 200, '', (['name="Aurora.xex"'] if MODE == 'quit' and T0[0] is not None and clock[0] - T0[0] > 70 else ['name="xefu1_1.xex" base=0x82000000'])
        return 200, 'OK', []
    def getmem(self, a, n): raise IOError('no')
    def close(self): pass
    banner = "201- connected"
    def __getattr__(self, name):
        def f(*a, **k): raise IOError("fake: " + name)
        return f
xp.Xbdm = FakeX
n = [0]
def fake_sample(x):
    if T0[0] is None: T0[0] = clock[0]
    t = clock[0] - T0[0]
    if MODE == 'dead' and t > 15: clock[0] += 5; return {'region': None, 'cpu': None, 'frames': None}
    if MODE == 'quit' and t > 70: return {'region': 0x5F004F, 'cpu': 0x5F004F, 'frames': 0}
    if MODE != 'freeze' or t <= 70: n[0] += 1
    return {'region': 0xC4000000, 'cpu': 0xC4FF0690, 'frames': 100 + n[0] * 30, 'total': 0x7FFE, 'free': 0x4163 - (n[0] < 6) * n[0], 'pde800': 0x07FB0067, 'cur': 0xD0000798, 'queue': 0, 'req': 4}
rt.sample = fake_sample
stop_at = float(sys.argv[2]) if len(sys.argv) > 2 else None
if stop_at is not None:
    _fs = fake_sample
    def fs2(x):
        if T0[0] is not None and clock[0] - T0[0] > stop_at: raise KeyboardInterrupt
        return _fs(x)
    rt.sample = fs2
sys.argv = ['run_test.py', '--label', 'sim', '--minutes', '5']
try:
    rt.main()
except SystemExit:
    pass
