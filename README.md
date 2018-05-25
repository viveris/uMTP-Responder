![uMTP-responder logo](https://raw.githubusercontent.com/viveris/uMTP-Responder/master/img/umtp-128h.png
)

# uMTP-Responder

## Ligthweight Media Transfer Protocol (MTP) responder daemon for GNU/Linux

The uMTP-Responder allows files to be transferred to and from devices through the devices USB port.

### Main characteristics and features

- Implemented in C.

- Lightweight implementation.

- User space implementation.

- As few dependencies as possible.

- Hook to the Gadget FS Linux layer.

- Dynamic handles allocation (No file-system pre-scan).

- (Optional) Syslog support.

## Current status

### What is working

- Folder listing.

- Folder creation.

- Files & Folders upload.

- Files & Folders download.

- Files & Folders deletion.

- Up to 16 storage instances supported.

### What is planned

- libcomposite support.

## Which platforms are supported ?

Any board with a USB device port should be compatible. The only requirement is to have the USB Gadget FS support enabled in your Linux kernel.

### Boards successfully tested

- Atmel Sama5D2 Xplained.

- Raspberry PI Zero (W).

### Client operating systems successfully tested

- Windows 7, Windows 10, Linux.

## How to build it ?

A simple "make" should be enough. If you are using a cross-compile environment, set the "CC" variable to your GCC cross compiler.
 
## How to set it up ?

A config file should copied into the folder /etc/umtprd/umtprd.conf
This file defines the storage entries (host path and name), the MTP device name, the USB vendor & product IDs and the USB device configuration.
Check the file [umtprd.conf](conf/umtprd.conf) file for details on available options.

## How to launch it ?

Once you have configured the correct settings in umtprd.conf, you can use umtprd.sh to launch it or use udev to launch the deamon when the usb device port is connected.

## License

This project is licensed under the GNU General Public License version 3 - see the [LICENSE](LICENSE) file for details

