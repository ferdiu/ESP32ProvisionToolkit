# Changelog

All notable changes to the ESP32ProvisionToolkit library will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [1.0.1] - 2026-01-30

### Documentation
- README.md updated with missing information

## [1.0.0] - 2026-01-30

### Added
- Initial release of ESP32ProvisionToolkit library
- Non-blocking WiFi provisioning state machine
- Captive portal with responsive web interface
- Persistent credential storage in NVS
- Hardware button reset mechanism with configurable duration
- Simple HTTP reset endpoint
- Authenticated HTTP reset with SHA-256 password hashing
- Double-reboot detection for emergency recovery
- Auto-wipe on max connection retries
- LED status indicator with multiple patterns
- mDNS support for easy device discovery
- Configurable logging levels (None/Error/Info/Debug)
- Callback system for WiFi events (connected, failed, AP mode, reset)
- Fluent configuration API
- Manual credential management methods
- Add custom routes in selected modes (AP and/or Connected)
- Complete API documentation
- Four example sketches (Basic, SecureReset, Headless, CustomRoutes)
- Security guide and best practices
- Integration guide with common use cases
- Quick reference card

### Features Implemented
- **Provisioning**: Automatic WiFi scanning, one-click network selection, mobile-friendly UI
- **Reset Mechanisms**: 4 different reset methods (hardware, HTTP simple, HTTP authenticated, double-reboot)
- **Security**: Password hashing, authenticated operations, configurable AP security
- **UX**: Visual LED feedback, mDNS support, responsive web design
- **Robustness**: Retry logic, timeout handling, state persistence, non-blocking operation

### Documentation
- README.md with complete feature overview and quick start
- QUICK_REFERENCE.md for common operations
- extras/API_REFERENCE.md with complete method documentation
- extras/INTEGRATION_GUIDE.md with real-world examples
- Comprehensive code comments
- Four fully-documented example sketches

### Technical Details
- Memory efficient: ~2KB base overhead, ~15KB during provisioning
- Non-blocking state machine for responsive operation
- Standard Arduino library structure
- Compatible with all ESP32 variants
- PlatformIO support included

---

## Version History Template

### [X.Y.Z] - YYYY-MM-DD

#### Added
- New features

#### Changed
- Changes to existing functionality

#### Deprecated
- Features marked for removal in future versions

#### Removed
- Removed features

#### Fixed
- Bug fixes

#### Security
- Security improvements or vulnerability fixes

---

## Maintenance Notes

### Breaking Changes Policy
- Major version (X.0.0): Breaking API changes allowed
- Minor version (0.Y.0): New features, no breaking changes
- Patch version (0.0.Z): Bug fixes only, no breaking changes

### Deprecation Policy
- Features will be deprecated for at least one minor version before removal
- Deprecated features will log warnings when used
- Migration guides will be provided for breaking changes

### Security Updates
- Security fixes will be backported to the last major version
- Critical security issues will trigger immediate patch releases

---

## Contributing

When contributing, please:
1. Update this CHANGELOG with your changes
2. Follow the format specified above
3. Place changes under "Unreleased" section
4. Categorize changes appropriately (Added, Changed, Fixed, etc.)
5. Reference issue/PR numbers where applicable

Example:
```markdown
### Added
- Add support for WiFi priority lists (#123)
- Implement exponential backoff retry strategy (#124)

### Fixed
- Fix memory leak in web server shutdown (#125)
- Correct LED blink timing accuracy (#126)
```

---

## Links

- [Repository](https://github.com/ferdiu/ESP32ProvisionToolkit)
- [Issues](https://github.com/ferdiu/ESP32ProvisionToolkit/issues)
- [Releases](https://github.com/ferdiu/ESP32ProvisionToolkit/releases)
