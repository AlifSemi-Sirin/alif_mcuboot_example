# Commands for Signing Binary Files Using Python Script

This guide provides commands to use the Python script for signing binary files with MCUBoot's `imgtool.py`.

---

## Prerequisites
1. **MCUBoot Directory**:
   - Ensure `imgtool.py` and signing key (`root-rsa-2048.pem`) are available in the MCUBoot directory.
   - Example: `/path/to/mcuboot/`
2. **Python Environment**:
   - Python 3.x installed.
3. **Input Binary File**:
   - Ensure the input `.bin` file is ready.

---

## Usage Syntax
```bash
python sign_binary.py <input_file> [output_file] [--ram-load-addition <load_address>]
```

### Arguments
- `<input_file>`: Path to the binary file to be signed (required).
- `[output_file]`: Path to the output signed binary file (optional, defaults to `signed_output.bin`).
- `--ram-load-addition <load_address>`: Optional load address to specify `--load-addr` for `imgtool`.

---

## Examples

### 1. Signing a Binary File Without RAM Load Addition
```bash
python sign_binary.py example_app.bin
```
**Output**:
- The signed file is saved as `signed_output.bin` in the current directory.

### 2. Signing a Binary File with a Custom Output Name
```bash
python sign_binary.py example_app.bin custom_signed_app.bin
```
**Output**:
- The signed file is saved as `custom_signed_app.bin` in the current directory.

### 3. Signing a Binary File with RAM Load Addition
```bash
python sign_binary.py example_app.bin signed_with_load.bin --ram-load-addition 0x800
```
**Output**:
- The signed file is saved as `signed_with_load.bin` in the current directory.

---

## Debugging

### Common Errors

#### `FileNotFoundError`
- **Cause**: Missing `imgtool.py`, key file, or input binary file.
- **Solution**: Verify the paths to:
  - `imgtool.py`
  - Signing key (`root-rsa-2048.pem`)
  - Input binary file

#### `subprocess.CalledProcessError`
- **Cause**: An error occurred during the signing process.
- **Solution**: Check the terminal output for the exact error and ensure all parameters are correct.

---

## Script Location
Ensure the script `sign_binary.py` is in the working directory or accessible through the specified path.

