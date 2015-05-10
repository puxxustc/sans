AM_CFLAGS = -pipe -W -Wall -fno-strict-aliasing

bin_PROGRAMS = sans

sans_SOURCES = \
    async_connect.c conf.c dns.c dname.c dnsmsg.c db.c global.c log.c main.c query.c sans.c socks5.c utils.c \
    async_connect.h conf.h dns.h dname.h dnsmsg.h db.h global.h log.h query.h sans.h socks5.h utils.h win.h
sans_LDADD = 

EXTRA_sans_SOURCES = ev.c ev.h ev_vars.h ev_wrap.h ev_epoll.c ev_select.c ev_poll.c ev_kqueue.c ev_port.c ev_win32.c resolv.c resolv.h

if BUILD_RESOLV
sans_SOURCES += resolv.c resolv.h
else
sans_LDADD += $(LIB_RESOLV)
endif

if BUILD_EV
sans_SOURCES += ev.c ev.h
else
sans_LDADD += $(LIB_EV)
endif
