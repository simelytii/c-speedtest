# C Speedtest

## Description
This project is a C implementation of an internet speed test application. The program selects a speed test server, measures download/upload speed and supports automatic server selection based on user location.

## Dependencies

Required libraries:
- libcurl
- cJSON

On Ubuntu:

```bash
sudo apt install libcurl4-openssl-dev libcjson-dev
```

On macOS:

```bash
brew install curl cjson
```

## Features
- Download speed test
- Upload speed test
- Automatic server selection
- Location detection
- Command line arguments
- JSON server list parsing

## Build

Compile the project:
```bash
make
```

Clean build files:
```bash
make clean
```

## Usage

Download:
```bash
./speedtest --download --server ID
```

Upload:
```bash
./speedtest --upload --server ID
```

Automatic:
```bash
./speedtest --auto
```

Show help:
```bash
./speedtest --help
```

## Project structure

```text
speedtest/
├── include/
│   ├── config.h
│   ├── download.h
│   ├── location.h
│   ├── server.h
│   └── upload.h
├── src/
│   ├── download.c
│   ├── location.c
│   ├── main.c
│   ├── server.c
│   └── upload.c
├── Makefile
├── README.md
└── speedtest_server_list.json
```

## Example output

```text
./speedtest --upload --server 9714

Selected server:
Country: Abkhaziya
City: Sukhum
Provider: A-Mobile
Host: speedtest.a-mobile.biz:8080

Running tests...

Upload test started
Using server: speedtest.a-mobile.biz:8080
Starting upload...
Upload test finished

========== SPEEDTEST RESULT ==========
Server: speedtest.a-mobile.biz:8080
Country: Abkhaziya
Upload speed: 91.78 Mbps
======================================
```