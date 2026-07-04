#!/bin/bash

echo "========================================"
echo "    lantransfer Installation Script     "
echo "========================================"
echo ""

echo "[!] WARNING: If lantransfer is currently running, it must be stopped to update."
read -p "    Do you approve killing any running instances? (y/n): " approve_kill
if [[ "$approve_kill" != "y" && "$approve_kill" != "Y" ]]; then
    echo "    Installation aborted."
    exit 0
fi

echo ""
echo "[*] Cleaning up old instances..."
if pgrep -x "lantransfer" > /dev/null; then
    echo "    Killing running background processes..."
    pkill -x "lantransfer"
fi
if systemctl is-active --quiet lantransfer.service 2>/dev/null; then
    echo "    Stopping existing systemd service (requires sudo)..."
    sudo systemctl stop lantransfer.service
fi
echo "    Cleaned up old instances."
echo ""

echo "[*] Compiling lantransfer..."
if ! make -f build.mk; then
    echo "[!] Compilation failed! Please check the errors above."
    exit 1
fi
echo "[+] Compilation successful!"
echo ""

echo "[*] Checking configuration file..."
if [ -f "lantransfer.json" ]; then
    echo "    [+] lantransfer.json exists. Keeping your configuration intact."
    if [ -f "lantransfer.example.json" ]; then
        echo "    Cleaning up unused example file..."
        rm -f lantransfer.example.json
    fi
else
    if [ -f "lantransfer.example.json" ]; then
        echo "    lantransfer.json not found. Renaming example file to lantransfer.json..."
        mv lantransfer.example.json lantransfer.json
        echo "    [+] lantransfer.json created! Please edit it to suit your network."
    else
        echo "    [!] Warning: lantransfer.example.json is missing!"
    fi
fi

echo ""
echo "========================================"
echo "          Installation Complete!        "
echo "========================================"
echo "You can run lantransfer locally using: ./lantransfer"
