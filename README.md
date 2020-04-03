![uMTP-responder logo](https://raw.githubusercontent.com/viveris/uMTP-Responder/master/img/umtp-128h.png
)

# uMTP-Responder

## Lightweight USB Media Transfer Protocol (MTP) responder daemon for GNU/Linux

The uMTP-Responder allows files to be transferred to and from devices through the devices USB port.

### Main characteristics and features

- Implemented in C.

- Lightweight implementation.

- User space implementation.

- As few dependencies as possible.

- Hook to the FunctionFS/libcomposite or the GadgetFS Linux layer.

- Dynamic handles allocation (No file-system pre-scan).

- Unicode support.

- (Optional) Syslog support.

## Current status

### What is working

- Folder listing.

- Folder creation.

- Files & Folders upload.

- Files & Folders download.

- Files & Folders deletion.

- Files & Folders renaming.

- Files / folders changes async events.

- Up to 16 storage instances supported.

- Storages mount / unmount.

- GadgetFS and FunctionFS/libcomposite modes supported.

## Which platforms are supported ?

Any board with a USB device port should be compatible. The only requirement is to have the USB FunctionFS (CONFIG_USB_FUNCTIONFS) or GadgetFS (CONFIG_USB_GADGETFS) support enabled in your Linux kernel.
You also need to enable the board-specific USB device port driver (eg. dwc2 for the RaspberryPi Zero).

uMTP-Responder is currently tested with various 4.x.x Linux kernel versions.
This may work with earlier kernels (v3.x.x and some v2.6.x versions) but without any guarantee.

### Successfully tested boards

- Atmel Sama5D2 Xplained.

- Raspberry PI Zero (W).

- BeagleBone Black.

- Allwinner SoC based board.

- Freescale i.MX6 SabreSD. (Kernel v4.14)

- Samsung Artik710. (FunctionFS mode)

### Successfully tested client operating systems

- Windows 7, Windows 10, Linux, Android.

## How to build it ?

A simple "make" should be enough if you build uMTPrd directly on the target.

If you are using a cross-compile environment, set the "CC" variable to your GCC cross compiler.

You can also enable the syslog support with the C flag "USE_SYSLOG" and the verbose/debug output with the "DEBUG" C flag.

examples:

On a cross-compile environment :

```c
make CC=armv6j-hardfloat-linux-gnueabi-gcc
```

On a cross-compile environment with both syslog support and debug output options enabled :

```c
make CC=armv6j-hardfloat-linux-gnueabi-gcc CFLAGS="-DUSE_SYSLOG -DDEBUG"
```

Note: syslog support and debug output options can be enabled separately.

(replace "armv6j-hardfloat-linux-gnueabi-gcc" with your target gcc cross-compiler)

If you want to use it on a Kernel version < 3.15 you need to compile uMTPrd with old-style FunctionFS descriptors support:

```c
make CC=armv6j-hardfloat-linux-gnueabi-gcc CFLAGS="-DOLD_FUNCTIONFS_DESCRIPTORS"
```

## How to set it up ?

A config file should copied into the folder /etc/umtprd/umtprd.conf
This file defines the storage entries (host path and name), the MTP device name, the USB vendor & product IDs and the USB device configuration.
Check the file [umtprd.conf](conf/umtprd.conf) file for details on available options.

## How to launch it ?

Once you have configured the correct settings in umtprd.conf, you can use umtprd_ffs.sh or umtprd_gfs.sh to launch it in FunctionFS/GadgetFS mode or use udev to launch the deamon when the usb device port is connected.

## License

This project is licensed under the GNU General Public License version 3 - see the [LICENSE](LICENSE) file for details
