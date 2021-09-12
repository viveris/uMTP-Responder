#!/bin/sh

# GadgetFS uMTPrd startup example/test script

# Raspberry Pi : start dwc2 & gadgetfs

modprobe dwc2
modprobe gadgetfs

# Mount gadgetfs

mkdir /dev/gadget
mount -t gadgetfs gadgetfs /dev/gadget

# Start umtprd
while [ test ]; do
  /usr/bin/umtprd
done
