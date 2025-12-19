# Contributing to ESP32-CAM Robust Video Stream Server

Thank you for your interest in contributing! This document provides guidelines for contributing to the project.

## How to Contribute

### Reporting Bugs

1. Check if the bug has already been reported in [Issues](https://github.com/kevin-william/esp32-robust-video-stream-server/issues)
2. If not, create a new issue with:
   - Clear description of the problem
   - Steps to reproduce
   - Expected vs actual behavior
   - Hardware details (board model, PSRAM, etc.)
   - Serial monitor output
   - Software versions (Arduino IDE, PlatformIO, libraries)

### Suggesting Features

1. Check existing issues and discussions
2. Create a new issue with:
   - Clear description of the feature
   - Use case and benefits
   - Proposed implementation (if any)
   - Potential drawbacks or concerns

### Pull Requests

1. **Fork the repository**
2. **Create a feature branch**
   ```bash
   git checkout -b feature/your-feature-name
   ```

3. **Make your changes**
   - Follow the code style (see below)
   - Add comments for complex logic
   - Update documentation if needed

4. **Test your changes**
   - Build successfully with PlatformIO
   - Test on actual hardware if possible
   - Verify no memory leaks
   - Check for regressions

5. **Commit your changes**
   ```bash
   git add .
   git commit -m "Add: Brief description of changes"
   ```

6. **Push and create PR**
   ```bash
   git push origin feature/your-feature-name
   ```
   - Create pull request on GitHub
   - Provide clear description
   - Link related issues
   - Include test results

## Code Style

### General Guidelines

- Use **4 spaces** for indentation (not tabs)
- Maximum line length: **100 characters**
- Follow existing code patterns
- Use meaningful variable names
- Add comments for non-obvious logic

### C++ Style

```cpp
// Good
void initCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    // ...
}

// Bad
void InitCamera(){
  camera_config_t Config;
  Config.ledc_channel=LEDC_CHANNEL_0;
  //...
}
```

### Formatting

We use `clang-format` with Google style (see `.clang-format`):

```bash
# Format all source files
find src include -name "*.cpp" -o -name "*.h" | xargs clang-format -i
```

### Naming Conventions

- **Functions**: `camelCase` or `snake_case` (be consistent)
- **Variables**: `snake_case` for locals, `camelCase` for members
- **Constants**: `UPPER_CASE` with underscores
- **Classes/Structs**: `PascalCase`
- **Files**: `snake_case.cpp` or `kebab-case.cpp`

Example:
```cpp
#define MAX_BUFFER_SIZE 1024

struct CameraConfig {
    int frame_size;
    int quality;
};

bool initCamera() {
    int retry_count = 0;
    // ...
}
```

## Project Structure

```
esp32-robust-video-stream-server/
├── src/                    # Source files (.cpp)
│   ├── main.cpp           # Main application
│   ├── camera.cpp         # Camera operations
│   ├── web_server.cpp     # Web server and API
│   ├── config.cpp         # Configuration management
│   ├── storage.cpp        # SD card and NVS
│   └── captive_portal.cpp # WiFi setup portal
├── include/               # Header files (.h)
│   ├── app.h
│   ├── camera_pins.h
│   ├── config.h
│   ├── web_server.h
│   ├── storage.h
│   └── captive_portal.h
├── docs/                  # Documentation
│   ├── API.md            # API reference
│   ├── ARCHITECTURE.md   # Architecture docs
│   └── BUILD.md          # Build instructions
├── data/                  # Data files
│   ├── config/           # Configuration examples
│   └── www/              # Web UI files
├── scripts/              # Build and utility scripts
├── platformio.ini        # PlatformIO configuration
└── README.md             # Main documentation
```

## Development Workflow

### Setting Up Development Environment

1. **Clone repository**
   ```bash
   git clone https://github.com/kevin-william/esp32-robust-video-stream-server.git
   cd esp32-robust-video-stream-server
   ```

2. **Install PlatformIO**
   ```bash
   pip install platformio
   ```

3. **Build project**
   ```bash
   pio run
   ```

4. **Upload to device**
   ```bash
   pio run --target upload
   ```

### Testing

#### Manual Testing

- Test all API endpoints
- Verify stream quality and FPS
- Check memory usage over time
- Test WiFi reconnection
- Verify configuration persistence
- Test captive portal flow

#### Automated Testing

Currently minimal automated testing. Planned:
- Unit tests for utilities
- Integration tests for API
- Memory leak detection

### Documentation

When adding features:
1. Update relevant documentation in `docs/`
2. Update API.md if adding/changing endpoints
3. Update README.md if changing setup/usage
4. Add examples if helpful

## Commit Messages

Use clear, descriptive commit messages:

```
Add: Feature description
Fix: Bug description
Update: What was updated
Refactor: What was refactored
Docs: Documentation changes
```

Examples:
```
Add: OTA update endpoint with HTTPS support
Fix: Memory leak in frame buffer release
Update: Increase watchdog timeout to 10s
Refactor: Extract WiFi logic to separate module
Docs: Add troubleshooting section to README
```

## Code Review Process

1. PR is submitted
2. Automated checks run (build, lint)
3. Maintainer reviews code
4. Feedback provided if needed
5. Changes incorporated
6. PR approved and merged

## Areas for Contribution

Looking for help with:

### High Priority
- OTA firmware update implementation
- HTTPS support with certificate management
- Unit tests for core modules
- Performance optimization
- Memory usage improvements

### Medium Priority
- Motion detection
- Face detection (with PSRAM)
- Time-lapse recording
- Multi-language web UI
- Mobile app integration

### Low Priority
- WebSocket support
- Cloud integration (AWS, Azure, Firebase)
- Advanced image processing
- Video codec improvements (H.264)

## Questions?

- Open an issue for discussion
- Check existing issues and PRs
- Review documentation in `docs/`

## License

By contributing, you agree that your contributions will be licensed under the Apache License 2.0.

---

Thank you for contributing! 🎉
