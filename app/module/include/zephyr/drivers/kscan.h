/*
 * Copyright (c) 2019 Intel Corporation
 * Copyright (c) 2024 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 *
 * Compatibility shim: Zephyr removed the kscan subsystem, but ZMK still
 * provides its own kscan drivers. This header supplies the types and inline
 * helpers that the old <zephyr/drivers/kscan.h> used to provide.
 */

#ifndef ZEPHYR_INCLUDE_DRIVERS_KSCAN_H_
#define ZEPHYR_INCLUDE_DRIVERS_KSCAN_H_

#include <zephyr/device.h>
#include <zephyr/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Keyboard scan callback called when user presses/releases a key on a
 * matrix keyboard.
 *
 * @param dev   Pointer to the device structure for the driver instance.
 * @param row   Describes row change.
 * @param column Describes column change.
 * @param pressed Describes the kind of key event.
 */
typedef void (*kscan_callback_t)(const struct device *dev, uint32_t row,
				 uint32_t column, bool pressed);

/**
 * @cond INTERNAL_HIDDEN
 *
 * Kscan driver API definition and target function prototypes.
 * (Internal use only.)
 */
typedef int (*kscan_config_t)(const struct device *dev,
			      kscan_callback_t callback);
typedef int (*kscan_disable_callback_t)(const struct device *dev);
typedef int (*kscan_enable_callback_t)(const struct device *dev);

__subsystem struct kscan_driver_api {
	kscan_config_t config;
	kscan_enable_callback_t enable_callback;
	kscan_disable_callback_t disable_callback;
};

/**
 * @endcond
 */

/**
 * @brief Configure a Keyboard scan callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 * @param callback Pointer to the callback function.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int kscan_config(const struct device *dev,
			       kscan_callback_t callback)
{
	const struct kscan_driver_api *api =
		(const struct kscan_driver_api *)dev->api;

	return api->config(dev, callback);
}

/**
 * @brief Enables callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int kscan_enable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
		(const struct kscan_driver_api *)dev->api;

	return api->enable_callback(dev);
}

/**
 * @brief Disables callback.
 *
 * @param dev Pointer to the device structure for the driver instance.
 *
 * @retval 0 If successful.
 * @retval Negative errno code if failure.
 */
static inline int kscan_disable_callback(const struct device *dev)
{
	const struct kscan_driver_api *api =
		(const struct kscan_driver_api *)dev->api;

	return api->disable_callback(dev);
}

#ifdef __cplusplus
}
#endif

#endif /* ZEPHYR_INCLUDE_DRIVERS_KSCAN_H_ */
