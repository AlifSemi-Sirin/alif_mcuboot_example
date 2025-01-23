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
#include <string.h>
#include "pinconf.h"

extern ARM_DRIVER_FLASH ARM_Driver_Flash_(1);
static ARM_DRIVER_FLASH *FlashDrv = &ARM_Driver_Flash_(1);

static ARM_FLASH_STATUS flash_status;

// Static page buffer and control variables
#define PAGE_SIZE                                              256
#define FLASH_SIZE                                             (16 * 1024 * 1024) // 16MB

static uint8_t rx_buffer[PAGE_SIZE];            // Buffer for a single page
static uint8_t tx_buffer[PAGE_SIZE];            
static uint32_t page_start_addr = 0xFFFFFFFF; // Start address of cached page
static uint32_t page_valid_size = 0;          // Valid data size in the page


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


/**
 * @brief Deinitializes the OSPI flash memory.
 *
 * This function ensures proper cleanup of the OSPI flash memory by
 * resetting related states, powering down the flash, and deinitializing the driver.
 *
 * @return int32_t Returns 0 on success, -1 on failure.
 */
int ospi_flash_deinit(void)
{
    int32_t ret;

    // Power off the flash
    ret = FlashDrv->PowerControl(ARM_POWER_OFF);
    if (ret != ARM_DRIVER_OK) {
        printf("Power down OSPI failed, error: %lx\n", ret);
        return -1;
    }

    // Deinitialize the flash driver
    ret = FlashDrv->Uninitialize();
    if (ret != ARM_DRIVER_OK) {
        printf("OSPI Flash: Deinitialization failed, error: %lx\n", ret);
        return -1;
    }

    return 0;
}


static int32_t ospi_drv_init(void)
{
    int32_t ret = FlashDrv->Initialize(NULL);
    if (ret != ARM_DRIVER_OK)
    {
        printf("OSPI Flash: Init failed, error: %lx\n", ret);
        return -1;
    }

    ret = FlashDrv->PowerControl(ARM_POWER_FULL);
    if (ret != ARM_DRIVER_OK)
    {
        printf("Power OSPI failed, error: %lx\n", ret);
        return -1;
    }

    return 0;
}

/**
 * @brief Fills the page buffer from the specified address.
 *
 * Reads a 256-byte page from flash memory into the buffer.
 *
 * @param addr Address to read from flash memory.
 * @return int32_t Returns 0 on success, -1 on failure.
 */
static int32_t ospi_fill_page(uint32_t addr)
{
    // Align address to the page size
    uint32_t aligned_addr = addr & ~(PAGE_SIZE - 1);

    // printf("Read OSPI at addr: 0x%lx\n", aligned_addr);

    // Check if the flash is busy
    flash_status = FlashDrv->GetStatus();
    if (flash_status.busy) {
        printf("OSPI busy!\n");
        return -1;
    }    

    // Read one page into the buffer
    int32_t ret = FlashDrv->ReadData(aligned_addr, rx_buffer, PAGE_SIZE);
    if (ret != PAGE_SIZE) {
        printf("Page fill error: %lx\n", ret);
        return -1;
    }

    // Update page metadata
    page_start_addr = aligned_addr;
    page_valid_size = PAGE_SIZE;

    // Print data
    // dump_data("-->>rd data", rx_buffer, PAGE_SIZE);

    return 0;
}

/**
 * @brief Reads data using page buffering to optimize flash access.
 *
 * Supports partial reads and unaligned addresses by using cached page data.
 *
 * @param addr Address to read from.
 * @param data Pointer to output buffer.
 * @param cnt Number of bytes to read.
 * @return int32_t Returns 0 on success, -1 on failure.
 */
static int32_t ospi_read_page_buffered(uint32_t addr, void *data, uint32_t cnt)
{
    uint8_t *output = (uint8_t *)data;
    uint32_t remaining = cnt;

    // Check if address and length exceed allowed range
    if ((addr + cnt) > FLASH_SIZE) {
        printf("Error: Read out of bounds: addr=0x%lx, cnt=0x%lx\n", addr, cnt);
        return -1;
    }

    while (remaining > 0) {
        // Align address to 256-byte boundary
        uint32_t aligned_addr = addr & ~(PAGE_SIZE - 1);

        // Load a new page if not already loaded
        if (aligned_addr != page_start_addr) {
            if (ospi_fill_page(aligned_addr) != 0) {
                return -1; // Error filling page
            }
        }

        // Calculate offset within the current page
        uint32_t offset = addr - page_start_addr;

        // Determine how many bytes can be copied from the page
        uint32_t bytes_to_copy = PAGE_SIZE - offset;
        if (bytes_to_copy > remaining) {
            bytes_to_copy = remaining;
        }

        // Copy data from the page buffer
        memcpy(output, &rx_buffer[offset], bytes_to_copy);

        // Update pointers and counters
        addr += bytes_to_copy;
        output += bytes_to_copy;
        remaining -= bytes_to_copy;
    }

    return 0;
}

