/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 * Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

#ifndef OSPI_BOOTLOADER_INIT_H
#define OSPI_BOOTLOADER_INIT_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

// Function declarations
void dump_data(const char *msg, const uint8_t *data, uint32_t len);
int ospi_flash_init(void);

#ifdef __cplusplus
}
#endif

#endif // OSPI_BOOTLOADER_INIT_H
