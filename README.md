# RGBStreamer

RGBStreamer is a Windows application that captures screen content, processes it to extract average RGB values, and sends this data via UDP to configured network devices. It's designed for real-time color streaming applications.

## Features

- **Screen Capture**: Captures screen content from any monitor using DirectX 11
- **RGB Processing**: Extracts average RGB values from captured frames
- **UDP Streaming**: Sends RGB data to multiple network devices simultaneously
- **Configurable**: JSON-based configuration for devices, capture interval, and data format
- **Logging**: Comprehensive logging system with timestamped log files
- **Multi-threaded**: Separate threads for capture, processing, and network sending

## System Requirements

- **Windows 10/11** (64-bit)
- **DirectX 11** (usually pre-installed on Windows 10/11)
- **Visual C++ Redistributable 2015-2022** (may be required)

## Installation

### For End Users

1. **Download** the RGBStreamer executable
2. **Create a config.json** file (see Configuration section below)
3. **Place both files** in the same directory
4. **Run** `RGBStreamer.exe config.json`

### Visual C++ Redistributable Installation

If you encounter "missing DLL" errors, install the Visual C++ Redistributable:
1. Download from Microsoft: https://aka.ms/vs/17/release/vc_redist.x64.exe
2. Run the installer
3. Restart the application

## Configuration

Create a `config.json` file with the following structure:

```json
{
  "captureIntervalMs": 33,
  "devices": [
    { "ip": "192.168.1.100", "port": 6000 },
    { "ip": "192.168.1.101", "port": 6000 }
  ],
  "format": "R{r:03d}G{g:03d}B{b:03d}\n"
}
```

### Configuration Parameters

- **captureIntervalMs**: Time between captures in milliseconds (default: 33ms = ~30 FPS)
- **devices**: Array of target devices to receive UDP data
  - **ip**: Target device IP address
  - **port**: Target device UDP port
- **format**: Data format string with placeholders:
  - `{r}`, `{g}`, `{b}`: RGB values (0-255)
  - `{r:03d}`, `{g:03d}`, `{b:03d}`: Zero-padded RGB values (e.g., 001, 255)

## Usage

### Command Line

```bash
# Basic usage
RGBStreamer.exe --config=config.json
```

### Monitor Selection

When you run the application:
1. It will display available monitors
2. Select the monitor to capture (0, 1, 2, etc.)
3. Press Enter to start capture
4. Press Ctrl+C to stop

### Example Output

```
=== Available Monitors ===
   Index |                Name |          Device | Resolution
----------------------------------------------------------------------
       0 | Monitor 1 (1920x1080) | \\.\DISPLAY1 | 1920x1080
       1 | Monitor 2 (2560x1440) | \\.\DISPLAY2 | 2560x1440

Select monitor to capture (0-1): 0
Selected monitor 0: Monitor 1 (1920x1080)

Starting capture from monitor 0...
Press Ctrl+C to stop.
```

## Logging

The application creates detailed logs in a `logs` directory:

- **Log files**: `logs/rgbstreamer_YYYYMMDD_HHMMSS.log`
- **Log content**: Capture operations, UDP sends, network errors, and system events
- **Format**: `[Timestamp] [Category] Message`

### Log Categories

- `[CAPTURE]`: Screen capture operations
- `[UDP]`: UDP sending operations
- `[NETWORK ERROR]`: Network connection errors
- General system events

## Troubleshooting

### Common Issues

1. **"Missing DLL" errors**
   - Install Visual C++ Redistributable 2015-2022

2. **"Failed to initialize capture"**
   - Ensure DirectX 11 is available
   - Check if the selected monitor exists

3. **"Failed to open UDP sender"**
   - Check Windows Firewall settings
   - Ensure no other application is using the same port

4. **"No monitors found"**
   - Ensure monitors are properly connected
   - Check display driver installation

### Performance Tips

- **Lower capture interval** for higher frame rates (but more CPU usage)
- **Use wired network** for better UDP reliability
- **Close unnecessary applications** to reduce system load

## Network Protocol

The application sends UDP packets with the configured format string. Each packet contains:
- RGB values (0-255)
- Formatted according to the `format` parameter
- Sent to all configured devices

### Example UDP Data

With format `"R{r:03d}G{g:03d}B{b:03d}\n"`:
- Red=255, Green=128, Blue=64 → `"R255G128B064\n"`

## Support

For issues and questions:
1. Check the log files in the `logs` directory
2. Review the troubleshooting section