include *.mk

dist: 
	sed -e 's+__ROOT__+$(ROOT_DIR)+g' < install.sh > $(DIST)/install.sh
	tar -zcvf $(DIST).tgz $(DIST)

clean:
	$(RM) -r $(DIST)
	$(RM) $(DIST).tgz
