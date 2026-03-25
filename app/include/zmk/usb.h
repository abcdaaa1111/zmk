/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zephyr/usb/usb_device.h>
#include <zephyr/usb/class/usb_hid.h>
#endif

#include <zmk/keys.h>
#include <zmk/hid.h>

enum zmk_usb_conn_state {
    ZMK_USB_CONN_NONE,
    ZMK_USB_CONN_POWERED,
    ZMK_USB_CONN_HID,
};

#if IS_ENABLED(CONFIG_ZMK_USB)
enum usb_dc_status_code zmk_usb_get_status(void);
#endif

enum zmk_usb_conn_state zmk_usb_get_conn_state(void);

static inline bool zmk_usb_is_powered(void) {
    return zmk_usb_get_conn_state() != ZMK_USB_CONN_NONE;
}
bool zmk_usb_is_hid_ready(void);
