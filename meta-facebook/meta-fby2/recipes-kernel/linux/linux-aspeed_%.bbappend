LINUX_VERSION_EXTENSION = "-fby2"

COMPATIBLE_MACHINE = "fby2|fbnd"

FILESEXTRAPATHS_prepend := "${THISDIR}/files:"

SRC_URI += "file://defconfig \
           "

KERNEL_MODULE_AUTOLOAD += " \
"
