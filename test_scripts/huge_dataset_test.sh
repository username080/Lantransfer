#!/bin/bash
set -e

echo "======================================"
echo "  Lantransfer Huge Dataset Test"
echo "======================================"

# Automatically read the cache directory provided dynamically by the Lantransfer Server C code!
if [ -z "$LANTRANSFER_CACHE_DIR" ]; then
    echo "[!] Error: LANTRANSFER_CACHE_DIR is not set. Are you running this through Lantransfer?"
    exit 1
fi

DATA_DIR="$LANTRANSFER_CACHE_DIR/dataset_test"
mkdir -p "$DATA_DIR"

CSV_FILE="$DATA_DIR/massive_data.csv"
BIN_FILE="$DATA_DIR/massive_data.bin"

echo "[1/3] Generating a massive 1,000,000 row CSV file natively inside the server cache ($DATA_DIR)..."
python3 -c "
with open('$CSV_FILE', 'w') as f:
    f.write('id,sensor_value,status\n')
    for i in range(1000000):
        f.write(f'{i},{i*1.14159},active\n')
"
echo "      Done! Generated CSV Size:"
ls -lh "$CSV_FILE"

echo ""
echo "[2/3] Simulating intense CPU conversion (CSV to raw Binary)..."
python3 -c "
import struct
with open('$CSV_FILE', 'r') as f_in, open('$BIN_FILE', 'wb') as f_out:
    next(f_in) # skip header
    for line in f_in:
        parts = line.strip().split(',')
        if len(parts) == 3:
            # Pack as integer and float binary structure
            f_out.write(struct.pack('if', int(parts[0]), float(parts[1])))
"
echo "      Done! Converted Binary Size:"
ls -lh "$BIN_FILE"

echo ""
echo "[3/3] Finalizing results..."
echo "      The massive binary dataset is now permanently stored deep inside the server cache at: $BIN_FILE"
echo "======================================"
echo "Dataset Pipeline Executed Successfully!"
