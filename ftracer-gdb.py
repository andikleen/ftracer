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

class Ftracer (gdb.Command): 
    def __init__(self):
        super (Ftracer, self).__init__("ftracer", gdb.COMMAND_NONE, gdb.COMPLETE_SYMBOL)

    def invoke(self, arg, from_tty):
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
                        o = (t.num, v["src"], v["dst"], v["arg1"], v["arg2"], v["arg3"])
                        events[tstamp].append(o)
                    else:
                        break
        print ("%6s %6s  %3s  %-20s    %-10s %s" %
                ("TIME", "DELTA", "THR", "CALLER", "CALLEE", "ARGS"))
        prev = 0
        delta = 0
        start = 0
        for t in sorted(events.keys()):
            if prev:
                delta = t - prev
            if start == 0:
                start = t
            for e in events[t]:
                print "%6.2f %6.2f " % ((t - start) / frequency, delta / frequency,),
                print "%3d " % (e[0],),
                src = " " * ((e[0] - 1) * 16) + "%-10s" % (resolve_with_off(int(e[1])),)
                print "%-20s -> %-10s %d %d %d" % (src, resolve(int(e[2])), int(e[3]), int(e[4]), int(e[5]))
            prev = t


Ftracer()
