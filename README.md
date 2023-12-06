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

- Storages lock / unlock.

- Global and Storages GID/UID override options.

- GadgetFS and FunctionFS/libcomposite modes supported.

## Which platforms are supported ?

Any board with a USB device port should be compatible. The only requirement is to have the USB FunctionFS (CONFIG_USB_FUNCTIONFS) or GadgetFS (CONFIG_USB_GADGETFS) support enabled in your Linux kernel.
You also need to enable the board-specific USB device port driver (eg. dwc2 for the RaspberryPi Zero).

uMTP-Responder was tested on various Linux kernel versions (4.x.x / 5.x.x / 6.x.x ...) .
This may work with earlier kernels (v3.x.x and some v2.6.x versions) but without any guarantee.

### Successfully tested boards

- Atmel Sama5D2 Xplained.

- Raspberry PI Zero (W).

- BeagleBone Black.

- Allwinner SoC based board.

- Freescale i.MX6 SabreSD. (Kernel v4.14)

- Samsung Artik710. (FunctionFS mode)

- Ultra96-V2 (Zynq UltraScale+, FunctionFS mode, SuperSpeed USB)

### Successfully tested client operating systems

- Windows 7, Windows 10, Windows 11, Linux, Android.

## How to build it ?

The project is build using cmake.
If you are compiling on the target, the compilation steps are:
```c
mkdir build
cd build
cmake ..
make -j8
```

If you are using a cross-compile environment, you can use the CMAKE_TOOLCHAIN_FILE variable to specify your toolchain:
```c
mkdir build
cd build
cmake -DCMAKE_TOOLCHAIN_FILE=<your_toolchain.cmake> ..
make -j8
```

A few compilation options are available:
- USE_SYSTEMD (default: ON). If systemd is available in the SDK, umtprd will notify systemd when it is ready.
- USE_SYSLOG (default: ON). Use syslog for the log messages
- OLD_FUNCTIONFS_DESCRIPTORS (default: OFF). Use it if you kernel version is < 3.15 (old-style FunctionFS descriptors support)
- SUPER_SPEED (default: OFF). Support for USB Super-Speed
- HIGH_SPEED (default: ON). Support for USB High-Speed
- FULL_SPEED (default: ON). Support for USB Full-Speed


## How to set it up ?

A config file should copied into the folder /etc/umtprd/umtprd.conf
This file defines the storage entries (host path and name), the MTP device name, the USB vendor & product IDs and the USB device configuration.
Check the file [umtprd.conf](conf/umtprd.conf) file for details on available options.

## How to launch it ?

Once you have configured the correct settings in umtprd.conf, you can use umtprd_ffs.sh or umtprd_gfs.sh to launch it in FunctionFS/GadgetFS mode or use udev to launch the deamon when the usb device port is connected.

## Runtime operations

uMTP-Responder supports dynamic commands to add/mount/umount/remove storage and lock/unlock storage.

Examples:

Unlock all locked storage (set with the 'locked' option in the configuration file) :

```c
umtprd -cmd:unlock
```

Lock all lockable storage (set with the 'locked' option in the configuration file) :

```c
umtprd -cmd:lock
```

"addstorage"/"rmstorage" commands to dynamically add/remove storage :

```c
umtprd '-cmd:addstorage:/tmp Tmp rw'
```

```c
umtprd '-cmd:rmstorage:Tmp'
```

Use double-quotes when arguments have spaces in them:

```c
umtprd '-cmd:addstorage:/path "My Path" rw,removable'
```

```c
umtprd '-cmd:rmstorage:"My Path"'
```

"mount"/"unmount" commands to dynamically mount/unmount storage.

```c
umtprd '-cmd:mount:"Storage name"'
```

```c
umtprd '-cmd:unmount:"Storage name"'
```

## License

This project is licensed under the GNU General Public License version 3 - see the [LICENSE](LICENSE) file for details
