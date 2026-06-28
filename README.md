# Lantransfer

Lantransfer is a lightweight, C-based command-line utility for transferring files and directories over a Local Area Network (LAN). It operates on a simple Client/Server architecture, allowing you to quickly send and receive files using JSON configuration.

## Features

- **Client/Server Architecture:** Run in either server mode to listen for incoming connections or client mode to send/request files.
- **Directory & File Support:** Seamlessly transfers individual files or entire directory trees.
- **JSON Configuration:** Easy-to-use configuration file (`lantransfer.json`) for managing network and environment settings.
- **Timestamped Cache:** Automatically organizes received files into timestamped cache directories, preventing accidental overwrites.
- **Lightweight & Fast:** Written in pure C with minimal dependencies.

## Building the Project

Lantransfer uses a standard Makefile. To build the project, simply run:

```bash
make
```

This will compile the source code and generate the `lantransfer` executable.
To clean up the build files, run:

```bash
make clean
```

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
- `server_cache_dir`: Directory where the server will save incoming files. (Default: `/home/<username>/Lantransfercache`)

## Usage

Ensure `lantransfer.json` is properly configured for the respective machine before running.

### 1. Start the Server

On the receiving machine, configure `"mode": "server"` in your JSON file and run:

```bash
./lantransfer
```

The server will start listening on the configured port.

### 2. Client Operations

On the sending/requesting machine, configure `"mode": "client"` in your JSON file.

**To send a file or directory to the server:**
```bash
./lantransfer send <local_path>
```

**To request (get) a file or directory from the server:**
```bash
./lantransfer get <remote_path>
```

## How It Works

- When files are transferred, they are saved inside the configured cache directory within a subdirectory named with the current timestamp (e.g., `client_cache/2026-06-28_15-06-30/`). This prevents filename collisions.
- Progress bars are displayed during the transfer of individual files.

## License

This project is open-source. Feel free to modify and distribute it as needed.