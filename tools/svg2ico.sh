#!/bin/bash

# Check if input SVG file is provided
if [ -z "$1" ]; then
    echo "Usage: $0 <input.svg> [output.ico]"
    exit 1
fi

INPUT_SVG=$1
OUTPUT_ICO=${2:-icon.ico}

# Temporary directory for PNG files
TMP_DIR=$(mktemp -d)
trap 'rm -rf -- "$TMP_DIR"' EXIT

# Sizes for the ICO file
SIZES=(16 24 32 48 64 128 256)

# Convert SVG to PNG at different sizes
for SIZE in "${SIZES[@]}"; do
    rsvg-convert -w "$SIZE" -h "$SIZE" "$INPUT_SVG" -o "$TMP_DIR/icon_${SIZE}.png"
done

# Convert PNG files to ICO
convert "$TMP_DIR/icon_16.png" "$TMP_DIR/icon_24.png" "$TMP_DIR/icon_32.png" \
        "$TMP_DIR/icon_48.png" "$TMP_DIR/icon_64.png" "$TMP_DIR/icon_128.png" \
        "$TMP_DIR/icon_256.png" "$OUTPUT_ICO"

echo "ICO file created: $OUTPUT_ICO"
