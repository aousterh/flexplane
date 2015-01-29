#!/usr/bin/python

from fastemu import *

class TestClassifier(PyClassifier):
    def classify(self, pkt_p):
        pkt = pkt_value(pkt_p)
        print "classify pkt=", pkt_p, "src", pkt.src, "dst", pkt.dst, "flow", pkt.flow 
        return 0,7

class TestQueueManager(PyQueueManager):
    def enqueue(self, pkt_p, port, queue):
        print "enqueue pkt", pkt_p, "port", port, "queue", queue

class TestScheduler(PyScheduler):
    def __init__(self):
        super(TestScheduler, self).__init__()
        self.mask = 1
        
    def schedule (self, port):
        print "schedule port", port
        return None
    
    def non_empty_port_mask(self):
        return self.mask

cla = TestClassifier()
qm = TestQueueManager()
sch = TestScheduler()

rtr = PyCompositeRouter(cla,qm,sch,1,0)

b = emu_packet()
b.src = 1
b.dst = 2
b.flow = 3

rtr.push(b)
c = pkt()
rtr.pull(0,c)