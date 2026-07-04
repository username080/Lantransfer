# lantransfer

lantransfer is a lightweight, C-based command-line utility for transferring files and directories over a Local Area Network (LAN). It operates on a simple Client/Server architecture, allowing you to quickly send and receive files using JSON configuration.

## Features

- **Client/Server Architecture:** Run in either server mode to listen for incoming connections or client mode to send/request files.
- **Directory & File Support:** Seamlessly transfers individual files or entire directory trees.
- **JSON Configuration:** Easy-to-use configuration file (`lantransfer.json`) for managing network and environment settings.
- **Timestamped Cache:** Automatically organizes received files into timestamped cache directories, preventing accidental overwrites.
- **Lightweight & Fast:** Written in pure C with minimal dependencies.

## Building the Project

We have provided a guided installation script to make setup effortless. Simply run:

```bash
./install.sh
```

This script will safely compile the `lantransfer` executable and automatically copy the `lantransfer.example.json` into a usable `lantransfer.json` config file for you.

## Configuration

The application is configured via a `lantransfer.json` file located in the same directory as the executable.

### Example `lantransfer.json`:

```json
{
  "mode": "client",
  "port": 8080,
  "server_ip": "127.0.0.1",
  "username": "user",
  "client_cache_dir": "client_cache",
  "server_cache_dir": ""
}
```

### Configuration Fields:
- `mode`: Operating mode. Set to `"server"` for the receiving end, or `"client"` for the sending/requesting end.
- `port`: The TCP port to use for the connection (e.g., `8080`).
- `server_ip`: The IP address of the server (used by the client to connect).
- `username`: Your username, used to formulate the default cache directory.
- `client_cache_dir`: Directory where the client will save files requested from the server. (Default: current directory)
- `server_cache_dir`: Directory where the server will save incoming files. (Default: `/home/<username>/lantransfercache`)

## Usage

Ensure `lantransfer.json` is properly configured for the respective machine before running.

### 1. Start the Server

On the receiving machine, configure `"mode": "server"` in your JSON file and run:

```bash
./lantransfer
```
The server will start listening on the configured port.

**Running as a Daemon:**
To run the server in the background (surviving terminal closures), append the `-d` or `--daemon` flag:
```bash
./lantransfer -d
```

### Running as a Systemd Service (Survive Reboots)
To have the server start automatically whenever the machine reboots, you can configure it via `systemd`.
1. Open the included `lantransfer.service` file and update the `WorkingDirectory` and `ExecStart` fields with the absolute path to your lantransfer directory.
2. Copy the service file to systemd's directory:
   ```bash
   sudo cp lantransfer.service /etc/systemd/system/
   ```
3. Reload systemd, enable the service, and start it:
   ```bash
   sudo systemctl daemon-reload
   sudo systemctl enable lantransfer.service
   sudo systemctl start lantransfer.service
   ```

### 2. Client Operations

On the sending/requesting machine, configure `"mode": "client"` in your JSON file.

**To send a file or directory to the server:**
```bash
./lantransfer send_file <local_path>
```

**To request (get) a file or directory from the server:**
```bash
./lantransfer get_file <remote_path>
```

**To execute a command or script on the server:**
```bash
# Execute a single string command and stream output live:
./lantransfer exec_command "ls -la /tmp"

# Execute a local script file on the remote server and stream output live:
./lantransfer exec_script ./my_script.sh

# Execute and save the output locally in your client cache:
./lantransfer exec_script ./my_script.sh -c

# Execute and have the server securely log the output in its server cache:
./lantransfer exec_script ./my_script.sh -s
```

## How It Works

- When files are transferred, they are saved inside the configured cache directory within a subdirectory named with the current timestamp (e.g., `client_cache/2026-06-28_15-06-30/`). This prevents filename collisions.
- Progress bars are displayed during the transfer of individual files.

## License

This project is open-source. Feel free to modify and distribute it as needed.