# sans #

[![Release](https://img.shields.io/github/release/XiaoxiaoPu/sans.svg?style=flat)](https://github.com/XiaoxiaoPu/sans/releases/latest)
[![License](https://img.shields.io/badge/license-GPL%203-blue.svg?style=flat)](http://www.gnu.org/licenses/gpl.html)
[![Build Status](https://travis-ci.org/XiaoxiaoPu/sans.svg?branch=master)](https://travis-ci.org/XiaoxiaoPu/sans)
[![Build Status](https://ci.xiaoxiao.im/buildStatus/icon?job=sans)](https://ci.xiaoxiao.im/job/sans)

**S**imple **A**nti-spoofing **N**ame **S**erver, designed to defend against DNS spoofing, suitable for embedded devices and low end boxes.

## Features ##

1. Support both UDP and TCP
2. Detect if a domain is polluted
3. Query polluted domains over TCP or SOCKS5

## Defects ##

1. No cache (for now)
2. Do not retry if error occurs

## Usage ##

```bash
usage: sans
  -h, --help           show this help
  -c, --config <file>  config file
  -d, --daemon         daemonize after initialization
  --pidfile <file>     PID file, default: /run/sans.pid
  --logfile <file>     log file, default: /var/log/sans.log
  -v, --verbose        verbose logging
  -V, --version        print version and exit

Bug report: <https://github.com/XiaoxiaoPu/sans/issues>.
```

## Configuration ##

key         | description
:----------:|:----------
user        | User to set privilege to, default: nobody
listen      | Listern address and port, default: 127.0.0.1:53
socks5      | SOCKS5 server
test_server | DNS server for testing if a domain is polluted, default: 8.8.8.8:53
cn_server   | DNS server for unpolluted domains, default: 114.114.114.114:53
server      | DNS server for polluted domains, default: 8.8.8.8:53

**sample config file:**

```ini
user=nobody
group=nobody
listen=127.0.0.1:5300
socks5=127.0.0.1:1080
test_server=8.8.8.8:53
cn_server=114.114.114.114:53
server=8.8.8.8:53
```

## Note ##

1. If SOCKS5 server is not given, polluted domains will be queried over TCP. It's faster than querying over SOCKS5, but may not work in some networks.

2. Since there is no cache in sans, you may need to set it as an upstream DNS server for Dnsmasq instead of using it directly.

## Build ##

1. configure and make

	```bash
	autoreconf -if
	./configure --prefix=/usr --sysconfdir=/etc
	make
	```

2. install

	```bash
	sudo make install
	```

## Cross compile ##

1. setup cross compile tool chain

2. build

	```bash
	autoreconf -if
	./configure --host=arm-unknown-linux-gnueabihf \
	    --prefix=/usr --sysconfdir=/etc
	make
	```

## Build with MinGW ##

```bash
autoreconf -if
./configure --host=i686-w64-mingw32
make
```

## TODO ##

*	cache
*	auto pre-query
*	recursive

## License ##

Copyright (C) 2014 - 2015, Xiaoxiao <i@xiaoxiao.im>

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
