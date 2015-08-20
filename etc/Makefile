include *.mk

install: $(BINS)
	$(INSTALL) $(INSTALL_OPTS) $(INSTALL_SOPTS) rc.pidp8 /etc/init.d
	/usr/sbin/update-rc.d rc.pidp8 defaults
