# Copyright 2017-present Facebook. All Rights Reserved.
FILESEXTRAPATHS_append := "${THISDIR}/files:"
SRC_URI += "file://interface/SoftwareJTAGHandler.c \
           "

LDFLAGS += " -lbic -lipmi -lipmb -lbic -lfby3_gpio -lgpio -lpal"
DEPENDS += "libbic libfby3-gpio libgpio libpal libipmi libipmb"
RDEPENDS_${PN} += "libbic libfby3-gpio libgpio libpal libipmi libipmb"
