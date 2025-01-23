# User Guide for AI/ML Application Loader

## Overview

This project is designed to enable users to seamlessly run AI/ML application examples on the Alif Ensemble DevKit without the need for heavy firmware reflashing. The system allows users to select and execute preloaded AI/ML applications from a menu interface. Applications are stored in external OSPI flash and dynamically loaded into internal memory for execution. This setup maximizes the flexibility and usability of the development kit, providing a straightforward way to test the performance of different AI/ML models.

---

## Key Features

- **Dynamic Application Loading**:
  - Applications are stored in external OSPI flash.
  - Users can load applications into the primary memory slot and execute them directly.
- **Menu-Based Application Selection**:
  - The device displays a bootloader menu upon startup.
  - Users can choose from:
    - `0`: Run the pre-flashed application in internal memory.
    - `1, 2, 3, ...`: Load and run AI/ML applications stored in external memory.
- **Preloaded AI/ML Examples**:
  - Examples include models for object detection, keyword spotting, and more.
  - Applications are optimized for the Ensemble platform's neural processing capabilities.

---

## Purpose

The goal of this project is to provide a flexible, user-friendly environment for evaluating the performance of the Alif Ensemble DevKit with various AI/ML applications. By leveraging the dynamic loading mechanism, users can:

- Experiment with different AI/ML use cases.
- Quickly switch between applications without firmware updates.
- Maximize the potential of the platform for diverse machine learning tasks.

---

## Quick Start Guide

### 1. Setup the Development Kit

