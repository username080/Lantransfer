#!/bin/bash
echo "Hello from the remote server execution script!"
echo "Current Directory: $(pwd)"
echo "Whoami: $(whoami)"

echo ""
echo "Creating some dummy data fetch operation..."
sleep 1

DATA="{'status': 'success', 'timestamp': '$(date)'}"
echo "Fetched Data: $DATA"
echo "Script finished executing remotely."
