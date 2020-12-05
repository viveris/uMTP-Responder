#define _LARGEFILE64_SOURCE
#define _FILE_OFFSET_BITS 64

//#define CONFIG_USB_NON_BLOCKING_WRITE 1

#define CONFIG_USB_FS_SUPPORT 1      // USB 1.1 Full speed
#define CONFIG_USB_HS_SUPPORT 1      // USB 2.0 High speed
//#define CONFIG_USB_SS_SUPPORT 1    // USB 3.0 SuperSpeed

#define CONFIG_READ_FILE_BUFFER_SIZE  (1024*1024) // Must be a 2^x value.
#define CONFIG_MAX_TX_USB_BUFFER_SIZE (16*512)    // Must be a multiple of 512 and be less than CONFIG_READ_FILE_BUFFER_SIZE
#define CONFIG_MAX_RX_USB_BUFFER_SIZE (16*512)    // Must be a multiple of 512

#include "custom_buildconf.h"
