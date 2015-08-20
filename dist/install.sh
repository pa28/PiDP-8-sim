#!/bin/bash

ROOT_DIR="__ROOT__"
BIN_DIR="${ROOT_DIR}/bin"
ETC_DIR="${ROOT_DIR}/etc"
INSTALL="/usr/bin/install"
INSTALL_OPTS="-o root -g root"
INSTALL_DOPTS="-m 0755"
INSTALL_XOPTS="-s -m 0755"
INSTALL_SOPTS="-m 0755"

${INSTALL} ${INSTALL_OPTS} ${INSTALL_DOPTS} -d ${BIN_DIR} ${ETC_DIR}
${INSTALL} ${INSTALL_OPTS} ${INSTALL_XOPTS} bin/* ${BIN_DIR}
${INSTALL} ${INSTALL_OPTS} ${INSTALL_SOPTS} rc.pidp8 /etc/init.d
${INSTALL} ${INSTALL_OPTS} ${INSTALL_FOPTS} etc/* $(ETC_DIR)
${INSTALL} --owner=bin --group=bin --mode=0755 palbart /usr/local/bin
${INSTALL} --owner=root --group=root --mode=0644 palbart.1.gz /usr/share/man/man1
/usr/sbin/update-rc.d rc.pidp8 defaults
