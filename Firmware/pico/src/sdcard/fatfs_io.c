// #include <stdint.h>
// #include "ff.h"
// #include "diskio.h"
// #include "sdcard/sdcard.h"

// #define DEV_RAM		0	/* Example: Map Ramdisk to physical drive 0 */
// #define DEV_MMC		1	/* Example: Map MMC/SD card to physical drive 1 */
// #define DEV_USB		2	/* Example: Map USB MSD to physical drive 2 */

// DSTATUS disk_initialize(BYTE pdrv) {
// 	switch (pdrv) {
// 	case DEV_RAM:
// 		return STA_NODISK;
// 	case DEV_MMC:
//         if (sdcard_init()) {
//             return 0;
//         }
// 		break;
// 	case DEV_USB:
// 		return STA_NODISK;
// 	}
// 	return STA_NOINIT;
// }

// DSTATUS disk_status(BYTE pdrv) {
// 	DSTATUS stat;
// 	int result;

// 	switch (pdrv) {
// 	case DEV_RAM :
// 		result = RAM_disk_status();

// 		// translate the reslut code here

// 		return stat;

// 	case DEV_MMC :
// 		result = MMC_disk_status();

// 		// translate the reslut code here

// 		return stat;

// 	case DEV_USB :
// 		result = USB_disk_status();

// 		// translate the reslut code here

// 		return stat;
// 	}
// 	return STA_NOINIT;
// }