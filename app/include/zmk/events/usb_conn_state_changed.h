/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>

#if IS_ENABLED(CONFIG_ZMK_USB)
#include <zephyr/usb/usb_device.h>
#endif

#include <zmk/event_manager.h>
#include <zmk/usb.h>

struct zmk_usb_conn_state_changed {
    enum zmk_usb_conn_state conn_state;
};

ZMK_EVENT_DECLARE(zmk_usb_conn_state_changed);