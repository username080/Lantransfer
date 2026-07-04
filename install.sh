#!/bin/bash

echo "========================================"
echo "    lantransfer Installation Script     "
echo "========================================"
echo ""

has_service=0
if [ -f /etc/systemd/system/lantransfer.service ]; then
    has_service=1
fi

echo "[*] Checking for running instances of lantransfer..."
if systemctl is-active --quiet lantransfer.service 2>/dev/null; then
    echo "    Stopping existing systemd service (requires sudo)..."
    sudo systemctl stop lantransfer.service
fi

if pgrep -x "lantransfer" > /dev/null; then
    echo "    Killing running background processes..."
    pkill -x "lantransfer"
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

if [ $has_service -eq 1 ]; then
    echo "[*] Existing service detected. Restarting lantransfer systemd service..."
    sudo systemctl daemon-reload
    sudo systemctl start lantransfer.service
    echo "    [+] Service successfully restarted! (Running in background)"
else
    echo "[*] Service Setup"
    read -p "Do you want to install lantransfer as a background systemd service? (y/n): " install_svc
    if [[ "$install_svc" == "y" || "$install_svc" == "Y" ]]; then
        DIR=$(pwd)
        USER=$(whoami)
        SVC_PATH="/etc/systemd/system/lantransfer.service"
        
        echo "    Generating service file..."
        cat <<EOF > /tmp/lantransfer.service
[Unit]
Description=lantransfer File Transfer Server
After=network.target

[Service]
Type=simple
User=$USER
WorkingDirectory=$DIR
ExecStart=$DIR/lantransfer
Restart=always
RestartSec=3

[Install]
WantedBy=multi-user.target
EOF

        echo "    Installing service to $SVC_PATH (requires sudo)..."
        sudo mv /tmp/lantransfer.service $SVC_PATH
        sudo systemctl daemon-reload
        sudo systemctl enable lantransfer.service
        sudo systemctl start lantransfer.service
        echo "    [+] Service installed and started! (It will now auto-start on boot)"
    else
        echo "    Skipping service installation."
    fi
fi

echo ""
echo "========================================"
echo "          Installation Complete!        "
echo "========================================"
echo "You can run lantransfer locally using: ./lantransfer"
