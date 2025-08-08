# volmix - Per-Application Audio Control

A lightweight system tray application for Linux that provides per-application volume control through PulseAudio. Inspired by `volumeicon` but extended to show individual audio streams from running applications.

## Features

- **System Tray Integration**: Displays as an icon in the system tray
- **Per-Application Control**: Individual volume sliders for each audio-producing application
- **Real-time Updates**: Dynamic menu updates as applications start/stop audio playback
- **Mouse Interaction**: 
  - Left click: Open application list menu
  - Right click: Context menu
  - Scroll wheel: Adjust master volume (like volumeicon)
- **Lightweight**: Minimal memory footprint and CPU usage

## Dependencies

### Required Libraries
- `libpulse-dev` - PulseAudio client library
- `libgtk-3-dev` - GTK3 GUI framework  
- `libglib2.0-dev` - GLib utilities
- `pkg-config` - Package configuration tool
- `build-essential` - Compilation tools
- `autotools-dev autoconf automake` - Build system

### Installation on Debian/Ubuntu
```bash
sudo apt install build-essential autotools-dev autoconf automake \
                 libpulse-dev libgtk-3-dev libglib2.0-dev pkg-config
```

## Building

1. **Generate build configuration:**
   ```bash
   ./autogen.sh
   ```

2. **Configure the build:**
   ```bash
   ./configure
   ```

3. **Compile the application:**
   ```bash
   make
   ```

4. **Install (optional):**
   ```bash
   sudo make install
   ```

## Usage

### Running from Build Directory
```bash
./src/volume &
```

### Running after Installation
```bash
volume &
```

The `&` runs the application in the background, allowing you to continue using the terminal.

## Controls

- **Left Click**: Open application menu (currently shows basic message)
- **Right Click**: Context menu with Quit option
- **Scroll Wheel**: Master volume control (placeholder - prints to console)
- **Ctrl+C**: Quit application (when run in foreground)

## Current Status

This is the initial development version with basic system tray functionality. The following features are implemented:

- ✅ System tray icon display
- ✅ Mouse event handling
- ✅ Basic menu system
- ✅ Clean shutdown handling

**Coming Next:**
- PulseAudio integration for application detection
- Dynamic volume sliders for each audio application
- Master volume control via scroll wheel
- Application icons in menu

## Development

The project uses standard autotools build system and follows C coding conventions. See `CLAUDE.md` for detailed development guidelines and architecture information.

### Testing

Always test GUI applications in background mode to prevent terminal hanging:
```bash
./src/volume &
```

To stop the application:
```bash
pkill volume
```

## License

TBD

## Contributing

This project is in early development. Please check the issue tracker and `CLAUDE.md` for development guidelines.