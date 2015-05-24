#!/usr/bin/env python3

import sys
import os
import signal
import time
from subprocess import Popen


if len(sys.argv) != 1:
    print('usage: test.py <config>')
    sys.exit(0)


sans = Popen(['src/sans', '-v', '-c', sys.argv[0]], shell=False, bufsize=0, close_fds=True)
time.sleep(1)

cmds = ['dig @127.0.0.1 -p 5300 twitter.com',
        'dig @127.0.0.1 -p 5300 www.meituan.com',
        'dig @127.0.0.1 -p 5300 -x 8.8.8.8',
        'dig @127.0.0.1 -p 5300 +tcp twitter.com',
        'dig @127.0.0.1 -p 5300 +tcp www.meituan.com']

for cmd in cmds:
    p = Popen(cmd.split(), shell=False, bufsize=0, close_fds=True)
    if p:
        r = p.wait()
        if r == 0:
            print('test passed')
    time.sleep(1)

for p in [sans]:
    try:
        os.kill(p.pid, signal.SIGINT)
        os.waitpid(p.pid, 0)
    except OSError:
        pass
