#!/bin/sh
#
# Copyright 2019-present Facebook. All Rights Reserved.
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

PATH=/sbin:/bin:/usr/sbin:/usr/bin

# Provision Status
for i in {1..3}; do
  pfr=$(/usr/sbin/i2cget -y 4 0x58 0x0a b 2>/dev/null)
  if [ $? -eq 0 ]; then
    pfr=$((pfr & 0x20))
    break
  fi
  pfr=255
  sleep 0.5
done

if [ $pfr -eq 255 ]; then
  exit
fi

for i in {1..3}; do
  c0=$(/usr/sbin/i2cget -y 4 0x58 0xc0 b 2>/dev/null)
  [  $? -eq 0 ] && c0=$((c0 & 0xff))
  c1=$(/usr/sbin/i2cget -y 4 0x58 0xc1 b 2>/dev/null)
  [  $? -eq 0 ] && c1=$((c1 & 0xff))

  offs=$(devmem 0x1e72120c 2>/dev/null)
  start=$((offs & 0xff))
  end=$(((offs >> 8) & 0xff))

  sleep 1
  if [[ "$c0" == "$start" && "$c1" == "$end" ]]; then
    break
  fi
done

# CHKPT_COMPLETE
for i in {1..3}; do
  /usr/sbin/i2cset -y 4 0x58 0x0f 0x09 b > /dev/null 2>&1
  if [ $? -eq 0 ]; then
    break
  fi
  sleep 0.5
done
