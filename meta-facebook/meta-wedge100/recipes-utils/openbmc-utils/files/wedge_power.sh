#!/bin/bash
#
# Copyright 2015-present Facebook. All Rights Reserved.
#
# This program file is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation; version 2 of the License.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
# FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
# for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program in a file named COPYING; if not, write to the
# Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor,
# Boston, MA 02110-1301 USA
#

. /usr/local/bin/openbmc-utils.sh

PATH=/sbin:/bin:/usr/sbin:/usr/bin:/usr/local/bin

prog="$0"

PWD1014A_SYSFS_DIR=$(i2c_device_sysfs_abspath 2-003a)
PWR_SYSTEM_SYSFS="${SYSCPLD_SYSFS_DIR}/pwr_cyc_all_n"
PWR_USRV_RST_SYSFS="${SYSCPLD_SYSFS_DIR}/usrv_rst_n"
PWR_TH_RST_SYSFS="${SYSCPLD_SYSFS_DIR}/th_sys_rst_n"
ALT_SYSRESET_SYSFS="${PWD1014A_SYSFS_DIR}/mod_hard_powercycle"

usage() {
    echo "Usage: $prog <command> [command options]"
    echo
    echo "Commands:"
    echo "  status: Get the current power status"
    echo
    echo "  on: Power on microserver and main power if not powered on already"
    echo "    options:"
    echo "      -f: Re-do power on sequence no matter if microserver has "
    echo "          been powered on or not."
    echo
    echo "  off: Power off microserver and main power ungracefully"
    echo
    echo "  reset: Power reset microserver ungracefully"
    echo "    options:"
    echo "      -s: Power reset whole wedge system ungracefully"
    echo
}

is_main_power_on() {
  status=$(cat $PWR_MAIN_SYSFS | head -1 )
  if [ "$status" == "0x1" ]; then
      return 0            # powered on
  else
      return 1
  fi
}

do_status() {
    return_code=0

    echo -n "Microserver power is "
    if wedge_is_us_on; then
        echo "on"
    else
        echo "off"
        return_code=1
    fi

    echo -n "System main power is "
    if is_main_power_on; then
        echo "on"
    else
        echo "off"
        return_code=1
    fi

    return $return_code
}

do_on_com_e() {
    echo 1 > $PWR_USRV_SYSFS
}

do_on_main_pwr() {
    echo 1 > $PWR_MAIN_SYSFS
}

do_on() {
    local force opt ret
    ret=0
    force=0
    while getopts "f" opt; do
        case $opt in
            f)
                force=1
                ;;
            *)
                usage
                exit -1
                ;;

        esac
    done

    if [ $force -eq 0 ]; then
        # need to check if uS is on or not
        if wedge_is_us_on 10 "."; then
            echo "Skipping because microserver is already on. Use -f to force."
            return 1
        fi
    fi

    # reset TH
    reset_brcm.sh

    # power on sequence
    if ! is_main_power_on; then
      try_and_log "turning on main power" do_on_main_pwr || ret=1
    fi
    try_and_log "powering on microserver" do_on_com_e || ret=1

    return $ret
}

do_off_com_e() {
    echo 0 > $PWR_USRV_SYSFS
}

do_off_main_pwr() {
    echo 0 > $PWR_MAIN_SYSFS
}

do_off() {
    local ret
    ret=0

    try_and_log "powering off microserver" do_off_com_e || ret=1
    try_and_log "turning off main power" do_off_main_pwr || ret=1

    return $ret
}

do_reset() {
    local system opt pulse_us
    system=0
    while getopts "s" opt; do
        case $opt in
            s)
                system=1
                ;;
            *)
                usage
                exit -1
                ;;
        esac
    done
    if [ $system -eq 1 ]; then
        pulse_us=100000             # 100ms
        logger "Power reset the whole system ..."
        echo -n "Power reset the whole system ..."
        sleep 1
        echo 0 > $PWR_SYSTEM_SYSFS
        # Echo 0 above should work already. However, after CPLD upgrade,
        # We need to re-generate the pulse to make this work
        usleep $pulse_us
        echo 1 > $PWR_SYSTEM_SYSFS
        usleep $pulse_us
        echo 0 > $PWR_SYSTEM_SYSFS
        usleep $pulse_us
        echo 1 > $PWR_SYSTEM_SYSFS
        logger -s "wedge_power.sh reset -s through CPLD failed. The system will wait for 5 more seconds."
        logger -s "Then, it will reset Using pwr1014a instead."
        sleep 5
        echo 0 > $ALT_SYSRESET_SYSFS
        sleep 1
    else
        if ! wedge_is_us_on; then
            echo "Power resetting microserver that is powered off has no effect."
            echo "Use '$prog on' to power the microserver on"
            return -1
        fi
        # reset TH first
        reset_brcm.sh
        echo -n "Power reset microserver ..."
        echo 0 > $PWR_USRV_RST_SYSFS
        sleep 1
        echo 1 > $PWR_USRV_RST_SYSFS
        logger "Successfully power reset micro-server"
    fi
    echo " Done"
    return 0
}

if [ $# -lt 1 ]; then
    usage
    exit -1
fi

command="$1"
shift

case "$command" in
    status)
        do_status $@
        ;;
    on)
        do_on $@
        ;;
    off)
        do_off $@
        ;;
    reset)
        do_reset $@
        ;;
    *)
        usage
        exit -1
        ;;
esac
