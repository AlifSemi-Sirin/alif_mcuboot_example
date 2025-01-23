# Multi-Slot Signing Script Guide

This guide explains how to use the Python script for signing and merging firmware images into a single binary file with metadata.

---

## Prerequisites
1. Python 3.x installed on your system.
2. Ensure the following paths in the script are valid:
   - `MCUBOOT_DIR`: Path to the MCUBoot directory.
   - `IMGTOOL_PATH`: Path to the `imgtool.py` script within MCUBoot.
   - `KEY_PATH`: Path to the signing key (e.g., `root-rsa-2048.pem`).
3. Input binary files to be signed.

---

## Script Overview
The script performs the following tasks:
1. Signs firmware binaries using the MCUBoot `imgtool.py` utility.
2. Merges the signed binaries into a single output file.
3. Appends metadata to the merged binary, including:
   - Slot ID
   - File name
   - File size
   - Firmware version
   - Comment

---

## Usage
Run the script using the command-line interface. Below are the command-line arguments:

### Required Arguments
- **`input_files`**: Paths to input binary files to be signed.
- **`--versions`**: Firmware versions for each input file (e.g., `1.2.2 1.3.0`).

### Optional Arguments
- **`--comments`**: Comments for each slot (e.g., `"Primary firmware" "Secondary firmware"`).
  - If not provided, defaults to `"No comments"`.
- **`--output_dir`**: Directory to store signed binary files (default: `signed_binaries`).
- **`--merged_output`**: Path to the final merged binary file (default: `merged_output.bin`).
- **`--ram-load-addition`**: Additional RAM load address parameter for `imgtool` (e.g., `0x800`).

---

## Examples

### Basic Usage
Sign and merge two firmware binaries:
```bash
python multi_slot_signing.py firmware1.bin firmware2.bin \
    --versions 1.0.0 2.1.0 \
    --output_dir signed_binaries \
    --merged_output merged_output.bin
```

### Including Comments
Add specific comments for each firmware:
```bash
python multi_slot_signing.py firmware1.bin firmware2.bin firmware3.bin \
    --versions 1.0.0 2.1.0 3.0.1 \
    --comments "Primary firmware" "Secondary firmware" "Debug firmware" \
    --output_dir signed_binaries \
    --merged_output merged_output.bin
```

### Using Default Comments
If no comments are provided, the script assigns `"No comments"`:
```bash
python multi_slot_signing.py firmware1.bin firmware2.bin \
    --versions 1.0.0 2.1.0 \
    --output_dir signed_binaries \
    --merged_output merged_output.bin
```

---

## Output
1. **Signed Binaries**:
   - Stored in the directory specified by `--output_dir`.
   - Example: `signed_binaries/signed_output_1.bin`, `signed_binaries/signed_output_2.bin`.

2. **Merged Binary**:
   - Single output file combining all signed binaries, specified by `--merged_output`.
   - Includes metadata appended at the end.

3. **Metadata**:
   - JSON format embedded in the merged binary.
   - Example:
     ```json
     {
         "slots": [
             {
                 "slot_id": 10,
                 "file": "firmware1.bin",
                 "size": 123456,
                 "version": "1.0.0",
                 "comment": "Primary firmware"
             },
             {
                 "slot_id": 11,
                 "file": "firmware2.bin",
                 "size": 654321,
                 "version": "2.1.0",
                 "comment": "Secondary firmware"
             }
         ]
     }
     ```

---

## Error Handling
1. **Mismatch Between Files and Versions**:
   - If the number of versions does not match the number of input files, the script raises an error:
     ```
     ValueError: Number of input files must match number of versions.
     ```

2. **Mismatch Between Files and Comments**:
   - If the `--comments` option is used, the number of comments must match the number of input files. Otherwise:
     ```
     ValueError: Number of comments must match number of input files.
     ```

3. **File Not Found**:
   - If a required file (e.g., input binary or signing key) is missing, the script raises:
     ```
     FileNotFoundError: <file_path>
     ```

---

## Notes
- Ensure all input files, versions, and optional comments are correctly specified.
- Metadata comments can include spaces if enclosed in quotes.



