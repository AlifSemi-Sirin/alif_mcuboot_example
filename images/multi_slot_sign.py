import subprocess
import os
import argparse

# python3 multi_slot_sign.py ethos-u-alif_kws.bin ethos-u-alif_kws.bin  --versions 1.4.2 1.4.3 --merged_output ccc_merged.bin

# Signing parameters
MCUBOOT_DIR = "/home/michael/Alif/alif_mcuboot_example/libs/mcuboot"  # Path to MCUBoot directory
IMGTOOL_PATH = os.path.join(MCUBOOT_DIR, "scripts", "imgtool.py")
KEY_PATH = os.path.join(MCUBOOT_DIR, "root-rsa-2048.pem")
START_ADDRESS = 0x00000000  # Start address 0for the first slot
HEADER_SIZE = 0x800  # Header size
ALIGN = 16  # Alignment

SLOT_SIZE = (1536 * 1024)  # Slot size (1.5 MB)

def sign_binary(input_file, output_file, slot_size, version, ram_load_addition=""):
    """
    Signs the given binary file and generates a signed output file.
    :param input_file: Path to the input binary file
    :param output_file: Path to the signed output binary file
    :param slot_size: Size of the slot to ensure the binary fits
    :param version: Firmware version for the image
    :param ram_load_addition: Additional RAM load parameter for imgtool
    """
    # Check if required files exist
    if not os.path.exists(IMGTOOL_PATH):
        raise FileNotFoundError(f"Imgtool not found at {IMGTOOL_PATH}")
    if not os.path.exists(KEY_PATH):
        raise FileNotFoundError(f"Signing key not found at {KEY_PATH}")
    if not os.path.exists(input_file):
        raise FileNotFoundError(f"Input binary file not found: {input_file}")

    # Check binary size
    if os.path.getsize(input_file) > slot_size:
        raise ValueError(f"Input file {input_file} exceeds slot size of {slot_size} bytes")

    # Build the command
    command = [
        "python", IMGTOOL_PATH,
        "sign",
        "--key", KEY_PATH,
        "--header-size", hex(HEADER_SIZE),
        "--align", str(ALIGN),
        "--pad-header",
        "--version", version,
        "--slot-size", hex(slot_size),
    ]

    # Add RAM load addition if specified
    if ram_load_addition:
        command.extend(["--load-addr", ram_load_addition])

    # Add input and output file paths
    command.extend([input_file, output_file])

    # Execute the command
    try:
        print(f"Signing {input_file} -> {output_file} {command}...")
        subprocess.run(command, check=True)
        print(f"File signed successfully: {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error signing file: {e}")
        raise

def merge_binaries(output_file, signed_files):
    """
    Merges signed binaries into a single binary file, filling gaps with 0xFF.
    :param output_file: Path to the final merged binary file.
    :param signed_files: List of paths to signed binary files.
    """
    with open(output_file, 'wb') as merged:
        for index, file in enumerate(signed_files):
            # Calculate start address for the slot
            start_address = index * SLOT_SIZE

            # Fill gaps with 0xFF
            merged.seek(start_address)
            with open(file, 'rb') as f:
                data = f.read()
                merged.write(data)

            # Fill remaining space in the slot with 0xFF
            remaining_size = SLOT_SIZE - len(data)
            merged.write(b'\xFF' * remaining_size)

if __name__ == "__main__":
    # Argument parser
    parser = argparse.ArgumentParser(description="Sign and merge multiple binary files using MCUBoot imgtool.")
    parser.add_argument("input_files", nargs='+', help="Paths to input binary files to be signed.")
    parser.add_argument(
        "--versions",
        nargs='+',
        help="Firmware versions for each input file (e.g., '1.2.2 1.2.3')."
    )
    parser.add_argument(
        "--output_dir",
        default="signed_binaries",
        help="Directory to store signed binary files (default: signed_binaries)."
    )
    parser.add_argument(
        "--merged_output",
        default="merged_output.bin",
        help="Path to the final merged binary file (default: merged_output.bin)."
    )
    parser.add_argument(
        "--ram-load-addition",
        default="",
        help="Additional RAM load address parameter for imgtool (e.g., '0x800')."
    )

    args = parser.parse_args()

    # Validate input
    if len(args.input_files) != len(args.versions):
        raise ValueError("Number of input files must match number of versions.")

    # Create output directory if it doesn't exist
    os.makedirs(args.output_dir, exist_ok=True)

    signed_files = []

    # Process each input file
    for index, input_file in enumerate(args.input_files):
        output_file = os.path.join(args.output_dir, f"signed_output_{index + 1}.bin")
        ram_load_address = hex(START_ADDRESS + index * SLOT_SIZE)  # Calculate start address for each slot
        try:
            sign_binary(input_file, output_file, SLOT_SIZE, args.versions[index], ram_load_address)
            signed_files.append(output_file)
        except Exception as e:
            print(f"Failed to process {input_file}: {e}")

    # Merge signed binaries into a single output file
    merge_binaries(args.merged_output, signed_files)
    print(f"Merged binary created: {args.merged_output}")
