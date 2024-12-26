/* Copyright (C) 2024 Alif Semiconductor - All Rights Reserved.
 * Use, distribution and modification of this code is permitted under the
 * terms stated in the Alif Semiconductor Software License Agreement
 *
 * You should have received a copy of the Alif Semiconductor Software
 * License Agreement with this file. If not, please write to:
 * contact@alifsemi.com, or visit: https://alifsemi.com/license
 *
 */

/*
 * Entrypoint for the MCUBoot bootloader.
 * */

#include "mcuboot_customize/flash_map_mram.h"
#include "Driver_Flash.h"
#include "RTE_Components.h"
#include CMSIS_device_header

#include <stdio.h>
#include "pinconf.h"

extern ARM_DRIVER_FLASH ARM_Driver_Flash_(1);
static ARM_DRIVER_FLASH *FlashDrv = &ARM_Driver_Flash_(1);

static ARM_FLASH_STATUS flash_status;

void dump_data(const char *msg, const uint8_t *data, uint32_t len)
{
    printf("%s:\n", msg);
    for (uint32_t i = 0; i < len; i++)
    {
        printf("%02X ", data[i]);
        if ((i + 1) % 16 == 0)
            printf("\n");
    }
    printf("\n");
}

static int32_t ospi_drv_init(void)
{
    int32_t status = FlashDrv->Initialize(NULL);
    if (status != ARM_DRIVER_OK)
    {
        printf("OSPI Flash: Init failed, error: %lx\n", status);
        return -1;
    }

    status = FlashDrv->PowerControl(ARM_POWER_FULL);
    if (status != ARM_DRIVER_OK)
    {
        printf("Power OSPI failed, error: %lx\n", status);
        return -1;
    }

    return 0;
}

static int32_t ospi_read_data(uint32_t addr, void *data, uint32_t cnt)
{
    flash_status = FlashDrv->GetStatus();
    if (flash_status.busy) {
        printf("OSPI busy!\n");
        return -1;
    }

    printf("OSPI Read addr: %lx, cnt: %lx\n", addr, cnt);
    int32_t status = FlashDrv->ReadData(addr, data, cnt);
    if ((int64_t)cnt != (int64_t)status)
    {
        printf("Read error: %lx\n", status);
        return -1;
    }

    dump_data("rd data", data, cnt);

    return 0;
}


static int32_t ospi_write_data(uint32_t addr, const void *data, uint32_t cnt)
{
    int32_t status = FlashDrv->EraseSector(addr);
    if (status != ARM_DRIVER_OK) {
        printf("Erase error: %lx\n", status);
        return -1;
    }

    flash_status = FlashDrv->GetStatus();
    if (flash_status.busy) {
        printf("OSPI busy!\n");
        return -1;
    }

    printf("OSPI Write addr: %lx, cnt: %lx\n", addr, cnt);
    FlashDrv->ProgramData(addr, data, cnt);
    // if (status != cnt) {
    //     printf("Write error: %lx\n", status);
    //     return -1;
    // }

    flash_status = FlashDrv->GetStatus();
    if (flash_status.busy) {
        printf("OSPI busy!\n");
        return -1;
    }

    return 0;
}


