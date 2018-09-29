modprobe libcomposite

mkdir cfg
mount none cfg -t configfs

mkdir cfg/usb_gadget/g1
cd cfg/usb_gadget/g1

mkdir configs/c.1

mkdir functions/ffs.umtp
mkdir functions/acm.usb0

mkdir strings/0x409
mkdir configs/c.1/strings/0x409

echo 0x0100 > idProduct
echo 0x1D6B > idVendor

echo "01234567" > strings/0x409/serialnumber
echo "Viveris Technologies" > strings/0x409/manufacturer
echo "The Viveris Product !" > strings/0x409/product

echo "Conf 1" > configs/c.1/strings/0x409/configuration
echo 120 > configs/c.1/MaxPower

ln -s functions/acm.usb0 configs/c.1
ln -s functions/ffs.umtp configs/c.1

mkdir /dev/ffs-umtp
mount -t functionfs umtp /dev/ffs-umtp
#./umtprd

cd ../../..

#ls  /sys/class/udc/

echo 300000.gadget > cfg/usb_gadget/g1/UDC

