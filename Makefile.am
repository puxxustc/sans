ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src

EXTRA_DIST = man/sans.8 contrib/systemd/sans.service \
             contrib/sample/sans.conf

man_MANS = man/sans.8

systemddir=$(libdir)/systemd/system
systemd_DATA=contrib/systemd/sans.service

sysconf_DATA=contrib/sample/sans.conf
