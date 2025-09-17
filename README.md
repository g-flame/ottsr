# Study Timer Pro (OTTSR) 🎯

A modern, cross-platform study timer application with customizable profiles, desktop notifications, and comprehensive session tracking. Built with GTK3 for a native look and feel across Linux, Windows, and macOS.

![Study Timer Pro Screenshot](docs/screenshot.png)

## ✨ Features

### 🎨 Modern Interface
- **Beautiful gradient design** with smooth animations and transitions
- **Responsive layout** that looks great on any screen size
- **Large timer display** with easy-to-read monospace font
- **Real-time progress bars** for visual feedback
- **Native desktop integration** with system notifications

### ⚡ Study Profiles
- **Pre-built profiles**: Pomodoro (25/5), Deep Work (90/20), Short Sprint (15/3)
- **Unlimited custom profiles** with individual settings
- **Flexible timing**: 1-180 minute study sessions, 1-60 minute breaks
- **Long break support** with configurable session intervals
- **Profile statistics** tracking total time and completion rates

### 🔔 Smart Notifications
- **Desktop notifications** for session and break reminders
- **Sound alerts** with volume control
- **Auto-start option** for seamless study sessions
- **Pause/resume functionality** with accurate time tracking

### 📊 Statistics & Tracking
- **Session completion tracking** across all profiles
- **Total study time** with detailed breakdowns
- **Real-time progress visualization** with animated progress bars
- **Persistent data storage** in JSON format

## 🚀 Quick Start

### Download

**Linux:**
- [AppImage](releases/latest) (Universal, recommended)
- [.deb package](releases/latest) (Debian/Ubuntu)  
- [.rpm package](releases/latest) (Fedora/RHEL)

**Windows:**
- [Installer](releases/latest) (Recommended)
- [Portable version](releases/latest)

**macOS:**
- [.dmg installer](releases/latest)

### First Run

1. **Select a profile** or create your own
2. **Enter your study subject** for better tracking
3. **Adjust timing** if needed (study/break minutes)
4. **Click "Start Session"** and focus on your work!

The app will guide you through study sessions and breaks with notifications.

## 🛠️ Building from Source

### Prerequisites

**Linux/macOS:**
```bash
# Ubuntu/Debian
sudo apt install build-essential cmake pkg-config libgtk-3-dev libjson-glib-1.0-dev

# Fedora
sudo dnf install gcc cmake pkg-config gtk3-devel json-glib-devel

# macOS (with Homebrew)
brew install cmake pkg-config gtk+3 json-glib
```

**Windows (MSYS2):**
```bash
pacman -S mingw-w64-x86_64-toolchain mingw-w64-x86_64-cmake mingw-w64-x86_64-gtk3 mingw-w64-x86_64-json-glib
```

### Build Steps

```bash
# Clone the repository
git clone https://github.com/g-flame/ottsr.git
cd ottsr

# Create build directory
mkdir build && cd build

# Configure
cmake .. -DCMAKE_BUILD_TYPE=Release

# Build
make -j$(nproc)  # Linux/macOS
# OR
ninja            # Windows/Alternative

# Install (optional)
sudo make install
```

### Development Build

```bash
# Debug build with extra warnings
cmake .. -DCMAKE_BUILD_TYPE=Debug

# Format code (requires clang-format)
make format

# Clean rebuild
make clean-all
```

## 📁 Project Structure

```
ottsr/
├── src/
│   ├── ottsr.h          # Header with all declarations
│   └── ottsr.c          # Main implementation
├── data/
│   ├── ottsr.desktop    # Linux desktop entry
│   ├── ottsr.png        # Application icon
│   └── ottsr.1          # Man page
├── docs/                # Documentation
├── CMakeLists.txt       # Build configuration
└── .github/workflows/   # CI/CD pipelines
```

## ⚙️ Configuration

Settings are stored in JSON format at:
- **Linux**: `~/.config/ottsr/settings.json`
- **Windows**: `%APPDATA%/ottsr/settings.json`  
- **macOS**: `~/Library/Application Support/ottsr/settings.json`

### Example Configuration

```json
{
  "version": "2.0.0",
  "theme": 0,
  "sound_volume": 70,
  "minimize_to_tray": true,
  "autostart_sessions": false,
  "active_profile": 0,
  "last_subject": "Mathematics",
  "profiles": [
    {
      "name": "Pomodoro",
      "study_minutes": 25,
      "break_minutes": 5,
      "long_break_minutes": 15,
      "sessions_until_long_break": 4,
      "sound_enabled": true,
      "notifications_enabled": true,
      "total_study_time": 7200,
      "total_sessions": 48,
      "completed_sessions": 42
    }
  ]
}
```

## 🎯 Usage Tips

### Study Techniques Supported

**🍅 Pomodoro Technique**
- 25-minute focused work sessions
- 5-minute short breaks
- 15-30 minute long breaks every 4 sessions

**🧠 Deep Work**
- 90-minute intensive focus periods
- 20-30 minute recovery breaks
- Ideal for complex, creative tasks

**⚡ Time-boxing**
- Custom intervals for any workflow
- Flexible break scheduling
- Perfect for meetings, tasks, or projects

### Productivity Tips

1. **Use descriptive subjects** - Track what you're learning
2. **Respect the breaks** - Step away from your desk
3. **Review statistics** - Monitor your progress over time
4. **Customize profiles** - Match your natural rhythm
5. **Enable notifications** - Stay focused without clock-watching

## 🤝 Contributing

We welcome contributions! Here's how to get started:

### Bug Reports
- Use the [issue tracker](https://github.com/g-flame/ottsr/issues)
- Include your OS, version, and steps to reproduce
- Add screenshots if helpful

### Feature Requests
- Check existing [issues](https://github.com/g-flame/ottsr/issues) first
- Describe the use case and expected behavior
- Consider contributing a pull request!

### Development
1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Make** your changes with tests
4. **Commit** with clear messages (`git commit -m 'Add amazing feature'`)
5. **Push** to your branch (`git push origin feature/amazing-feature`)
6. **Open** a Pull Request

### Code Style
- Follow existing formatting (use `make format`)
- Add comments for complex logic
- Test on multiple platforms when possible
- Update documentation for new features

## 📜 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **Original concept** by g-flame
- **GTK rewrite** modernized by Claude AI
- **Icons** from the [Feather](https://feathericons.com/) icon set
- **Inspiration** from the Pomodoro Technique® by Francesco Cirillo

## 📚 Changelog

### v2.0.0 (Current)
- 🎉 Complete rewrite with GTK3
- ✨ Cross-platform support (Linux, Windows, macOS)
- 🎨 Modern gradient UI with animations
- 📱 Multiple pre-built study profiles
- 🔔 Native desktop notifications
- 📊 Enhanced statistics tracking
- ⚙️ JSON configuration system
- 🏗️ Professional CMake build system

### v1.0.0 (Legacy)
- Windows-only Win32 application
- Basic timer functionality
- Simple configuration file
- Limited customization options

---

**Made with ❤️ for productive studying**

[⬆️ Back to top](#study-timer-pro-ottsr-)

<!-- Badges -->
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Build Status](https://github.com/g-flame/ottsr/workflows/Build%20and%20Release/badge.svg)](https://github.com/g-flame/ottsr/actions)
[![GitHub release (latest by date)](https://img.shields.io/github/v/release/g-flame/ottsr)](https://github.com/g-flame/ottsr/releases/latest)
[![GitHub downloads](https://img.shields.io/github/downloads/g-flame/ottsr/total)](https://github.com/g-flame/ottsr/releases)