static void ospi_read_data_clear_cash(void)
{
    page_start_addr = 0xFFFFFFFF;
    page_valid_size = 0;
}

/**
 * @brief Reads data from OSPI flash memory, ensuring 16-bit alignment.
 * 
 * This function handles cases where the OSPI flash memory only supports 
 * 16-bit word reads. If the requested data size (cnt) is odd, it performs 
 * an aligned read and safely handles the last byte using a temporary buffer.
 *
 * Algorithm:
 * 1. Ensure the total number of bytes to read is even to match 16-bit alignment.
 * 2. Use a temporary buffer to handle any odd-byte cases without overwriting memory.
 * 3. Copy only the required bytes into the provided buffer.
 *
 * @param addr  The start address in flash memory to read from.
 * @param data  Pointer to the buffer where data will be stored.
 * @param cnt   Number of bytes to read.
 * @return int32_t Returns 0 on success, -1 on failure.
 */
static int32_t ospi_read_data(uint32_t addr, void *data, uint32_t cnt)
{
    // printf("OSPI RD buffered: 0x%lx, cnt: 0x%lx\n", addr, cnt);

    // Read data using page-buffered function
    int32_t status = ospi_read_page_buffered(addr, data, cnt);
    if (status != 0) {
        printf("Read error: %lx\n", status);
        return status;
    }

    return 0; 
}

/**
 * @brief Writes data to OSPI flash memory in 256-byte pages.
 *
 * This function writes data to the flash memory by splitting it into
 * 256-byte pages, as required by the flash memory's page-based programming.
 *
 * @param addr Address to write data to in flash memory.
 * @param data Pointer to the data to be written.
 * @param cnt Number of bytes to write.
 * @return int32_t Returns 0 on success, -1 on failure.
 */
static int32_t ospi_write_data(uint32_t addr, const void *data, uint32_t cnt)
{
    const uint8_t *input = (const uint8_t *)data;  // Input data buffer
    uint32_t remaining = cnt;                      // Remaining bytes to write

    // Check if address and length exceed allowed range
    if ((addr + cnt) > FLASH_SIZE)
    {
        printf("Error: Write out of bounds: addr=0x%lx, cnt=0x%lx\n", addr, cnt);
        return -1;
    }

    while (remaining > 0)
    {
        // Align address to 256-byte page boundary
        uint32_t aligned_addr = addr & ~(PAGE_SIZE - 1);
        uint32_t offset = addr - aligned_addr;

        // Determine how many bytes to write in the current page
        uint32_t bytes_to_write = PAGE_SIZE - offset;
        if (bytes_to_write > remaining)
        {
            bytes_to_write = remaining;
        }

        // Step 1: Read the current page into a buffer
        int32_t status = FlashDrv->ReadData(aligned_addr, tx_buffer, PAGE_SIZE);
        if (status != PAGE_SIZE)
        {
            printf("Read error at 0x%lx: %lx\n", aligned_addr, status);
            return -1;
        }

        // Step 2: Modify the required portion in the page buffer
        memcpy(&tx_buffer[offset], input, bytes_to_write);

        // Step 3: Write the modified page back to the flash memory
        status = FlashDrv->ProgramData(aligned_addr, tx_buffer, PAGE_SIZE);
        if (status != PAGE_SIZE)
        {
            printf("Write error at 0x%lx: %lx\n", aligned_addr, status);
            return -1;
        }

        // Step 4: Wait until the flash is ready
        flash_status = FlashDrv->GetStatus();
        if (flash_status.busy)
        {
            printf("OSPI busy during write at 0x%lx\n", aligned_addr);
            return -1;
        }

        // Update pointers and counters
        addr += bytes_to_write;
        input += bytes_to_write;
        remaining -= bytes_to_write;
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

    return 0;
}
