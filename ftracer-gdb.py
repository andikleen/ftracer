# dump ftracer from multiple threads interleaved from gdb
import gdb
import re
import collections

def getstr(v):
    v = "%s" % (v,)
    t = re.search('"(.+)"', v)
    if t:
        return t.group(1)
    return v

def resolve_with_off(v):
    x = gdb.execute("p/a %d" % (v,), False, True)
    m = re.search(r'<(.*)>', x)
    return m.group(1)

def resolve(v):
    v = resolve_with_off(v)
    m = re.match(r'(.*)\+.*', v)
    return m.group(1)

class Thr:
    def __init__(self):
        self.stack = []

    def update(self, rsp):
        if rsp == 0 or not self.stack or rsp < self.stack[-1]:
            self.stack.append(rsp)
        else:
            while self.stack and rsp > self.stack[-1]:
                self.stack.pop()

    def level(self):
        return len(self.stack) - 1

class Ftracer (gdb.Command): 
    def __init__(self):
        super (Ftracer, self).__init__("ftracer", gdb.COMMAND_NONE, gdb.COMPLETE_SYMBOL)

    def invoke(self, arg, from_tty):
        args = arg.split()
        max = 0
        if len(args) >= 1:
            max = int(args[0])
        events = collections.defaultdict(list)
        threads = {}
        frequency = float(gdb.selected_frame().read_var("ftracer_frequency"))
        for i in gdb.inferiors():
            for t in i.threads():
                gdb.execute("thread %d" % (t.num,))
                ftracer_len = gdb.selected_frame().read_var("ftracer_size") 
                x = gdb.selected_frame().read_var("ftracer_tbuf")
                for j in range(0, ftracer_len-1):
                    v = x[j]
                    tstamp = int(v["tstamp"])
                    if tstamp:
                        o = (t.num, v["func"], v["arg1"], v["arg2"], v["arg3"], v["rsp"])
                        events[tstamp].append(o)
                    else:
                        break
        print ("%6s %6s %3s %-25s %s" %
                ("TIME", "DELTA", "THR", "FUNC", "ARGS"))
        prev = 0
        delta = 0
        start = 0
        threads = collections.defaultdict(Thr)
        k = sorted(events.keys())
        if max:
            k = collections.deque(k, max)
        for t in k:
            if prev:
                delta = t - prev
            if start == 0:
                start = t
            for e in events[t]:
                thr = threads[e[0]]
                thr.update(int(e[5]))
                func = " " * (thr.level() * 2) + resolve(int(e[1]))
                print "%6.2f %6.2f %3d %-25s %d %d %d" % (
                        (t - start) / frequency,
                        delta / frequency,
                        e[0],
                        func,
                        int(e[2]), int(e[3]), int(e[4]))
            prev = t


Ftracer()
