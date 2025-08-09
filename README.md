# volmix - Per-Application Audio Control

A lightweight system tray application for Linux that provides per-application volume control through PulseAudio. Inspired by `volumeicon` but extended to show individual audio streams from running applications.

## Features

- **System Tray Integration**: Clean system tray icon with intuitive interaction
- **Per-Application Control**: Individual volume sliders for each audio-producing application
- **Real-time Updates**: Dynamic discovery of applications playing audio via PulseAudio
- **Smart Interaction**: 
  - Left click: Toggle volume control window (show/hide)
  - Right click: Context menu with quit option
  - Mouse wheel: Adjust master volume
- **Streamlined Interface**: Clean UI without redundant buttons
- **Lightweight**: Minimal memory footprint and CPU usage

## Installation

### Option 1: Download Pre-built Package (Recommended)

Download the latest `.deb` package from the [releases page](https://github.com/cwage/volmix/releases):

1. Go to the [releases page](https://github.com/cwage/volmix/releases)
2. Download the `.deb` file for your architecture:
   - `volmix_*_amd64.deb` for most desktop/laptop systems
   - `volmix_*_arm64.deb` for ARM-based systems (Raspberry Pi 4+, etc.)
3. Install the package:
   ```bash
   sudo dpkg -i volmix_*.deb
   sudo apt-get install -f  # Install any missing dependencies
   ```

**Available Architectures:**
- `amd64` (x86_64) - Most desktop/laptop systems
- `arm64` (AArch64) - ARM-based systems, Raspberry Pi 4+

### Option 2: Build from Source

#### Build Dependencies

Required libraries:
- `libpulse-dev` - PulseAudio client library
- `libgtk-3-dev` - GTK3 GUI framework  
- `libglib2.0-dev` - GLib utilities
- `pkg-config` - Package configuration tool
- `build-essential` - Compilation tools
- `autotools-dev autoconf automake` - Build system

Install build dependencies on Debian/Ubuntu:
```bash
sudo apt install build-essential autotools-dev autoconf automake \
                 libpulse-dev libgtk-3-dev libglib2.0-dev pkg-config
```

#### Building Steps

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

### Running after Installation
```bash
# Start volmix in the background
volmix &
```

### Running from Build Directory
```bash
./src/volmix &
```

The `&` runs the application in the background, allowing you to continue using the terminal.

## Controls

- **Left Click**: Toggle volume control window (show/hide)
- **Right Click**: Context menu with quit option  
- **Mouse Wheel**: Adjust master volume
- **Window Close**: Use window controls or click tray icon to hide
- **Ctrl+C**: Quit application (when run in foreground)

## Requirements

- **PulseAudio**: Audio system integration
- **GTK3**: GUI framework  
- **Linux Desktop**: System tray support (GNOME, KDE, XFCE, etc.)

## Current Status

This is a functional release with complete per-application volume control:

- ✅ System tray integration with toggle behavior
- ✅ Real-time PulseAudio application detection
- ✅ Individual volume controls for each audio stream
- ✅ Master volume control via mouse wheel
- ✅ Clean, streamlined interface
- ✅ Optimized window reuse for better performance

## Development

The project uses standard autotools build system and follows C coding conventions. See `CLAUDE.md` for detailed development guidelines and architecture information.

### Testing

Always test GUI applications in background mode to prevent terminal hanging:
```bash
./src/volmix &
```

To stop the application:
```bash
pkill volmix
```

## Troubleshooting

### Common Issues

**System tray icon not appearing:**
- Ensure your desktop environment supports system tray (most do)
- Try restarting your window manager/desktop environment
- Check that `volmix` process is running: `ps aux | grep volmix`

**No applications shown in volume control:**
- Make sure applications are actively playing audio
- Verify PulseAudio is running: `pulseaudio --check`
- Check PulseAudio applications: `pactl list sink-inputs`

**Permission or dependency errors:**
- Install missing dependencies: `sudo apt-get install -f`
- Ensure user is in audio group: `sudo usermod -a -G audio $USER`

## License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## Contributing

Contributions are welcome! Please feel free to submit issues and pull requests.

- **Bug Reports**: Use the [issue tracker](https://github.com/cwage/volmix/issues)
- **Feature Requests**: Open an issue with your ideas
- **Development**: See `CLAUDE.md` for architecture and development guidelines

## Acknowledgments

Inspired by [`volumeicon`](https://github.com/Maato/volumeicon) - thanks to the original developers for the concept and design inspiration.