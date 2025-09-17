# Over The Top Study Reminder (OTTSR)

A professional, lightweight study timer application for Windows with Pomodoro technique support, customizable profiles, and session tracking.

## Features

- **Multiple Study Profiles**: Create and manage different study profiles for various subjects
- **Pomodoro Timer**: Configurable study and break intervals with long break support
- **Session Tracking**: Persistent statistics including total study time and completed sessions
- **Clean Interface**: Modern, distraction-free user interface
- **Audio Notifications**: Configurable sound alerts for session transitions
- **Automatic Management**: Optional auto-start for continuous study sessions
- **Portable**: Single executable with configuration file - no installation required

## Quick Start

1. Download the latest release from [Releases](https://github.com/g-flame/ottsr/releases)
2. Extract the portable package or run the installer
3. Launch `ottsr.exe`
4. Set your study subject and preferred time intervals
5. Click "Start Session" to begin studying

## Building from Source

### Prerequisites
- Windows 10/11
- Visual Studio 2019+ or Build Tools
- Windows SDK

### Compilation
```bash
git clone https://github.com/g-flame/ottsr.git
cd ottsr

# Using Visual Studio Developer Command Prompt
cl.exe /Fe:ottsr.exe /O2 /MD src/ottsr.c /link user32.lib gdi32.lib kernel32.lib comctl32.lib winmm.lib

# Or using the included batch file
build.bat
```

## Configuration

OTTSR stores all settings in `ottsr.conf`. The file is created automatically on first run, but you can manually edit it for advanced customization:

```ini
# Theme: 0=Light, 1=Dark, 2=Auto
theme=0

# Sound volume (0-100)
sound_volume=70

# Auto-start next session after break
autostart_sessions=0

# Profile settings
profile_0_name=Default
profile_0_study_minutes=25
profile_0_break_minutes=5
profile_0_long_break_minutes=15
profile_0_sessions_until_long_break=4
```

## System Requirements

- Windows 7 or later
- 1 MB free disk space
- No additional runtime dependencies

## License

MIT License - see [LICENSE](LICENSE) for details

## Credits

Developed by [g-flame](https://github.com/g-flame)

## Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## Support

- Create an [Issue](https://github.com/g-flame/ottsr/issues) for bug reports
- Check existing issues before creating new ones
- Provide system information and steps to reproduce for bugs
profile_0_long_break_minutes=15
profile_0_sessions_until_long_break=4
profile_0
