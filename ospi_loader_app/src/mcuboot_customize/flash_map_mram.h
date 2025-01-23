/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

#ifndef OSPI_DRIVER_H
#define OSPI_DRIVER_H

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdint.h>

    typedef int32_t (*OspiReadData_t)(uint32_t addr, void *data, uint32_t cnt);
    typedef int32_t (*OspiWriteData_t)(uint32_t addr, const void *data, uint32_t cnt);
    typedef int32_t (*OspiEraseData_t)(uint32_t addr, uint32_t size);
    typedef int32_t (*OspiInit_t)(void);
    typedef int32_t (*OspiDeinit_t)(void);

    typedef struct
    {
        OspiInit_t Init;
        OspiDeinit_t Deinit;
        OspiReadData_t ReadData;
        OspiWriteData_t WriteData;
        OspiEraseData_t EraseData;
    } OspiDriver_t;

    extern OspiDriver_t OSPI_Driver;

#ifdef __cplusplus
}
#endif

#endif /* OSPI_DRIVER_H */
