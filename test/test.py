#!/usr/bin/env python3

import sys
import os
import signal
import time
from subprocess import Popen

sans = ['src/sans', '-v', '-c', 'test/test1.conf']
p1 = Popen(sans, shell=False, bufsize=0, close_fds=True)
time.sleep(1)

cmds = ['dig @127.0.0.1 -p 5300 twitter.com',
        'dig @127.0.0.1 -p 5300 www.taobao.com',
        'dig @127.0.0.1 -p 5300 -x 8.8.8.8',
        'dig @127.0.0.1 -p 5300 +tcp twitter.com',
        'dig @127.0.0.1 -p 5300 +tcp www.taobao.com']

for cmd in cmds:
	p2 = Popen(cmd.split(), shell=False, bufsize=0, close_fds=True)
	if p2 is not None:
		r = p2.wait()
	if r == 0:
		print('test passed')
	time.sleep(1)

for p in [p1]:
	try:
		os.kill(p.pid, signal.SIGINT)
		os.waitpid(p.pid, 0)
	except OSError:
		pass

sans = ['src/sans', '-v', '-c', 'test/test2.conf']
p1 = Popen(sans, shell=False, bufsize=0, close_fds=True)
time.sleep(1)

cmds = ['dig @127.0.0.1 -p 5300 twitter.com',
        'dig @127.0.0.1 -p 5300 www.taobao.com',
        'dig @127.0.0.1 -p 5300 -x 8.8.8.8',
        'dig @127.0.0.1 -p 5300 +tcp twitter.com',
        'dig @127.0.0.1 -p 5300 +tcp www.taobao.com']

for cmd in cmds:
	p2 = Popen(cmd.split(), shell=False, bufsize=0, close_fds=True)
	if p2 is not None:
		r = p2.wait()
	if r == 0:
		print('test passed')
	time.sleep(1)

for p in [p1]:
	try:
		os.kill(p.pid, signal.SIGINT)
		os.waitpid(p.pid, 0)
	except OSError:
		pass

sys.exit(0)
