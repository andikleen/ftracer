# dump ftracer from multiple threads interleaved from gdb
import gdb
import re

def getstr(v):
    v = "%s" % (v,)
    t = re.search('"(.+)"', v)
    if t:
        return t.group(1)
    return v

class Ftracer (gdb.Command): 
    def __init__(self):
        super (Ftracer, self).__init__("ftracer", gdb.COMMAND_NONE, gdb.COMPLETE_SYMBOL)

    def invoke(self, arg, from_tty):
        events = {}
        threads = {}
        for i in gdb.inferiors():
            for t in i.threads():
                gdb.execute("thread %d" % (t.num,))
                ftracer_len = gdb.selected_frame().read_var("ftracer_size") 
                x = gdb.selected_frame().read_var("ftracer_tbuf")
                for j in range(0, ftracer_len-1):
                    v = x[j]
                    #print v
                    #print t.num
                    tstamp = int(v["tstamp"])
                    if tstamp:
                        o = (t.num, v["src"], v["dst"], v["arg1"], v["arg2"], v["arg3"])
                        #o = (t.num, v["src"], v["dst"])
                        if tstamp in events:
                            events[tstamp].append(o)
                        else:
                            events[tstamp] = (o)
                    else:
                        break
        print "collected"
        prev = 0
        delta = 0
        for t in sorted(events.keys()):
            if prev:
                delta = t - prev
            print t,": "
            for e in events[t]:
                print e
                print "%10d" % (delta,),
                print " " * ((e[0] - 1) * 16),
                #print "%s:%d:%s:%d" % (getstr(e[1]), int(e[2]), getstr(e[3]), e[4])
            prev = t


Ftracer()
