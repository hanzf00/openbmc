#!/bin/sh

# TODO:
# 	Remove this workwround at DVT stage

echo "Overwrite the register 0x1 of AUX HSC with 0x2"
i2cset -f -y 5 0x70 0x4
i2cset -f -y 5 0x43 0x1 0x2
