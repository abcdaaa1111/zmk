/*
 * CherryUSB configuration for Zephyr
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef CHERRYUSB_CONFIG_H
#define CHERRYUSB_CONFIG_H

/* ================ USB common Configuration ================ */

#include <zephyr/kernel.h>

#define CONFIG_USB_PRINTF(...) printk(__VA_ARGS__)

#ifndef CONFIG_USB_DBG_LEVEL
#define CONFIG_USB_DBG_LEVEL 3
#endif

/* Enable print with color */
// #define CONFIG_USB_PRINTF_COLOR_ENABLE

/* data align size when use dma */
#define CONFIG_USB_ALIGN_SIZE 4

/* attribute data into no cache ram */
#define USB_NOCACHE_RAM_SECTION 

/* ================= USB Device Stack Configuration ================ */

#ifndef CONFIG_USBDEV_MAX_BUS
#define CONFIG_USBDEV_MAX_BUS 1
#endif

/* Ep0 in and out transfer buffer */
#ifndef CONFIG_USBDEV_REQUEST_BUFFER_LEN
#define CONFIG_USBDEV_REQUEST_BUFFER_LEN 512
#endif

/* enable advance desc register api */
#define CONFIG_USBDEV_ADVANCE_DESC

#ifndef CONFIG_USBDEV_MSC_MAX_LUN
#define CONFIG_USBDEV_MSC_MAX_LUN 1
#endif

#ifndef CONFIG_USBDEV_MSC_MAX_BUFSIZE
#define CONFIG_USBDEV_MSC_MAX_BUFSIZE 512
#endif

/* ================= USB Device Port Configuration ================ */

/* MUSB EP number for SiFli */
#define CONFIG_USB_MUSB_EP_NUM 8
#define CONFIG_USB_MUSB_SIFLI

/* Map PKG defines for SiFli glue code compatibility */
#define PKG_CHERRYUSB_DEVICE

/* ================ USB HOST Stack Configuration ================== */

#define CONFIG_USBHOST_MAX_RHPORTS          1
#define CONFIG_USBHOST_MAX_EXTHUBS          0
#define CONFIG_USBHOST_MAX_EHPORTS          4
#define CONFIG_USBHOST_MAX_INTERFACES       8
#define CONFIG_USBHOST_MAX_INTF_ALTSETTINGS 2
#define CONFIG_USBHOST_MAX_ENDPOINTS        4

#define CONFIG_USBHOST_DEV_NAMELEN 16

#ifndef CONFIG_USBHOST_MAX_BUS
#define CONFIG_USBHOST_MAX_BUS 1
#endif

#ifndef CONFIG_USBHOST_REQUEST_BUFFER_LEN
#define CONFIG_USBHOST_REQUEST_BUFFER_LEN 512
#endif

#ifndef CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT
#define CONFIG_USBHOST_CONTROL_TRANSFER_TIMEOUT 500
#endif

#endif /* CHERRYUSB_CONFIG_H */