int ospi_flash_init(void)
{
    /* OSPI1 interface (Flash) */
    pinconf_set(PORT_9, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE);  // D0
    pinconf_set(PORT_9, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE);  // D1
    pinconf_set(PORT_9, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE);  // D2
    pinconf_set(PORT_10, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // D3
    pinconf_set(PORT_10, PIN_1, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // D4
    pinconf_set(PORT_10, PIN_2, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // D5
    pinconf_set(PORT_10, PIN_3, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // D6
    pinconf_set(PORT_10, PIN_4, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // D7
    pinconf_set(PORT_5, PIN_5, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST);                        // SCLK
    pinconf_set(PORT_8, PIN_0, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST);                        // SCLKN
    pinconf_set(PORT_5, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST);                        // SS0
    pinconf_set(PORT_10, PIN_7, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // RXDS
    // Ensemble B1 workaround - function of P10_7 is controlled by function of P5_6. Fixed in B2
    pinconf_set(PORT_5, PIN_6, PINMUX_ALTERNATE_FUNCTION_1, PADCTRL_OUTPUT_DRIVE_STRENGTH_12MA | PADCTRL_SLEW_RATE_FAST | PADCTRL_READ_ENABLE); // RXDS
    pinconf_set(PORT_LP, PIN_7, PINMUX_ALTERNATE_FUNCTION_0, 0);   
    
    OSPI_Driver.Init = ospi_drv_init;                       
    OSPI_Driver.ReadData = ospi_read_data;
    OSPI_Driver.WriteData = ospi_write_data;

    printf("OSPI Init\n");
    OSPI_Driver.Init();

    // status = FlashDrv->Initialize(NULL);
    // if (status != ARM_DRIVER_OK)
    // {
    //     printf("OSPI Flash: Init failed, error: %lx\n", status);
    //     return -1;
    // }

    // status = FlashDrv->PowerControl(ARM_POWER_FULL);
    // if (status != ARM_DRIVER_OK)
    // {
    //     printf("Power OSPI failed, error: %lx\n", status);
    //     return -1;
    // }

    // const uint32_t sector_addr = 1 * FlashDrv->GetInfo()->sector_size;
    // const uint32_t sector_size = FlashDrv->GetInfo()->sector_size;
    // const uint32_t sector_count = FlashDrv->GetInfo()->sector_count;
    // printf("OSPI Flash: Sector Size: %lu bytes, Sector Count: %lu\n", sector_size, sector_count);
    // printf("OSPI Flash: Sector Address: %lu bytes\n", sector_addr);
    // printf("\n");

    // status = FlashDrv->EraseSector(TEST_ADDRESS);
    // if (status != ARM_DRIVER_OK) {
    //     printf("Erase error: %lx\n", status);
    //     return;
    // }

    // ARM_FLASH_STATUS flash_status = FlashDrv->GetStatus();
    // if (flash_status.busy) {
    //     printf("OSPI busy!\n");
    //     return;
    // }

    // for (uint32_t i = 0; i < TEST_DATA_SIZE; i++) {
    //     tx_buffer[i] = (uint8_t)(i & 0xFF); // Заполняем данные тестовыми значениями
    // }
    // dump_data("wr data:", tx_buffer, TEST_DATA_SIZE);

    // // data destroyed in the TX buffer after tx finish
    // memcpy(add_buffer, tx_buffer, TEST_DATA_SIZE);
    // status = FlashDrv->ProgramData(TEST_ADDRESS, tx_buffer, TEST_DATA_SIZE);
    
    // if (status != TEST_DATA_SIZE) {
    //     printf("Write error: %lx\n", status);
    //     return;
    // }

    // flash_status = FlashDrv->GetStatus();
    // if (flash_status.busy) {
    //     printf("OSPI busy!\n");
    //     return;
    // }

    // memset(rx_buffer, 0, TEST_DATA_SIZE); // Очищаем буфер
    // status = FlashDrv->ReadData(TEST_ADDRESS, rx_buffer, TEST_DATA_SIZE);
    // if (status != TEST_DATA_SIZE) {
    //     printf("Read error: %lx\n", status);
    //     return;
    // }

    // dump_data("Read data mass", rx_buffer, TEST_DATA_SIZE);

    // // Check that the data was written correctly
    // if (memcmp(add_buffer, rx_buffer, TEST_DATA_SIZE) == 0) {
    //     printf("WR Data success.\n");
    // } else {
    //     printf("WR Data failed.\n");
    // }

    // status = FlashDrv->PowerControl(ARM_POWER_OFF);
    // if (status != ARM_DRIVER_OK) {
    //     printf("Power OFF OSPI error: %lx\n", status);
    // }

    // status = FlashDrv->Uninitialize();
    // if (status != ARM_DRIVER_OK) {
    //     printf("Deinit OSPI error: %lx\n", status);
    // }

    return 0;
}
