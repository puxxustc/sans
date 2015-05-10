#!/bin/bash

set -e

export CC=gcc
./configure --enable-debug --enable-coverage
make

src/sans -h || true
src/sans --version || true
test/test.py

cd src
rm -f *.html
gcov async_connect.c conf.c dns.c dname.c dnsmsg.c db.c global.c log.c main.c query.c sans.c socks5.c utils.c
gcovr -e 'ev|ns_name.c|ns_netint.c|ns_parse.c|res_comp.c|res_mkquery.c' -r . --html  --html-details  -o index.html
cd ..

COVERAGE=$(gcovr -e 'ev|ns_name.c|ns_netint.c|ns_parse.c|res_comp.c|res_mkquery.c' -r . | grep TOTAL | awk '{print $4}' | cut -c 1-2)
if [ ${COVERAGE} -lt 60 ]; then
	URL="https://img.shields.io/badge/coverage-${COVERAGE}%-red.svg?style=flat"
elif [ ${COVERAGE} -lt 80 ]; then
	URL="https://img.shields.io/badge/coverage-${COVERAGE}%-yellow.svg?style=flat"
else
	URL="https://img.shields.io/badge/coverage-${COVERAGE}%-green.svg?style=flat"
fi
curl -s -o .coverage.svg ${URL}

make distclean
rm -f src/*.gcov src/*.gcda src/*.gcno
