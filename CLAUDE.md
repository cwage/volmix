# Volume - Per-Application Audio Control Tool

## Project Overview

A lightweight system tray application for Linux that provides per-application volume control through PulseAudio. Inspired by `volumeicon` but extended to show individual audio streams from running applications.

## Core Requirements

### Functional Requirements
- **System Tray Integration**: Display as an icon in the system tray
- **Application Discovery**: Automatically detect applications currently playing audio via PulseAudio
- **Per-App Volume Control**: Individual volume sliders for each audio-producing application
- **Dynamic Menu**: Menu updates in real-time as applications start/stop audio playback
- **Mouse Interaction**: 
  - Left click: Open application list menu
  - Scroll wheel: Adjust master volume (like volumeicon)
  - Per-app sliders: Click to set, mouse wheel to adjust
- **Lightweight**: Minimal memory footprint and CPU usage

### Technical Requirements
- **Language**: C (for performance and simplicity)
- **GUI Framework**: GTK3 (for desktop environment compatibility)
- **Audio Backend**: PulseAudio client API
- **Build System**: Autotools (autoconf/automake)
- **Portability**: Support major Linux desktop environments (GNOME, KDE, XFCE, etc.)

## Architecture Design

### Core Components
1. **Main Application (`volume.c`)**
   - GTK application initialization
   - System tray icon management
   - Event loop and signal handling

2. **PulseAudio Client (`pulse_client.c`)**
   - Connection to PulseAudio daemon
   - Sink input enumeration (applications with audio streams)
   - Volume control operations
   - Real-time stream monitoring

3. **GUI Manager (`gui.c`)**
   - System tray icon creation and updates
   - Dynamic popup menu generation
   - Volume slider widgets
   - Mouse event handling

4. **Configuration (`config.c`)**
   - User preferences (icon theme, update intervals, etc.)
   - Settings persistence

### Data Structures
```c
typedef struct {
    uint32_t index;           // PulseAudio sink input index
    char *name;               // Application name
    char *process_name;       // Process name for icon lookup
    pa_cvolume volume;        // Current volume levels
    int muted;                // Mute state
} app_audio_t;

typedef struct {
    pa_context *pa_ctx;       // PulseAudio context
    pa_mainloop_api *pa_api;  // PulseAudio mainloop API
    GtkWidget *tray_icon;     // System tray icon
    GtkWidget *popup_menu;    // Application list menu
    GList *audio_apps;        // List of app_audio_t
} volume_app_t;
```

## Dependencies

### Required Libraries
- `libpulse-dev` - PulseAudio client library
- `libgtk-3-dev` - GTK3 GUI framework
- `libglib2.0-dev` - GLib utilities

### Optional Libraries
- `libnotify-dev` - Desktop notifications (for volume change feedback)

## Build Requirements

### Debian/Ubuntu
```bash
sudo apt install build-essential autotools-dev autoconf automake libpulse-dev libgtk-3-dev libglib2.0-dev pkg-config
```

**Note**: Commands requiring sudo should be run by the user, not automatically executed.

### Build Process
```bash
./autogen.sh
./configure
make
sudo make install
```

## Project Structure
```
volume/
├── src/
│   ├── volume.c          # Main application
│   ├── pulse_client.c    # PulseAudio integration
│   ├── pulse_client.h
│   ├── gui.c            # GUI management
│   ├── gui.h
│   ├── config.c         # Configuration handling
│   └── config.h
├── data/
│   └── icons/           # Application icons
├── configure.ac         # Autoconf configuration
├── Makefile.am         # Automake configuration
├── autogen.sh          # Build script
└── README.md           # User documentation
```

## Design Principles

### Inspired by volumeicon
- **Simplicity**: Clean, minimal interface
- **Reliability**: Robust error handling and recovery
- **Portability**: Work across different desktop environments
- **Performance**: Low resource usage

### Extensions Beyond volumeicon
- **Per-Application Control**: Individual volume management
- **Real-time Updates**: Dynamic application list
- **Enhanced Interaction**: Mouse wheel support on sliders

## Development Standards

### Code Style
- Follow volumeicon's coding conventions
- Use consistent indentation (4 spaces)
- Clear variable and function naming
- Comprehensive error handling

### Testing
- Manual testing across desktop environments
- Memory leak detection with valgrind
- PulseAudio connection stability testing
- **Important**: Always test GUI applications with `&` to run in background (e.g. `./src/volume &`) to prevent CLI hanging

### Documentation
- Inline code documentation
- Man page for installation and usage
- README with build and usage instructions

## Future Enhancements (Optional)
- Application icon display in menu
- Volume level indicators (mute states, peak levels)
- Keyboard shortcuts for common operations
- Configuration GUI for advanced settings
- Support for multiple audio devices/sinks