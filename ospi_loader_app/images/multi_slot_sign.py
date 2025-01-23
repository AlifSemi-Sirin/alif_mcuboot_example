import subprocess
import os
import argparse
import json

# Signing parameters
MCUBOOT_DIR = "/home/michael/Alif/alif_mcuboot_example/libs/mcuboot"  # Path to MCUBoot directory
IMGTOOL_PATH = os.path.join(MCUBOOT_DIR, "scripts", "imgtool.py")
KEY_PATH = os.path.join(MCUBOOT_DIR, "root-rsa-2048.pem")
HEADER_SIZE = 0x800  # Header size
ALIGN = 32  # Alignment
EXTRA_SPACE = 32 * 1024  # 32 KB additional space per slot
ALIGNMENT_BOUNDARY = 256  # 256-byte alignment
MAX_STRING_LEN = 32  # Maximum length for strings in metadata

def align_size(size, alignment):
    """
    Aligns the size to the specified boundary.
    """
    return (size + alignment - 1) & ~(alignment - 1)

def sign_binary(input_file, output_file, slot_size, version, ram_load_addition=""):
    """
    Signs the given binary file and generates a signed output file.
    """
    if not os.path.exists(IMGTOOL_PATH):
        raise FileNotFoundError(f"Imgtool not found at {IMGTOOL_PATH}")
    if not os.path.exists(KEY_PATH):
        raise FileNotFoundError(f"Signing key not found at {KEY_PATH}")
    if not os.path.exists(input_file):
        raise FileNotFoundError(f"Input binary file not found: {input_file}")

    if os.path.getsize(input_file) > slot_size:
        raise ValueError(f"Input file {input_file} exceeds slot size of {slot_size} bytes")

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

    if ram_load_addition:
        command.extend(["--load-addr", ram_load_addition])

    command.extend([input_file, output_file])

    try:
        print(f"Signing {input_file} -> {output_file} {command}...")
        subprocess.run(command, check=True)
        print(f"File signed successfully: {output_file}")
    except subprocess.CalledProcessError as e:
        print(f"Error signing file: {e}")
        raise

def merge_binaries(output_file, signed_files, slot_sizes, metadata):
    """
    Merges signed binaries into a single binary file with metadata appended.
    """
    with open(output_file, 'wb') as merged:
        offset = 0
        for index, file in enumerate(signed_files):
            slot_size = slot_sizes[index]

            # Write binary data to the slot
            merged.seek(offset)
            with open(file, 'rb') as f:
                data = f.read()
                merged.write(data)

            # Fill remaining space in the slot
            remaining_size = slot_size - len(data)
            merged.write(b'\xFF' * remaining_size)

            # Update offset
            offset += slot_size

        # Append metadata as JSON
        metadata_offset = align_size(offset, ALIGNMENT_BOUNDARY)
        merged.seek(metadata_offset)
        merged.write(metadata.encode('utf-8'))

    print(f"Metadata appended at offset {metadata_offset}.")

def generate_metadata(input_files, versions, comments):
    """
    Generates metadata content as a JSON string.
    :param input_files: List of input file paths.
    :param versions: List of firmware versions.
    :param comments: List of comments for each slot.
    :return: JSON-formatted string containing metadata.
    """
    metadata = {"slots": []}
    slot_id = 10

    for index, (file, version) in enumerate(zip(input_files, versions)):
        # Get file size
        img_size = os.path.getsize(file)

        # Use the provided comment if available, otherwise use "No comments"
        comment = comments[index] if index < len(comments) else "No comments"

        metadata["slots"].append({
            "slot_id": slot_id,
            "file": os.path.basename(file),
            "size": img_size,
            "version": version,
            "comment": comment
        })

        slot_id += 1

    return json.dumps(metadata, indent=4)

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Sign and merge multiple binary files using MCUBoot imgtool.")
    parser.add_argument("input_files", nargs='+', help="Paths to input binary files to be signed.")
    parser.add_argument(
        "--versions",
        nargs='+',
        help="Firmware versions for each input file (e.g., '1.2.2 1.3.0')."
    )
    parser.add_argument(
        "--comments",
        nargs='+',
        help="Comments for each slot (e.g., 'Primary firmware' 'Secondary firmware')."
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

    if len(args.input_files) != len(args.versions):
        raise ValueError("Number of input files must match number of versions.")

    # Ensure comments match the number of input files
    if args.comments and len(args.comments) != len(args.input_files):
        raise ValueError("Number of comments must match number of input files.")

    os.makedirs(args.output_dir, exist_ok=True)

    signed_files = []
    slot_sizes = []
    offset = 0

    for index, input_file in enumerate(args.input_files):
        output_file = os.path.join(args.output_dir, f"signed_output_{index + 1}.bin")

        # Calculate slot size with alignment and additional space
        file_size = os.path.getsize(input_file)
        aligned_size = align_size(file_size + EXTRA_SPACE, ALIGNMENT_BOUNDARY)
        slot_sizes.append(aligned_size)

        ram_load_address = hex(offset)

        try:
            sign_binary(input_file, output_file, aligned_size, args.versions[index], ram_load_address)
            signed_files.append(output_file)
        except Exception as e:
            print(f"Failed to process {input_file}: {e}")

        offset += aligned_size

    # Generate metadata
    comments = args.comments if args.comments else ["No comments" for _ in args.input_files]
    metadata = generate_metadata(args.input_files, args.versions, comments)

    # Merge all signed binaries into one and append metadata
    merge_binaries(args.merged_output, signed_files, slot_sizes, metadata)
    print(f"Merged binary created: {args.merged_output}")