- Ensure the Alif Ensemble DevKit is connected to your computer via USB.
- Use a terminal emulator such as [Tera Term](https://teratermproject.github.io/index-en.html) or `minicom` (Linux) to communicate with the board.
- Configure the terminal to use the appropriate serial port with a baud rate of `115200`.

### 2. Bootloader Menu

Upon powering the device, the bootloader displays a menu similar to:

```
==== Bootloader Menu ====
0. Start Primary Image: v1.5.1, size 145712, slot_id 1, File: flasher.bin, Comment: Bootloader application
1. Copy Secondary Image: v1.5.1, size 145712, slot_id 10, File: flasher.bin, Comment: Bootloader application
2. Copy Secondary Image: v1.5.2, size 749104, slot_id 11, File: ethos-u-alif_kws.bin, Comment: Application 1
3. Copy Secondary Image: v1.5.3, size 1133248, slot_id 12, File: ethos-u-alif_obj_detection.bin, Comment: Application 2
=========================
Enter your choice:
```

- Press `0` to run the default application stored in internal memory.
- Press `1, 2, 3...` to load and run an application from external OSPI flash.

---

## Updating OSPI Applications Using the Flasher App

If you need to update or program new applications into the OSPI flash, follow the steps below:

### Step 1: Select the Flasher Application
1. Power on the device and use the bootloader menu.
2. Select the `flasher` application (e.g., option `1` or as indicated in your configuration).

### Step 2: Run the Flasher Application
1. Once the flasher application is running, observe the LED sequence:
   - **Red -> Green -> Off**: Indicates successful startup of the flasher application.

### Step 3: Connect the Host System
1. Connect a USB cable to the micro-USB port (J2) on the Alif Ensemble DevKit.
2. Identify the USB device on your host system. It typically appears as `/dev/ttyACMx` (where `x` can be `2`, `3`, etc.).

### Step 4: Start a Terminal Emulator
1. Open a terminal emulator such as `minicom`:
   ```bash
   minicom -D /dev/ttyACMx
   ```
2. Replace `/dev/ttyACMx` with the correct port for your device.

### Step 5: Transfer Firmware to OSPI Flash
1. Press `Ctrl-A`, then `S` to open the file transfer menu in `minicom`.
2. Select `XModem` as the transfer protocol.
3. Choose the firmware file you want to upload to the OSPI flash.
4. Confirm the transfer and wait for it to complete.

### Step 6: Verify the Transfer
1. Observe the terminal for success messages from the flasher application.
2. If no errors are reported, the firmware has been successfully written to the OSPI flash.

---

## System Architecture

### Memory Layout

- **Primary Slot**:
  - Located in internal MRAM.
  - Runs the currently active application.
- **Secondary Slots**:
  - Stored in external OSPI flash.
  - Contain preloaded AI/ML application images.

### Bootloader Workflow

1. **Initialization**:
   - The bootloader initializes MRAM and OSPI flash.
   - Dynamically scans and validates images in OSPI flash.
2. **Menu Display**:
   - Reads metadata from each image and displays a selection menu.
3. **Application Execution**:
   - Copies the selected image into MRAM.
   - Validates and executes the image.

---

## Supported AI/ML Applications

The preloaded applications demonstrate various AI/ML capabilities, such as:

- **Keyword Spotting**:
  - Detects specific spoken keywords.
- **Object Detection**:
  - Identifies and classifies objects in images.
- **Other Use Cases**:
  - Custom AI/ML tasks tailored to the platform.

---

## Compilation for Bootloader Execution

### Memory Map Requirements

To ensure compatibility with the bootloader, applications must adhere to the following memory map:

- **Memory Configuration**:
  ```
  MEMORY
  {
    DTCM  (rwx) : ORIGIN = 0x20000000, LENGTH = 0x00040000
    SRAM0 (rwx) : ORIGIN = 0x02000000, LENGTH = 0x00400000
    MRAM  (rx)  : ORIGIN = 0x80010800, LENGTH = 0x00476800
  }
  ```

- **Stack and Heap Configuration**:
  ```
  __STACK_SIZE    = 0x00002000;
  __HEAP_SIZE     = 0x00004000;
  __APP_HEAP_SIZE = 0x00004000;
  ```

- **Section Allocation**:
  - Startup code and read-only data are placed in MRAM.
  - Executable code (.text) resides in MRAM.
  - Initialized data (.data) is loaded into DTCM and copied from MRAM at runtime.
  - Uninitialized data (.bss) is allocated in DTCM.
  - Application-specific heap is configured in SRAM0.

- **Example Linker Script Extract**:
  ```
  SECTIONS
  {
    .startup : ALIGN(16) {
      KEEP(*(.vectors))
      KEEP(*(.init))
      KEEP(*(.fini))
      *(.rodata*)
    } > MRAM

    .text : ALIGN(16) {
      *(.text*)
    } > MRAM

    .data : ALIGN(16) {
      __data_start__ = .;
      *(.data)
      __data_end__ = .;
    } > DTCM AT > MRAM

    .bss (NOLOAD) : ALIGN(8) {
      __bss_start__ = .;
      *(.bss)
      __bss_end__ = .;
    } > DTCM

    .stack (NOLOAD) : ALIGN(8) {
      __StackLimit = .;
      . = . + __STACK_SIZE;
      __StackTop = .;
    } > DTCM
  }
  ```

### Compilation Steps

1. Configure the linker script to match the bootloader’s memory requirements.
2. Compile the application using the appropriate toolchain (e.g., GCC Arm).
3. Use `imgtool.py` to sign the compiled binary and add the necessary header.
4. Verify the final binary with the bootloader’s validation tools (if available).

---

## Advantages

- **Ease of Use**:
  - Simplifies testing AI/ML models on the Ensemble platform.
- **Flexibility**:
  - Quickly switch between applications without firmware reflashing.
- **Performance Evaluation**:
  - Enables users to measure the performance of the platform with different AI/ML tasks.

---

## Troubleshooting

- **Bootloader Errors**:
  - Ensure that the OSPI flash is correctly programmed.
  - Check for metadata issues if images are not displayed in the menu.
- **Communication Issues**:
  - Verify the terminal emulator settings (baud rate, serial port, etc.).
- **Flasher Application**:
  - Ensure the XMODEM protocol is correctly configured.

---

## Additional Resources

- [MCUboot Documentation](https://docs.mcuboot.com/)
- [Alif Ensemble DevKit Documentation](https://alifsemi.com/support/software-tools/ensemble/)
- [Getting Started with Alif Templates](https://github.com/alifsemi/alif_vscode-template/blob/main/doc/getting_started.md)

