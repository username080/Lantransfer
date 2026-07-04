#!/bin/bash

# Create some test files
mkdir -p test_data/subdir
echo "Hello World" > test_data/file1.txt
echo "Subdir file" > test_data/subdir/file2.txt

# Configure for server
cat <<EOF > lantransfer.json
{
  "mode": "server",
  "port": 8080,
  "server_ip": "127.0.0.1",
  "username": "$(whoami)",
  "client_cache_dir": "client_cache",
  "server_cache_dir": ""
}
EOF

# Start server in background
./lantransfer &
SERVER_PID=$!
sleep 1

# Configure for client
cat <<EOF > lantransfer.json
{
  "mode": "client",
  "port": 8080,
  "server_ip": "127.0.0.1",
  "username": "$(whoami)",
  "client_cache_dir": "client_cache",
  "server_cache_dir": ""
}
EOF

# Send files
./lantransfer send test_data

sleep 1
echo "Checking server received files:"
find /home/$(whoami)/lantransfercache -type f

# Get files
mkdir -p client_cache
./lantransfer get test_data

sleep 1
echo "Checking client received files:"
find client_cache -type f

# Kill server
kill $SERVER_PID
