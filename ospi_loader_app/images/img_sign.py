import subprocess
import os
import argparse

# Signing parameters
MCUBOOT_DIR = "/home/michael/Alif/alif_mcuboot_example/libs/mcuboot"  # Specify the path to the MCUBoot directory
IMGTOOL_PATH = os.path.join(MCUBOOT_DIR, "scripts", "imgtool.py")
KEY_PATH = os.path.join(MCUBOOT_DIR, "root-rsa-2048.pem")
HEADER_SIZE = 0x800  # Header size
ALIGN = 16  # Alignment

VERSION = "1.2.2"  # Firmware version
SLOT_SIZE = (1536 * 1024)  # Slot size

def sign_binary(input_file, output_file, ram_load_addition=""):
    """
    Signs the given binary file and generates a signed output file.
    :param input_file: Path to the input binary file
    :param output_file: Path to the signed output binary file
    :param ram_load_addition: Additional RAM load parameter for imgtool
    """
    # Check if required files exist
    if not os.path.exists(IMGTOOL_PATH):
        raise FileNotFoundError(f"Imgtool not found at {IMGTOOL_PATH}")
    if not os.path.exists(KEY_PATH):
        raise FileNotFoundError(f"Signing key not found at {KEY_PATH}")
    if not os.path.exists(input_file):
        raise FileNotFoundError(f"Input binary file not found: {input_file}")

    # Build the command
    command = [
        "python", IMGTOOL_PATH,
        "sign",
        "--key", KEY_PATH,
        "--header-size", hex(HEADER_SIZE),
        "--align", str(ALIGN),
        "--pad-header",
        "--version", VERSION,
        "--slot-size", hex(SLOT_SIZE),
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

if __name__ == "__main__":
    # Argument parser
    parser = argparse.ArgumentParser(description="Sign a binary file using MCUBoot imgtool.")
    parser.add_argument("input_file", help="Path to the input binary file to be signed.")
    parser.add_argument(
        "output_file",
        nargs="?",
        default="signed_output.bin",
        help="Path to the output signed binary file (default: signed_output.bin)."
    )
    parser.add_argument(
        "--ram-load-addition",
        default="",
        help="Additional RAM load address parameter for imgtool (e.g., '0x800')."
    )

    args = parser.parse_args()

    # Sign the binary
    sign_binary(args.input_file, args.output_file, args.ram_load_addition)
