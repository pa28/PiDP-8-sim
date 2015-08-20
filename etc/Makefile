include *.mk

all: rc.pidp8

install: all
	$(INSTALL) $(INSTALL_OPTS) $(INSTALL_SOPTS) rc.pidp8 /etc/init.d
	/usr/sbin/update-rc.d rc.pidp8 defaults

dist: all
	cp rc.pidp8 ../dist/$(DIST)

clean:
	$(RM) rc.pidp8

rc.pidp8: rc.pidp8.in
	sed -e 's+__ROOT__+$(ROOT_DIR)+g' < $< > $@
