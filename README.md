# Deduplikate

A modern KDE6 application for finding and managing duplicate files, powered by the [czkawka](https://github.com/qarmin/czkawka) backend.

![License](https://img.shields.io/badge/license-GPL--3.0-blue.svg)

## Features

- **Fast Duplicate Detection**: Uses the proven czkawka_core engine for efficient file scanning
- **Multiple Detection Methods**:
  - Hash-based (Blake3, CRC32, XXH3)
  - Name-based
  - Size-based
  - Size + Name combination
- **Modern KDE6 Interface**: Native Qt6/KDE Frameworks 6 integration
- **Three-Panel Layout**: Tool selection, results view, and settings panel (similar to krokiet)
- **Flexible Options**:
  - Recursive directory scanning
  - Hard link detection
  - File size filtering
  - Include/exclude directory lists
  - Caching for faster subsequent scans
- **Tree View Results**: Organized by duplicate groups with checkboxes for selection
- **Selection Tools**: Select all, none, or invert selection
- **Progress Reporting**: Real-time scan progress with status updates

## Screenshots

*(Add screenshots here once the application is built)*

## Architecture

```
┌─────────────────┐
│   Qt6/KDE6 UI   │  (C++)
└────────┬────────┘
         │
┌────────▼────────┐
│  Rust FFI Bridge│  (Rust + cbindgen)
└────────┬────────┘
         │
┌────────▼────────┐
│  czkawka_core   │  (Rust)
└─────────────────┘
```

The application uses a Rust FFI bridge to interface with the czkawka_core library, exposing a C API that can be called from C++/Qt.

## Prerequisites

### System Requirements

- **Operating System**: Linux (KDE Plasma recommended)
- **CMake**: 3.16 or later
- **C++ Compiler**: GCC 7+ or Clang 5+ with C++17 support
- **Rust**: 1.70.0 or later (for building the FFI bridge)
- **cbindgen**: For generating C headers from Rust

### Dependencies

#### Ubuntu/Debian

```bash
sudo apt update
sudo apt install \
    cmake \
    extra-cmake-modules \
    qtbase6-dev \
    qt6-base-dev \
    libkf6coreaddons-dev \
    libkf6i18n-dev \
    libkf6xmlgui-dev \
    libkf6configwidgets-dev \
    libkf6widgetsaddons-dev \
    libkf6kio-dev \
    cargo \
    rustc
```

#### Fedora

```bash
sudo dnf install \
    cmake \
    extra-cmake-modules \
    qt6-qtbase-devel \
    kf6-kcoreaddons-devel \
    kf6-ki18n-devel \
    kf6-kxmlgui-devel \
    kf6-kconfigwidgets-devel \
    kf6-kwidgetsaddons-devel \
    kf6-kio-devel \
    cargo \
    rust
```

#### Arch Linux

```bash
sudo pacman -S \
    cmake \
    extra-cmake-modules \
    qt6-base \
    kf6-kcoreaddons \
    kf6-ki18n \
    kf6-kxmlgui \
    kf6-kconfigwidgets \
    kf6-kwidgetsaddons \
    kf6-kio \
    rust \
    cargo
```

### Install cbindgen

```bash
cargo install cbindgen
```

### Clone czkawka

The application requires the czkawka repository to be cloned next to the deduplikate directory:

```bash
cd /path/to/parent/directory
git clone https://github.com/qarmin/czkawka.git
git clone <this-repo> deduplikate
```

Your directory structure should look like:
```
parent/
├── czkawka/
│   ├── czkawka_core/
│   ├── czkawka_cli/
│   └── ...
└── deduplikate/
    ├── src/
    ├── CMakeLists.txt
    └── README.md
```

## Building

### Quick Build

```bash
cd deduplikate
mkdir build
cd build
cmake ..
make -j$(nproc)
```

### Debug Build

```bash
mkdir build-debug
cd build-debug
cmake -DCMAKE_BUILD_TYPE=Debug ..
make -j$(nproc)
```

### Release Build

```bash
mkdir build-release
cd build-release
cmake -DCMAKE_BUILD_TYPE=Release ..
make -j$(nproc)
```

## Installation

After building, install the application:

```bash
cd build
sudo make install
```

Or run directly from the build directory:

```bash
./deduplikate
```

## Usage

1. **Launch the application**: Run `deduplikate` from your application menu or terminal

2. **Select tool**: Click on "Duplicate Files" in the left panel (currently the only implemented tool)

3. **Configure settings** (right panel):
   - Choose detection method (Hash, Name, Size, or Size+Name)
   - Select hash type if using hash-based detection
   - Enable/disable options (recursive search, ignore hard links, use cache)
   - Set minimum and maximum file sizes
   - Add directories to include in the scan
   - Optionally add directories to exclude

4. **Start scan**: Click the "Scan" button

5. **View results**: Duplicate groups will appear in the center panel, organized hierarchically

6. **Select files**: Use checkboxes to select files you want to delete, or use selection buttons:
   - Select All
   - Select None
   - Invert Selection

7. **Delete** (future feature): Click "Delete Selected" to remove checked files

## Detection Methods

### Hash (Recommended)

Compares files by computing cryptographic hashes of their contents. This is the most accurate method and will detect duplicates even if filenames differ.

**Hash Types:**
- **Blake3**: Cryptographically secure, fast, recommended for most use cases
- **CRC32**: Fastest but less accurate (may have collisions)
- **XXH3**: Very fast, good balance between speed and accuracy

### Name

Finds files with identical names, optionally case-insensitive.

### Size

Groups files by their byte size. Fast but may produce false positives.

### Size + Name

Combines both size and name matching for better accuracy than size alone.

## Roadmap

- [x] Basic UI with three-panel layout
- [x] Duplicate file detection
- [x] Results display in tree view
- [x] Progress reporting
- [ ] File deletion functionality
- [ ] Move to folder functionality
- [ ] Hard link creation
- [ ] Symbolic link creation
- [ ] Export results to JSON
- [ ] Settings persistence
- [ ] Additional tools from czkawka:
  - [ ] Empty folders
  - [ ] Big files
  - [ ] Temporary files
  - [ ] Similar images
  - [ ] Similar videos
  - [ ] Similar music

## Development

### Project Structure

```
deduplikate/
├── CMakeLists.txt              # Main CMake configuration
├── README.md                   # This file
└── src/
    ├── main.cpp                # Application entry point
    ├── mainwindow.{h,cpp}      # Main window implementation
    ├── duplicatefinder.{h,cpp} # Duplicate finder logic (C++ wrapper)
    ├── duplicatemodel.{h,cpp}  # Qt model for results display
    ├── settingsdialog.{h,cpp}  # Settings dialog (future)
    └── czkawka_bridge/         # Rust FFI bridge
        ├── CMakeLists.txt      # CMake for Rust build
        ├── Cargo.toml          # Rust project manifest
        ├── cbindgen.toml       # cbindgen configuration
        ├── build.rs            # Rust build script
        └── src/
            └── lib.rs          # Rust FFI implementation
```

### FFI Bridge

The Rust FFI bridge (`czkawka_bridge`) provides a C-compatible API:

- `czkawka_duplicate_finder_new()` - Create finder instance
- `czkawka_duplicate_finder_add_directory()` - Add directory to scan
- `czkawka_duplicate_finder_search()` - Start scan
- `czkawka_duplicate_finder_get_group()` - Retrieve results
- `czkawka_duplicate_finder_free()` - Clean up

The C header is auto-generated from Rust using cbindgen.

### Contributing

Contributions are welcome! Please:

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## License

This project is licensed under the GPL-3.0 License - see the LICENSE file for details.

The czkawka_core backend is licensed under MIT License.

## Credits

- **czkawka**: [qarmin/czkawka](https://github.com/qarmin/czkawka) - The excellent duplicate file finder backend
- **KDE**: For the amazing KDE Frameworks
- **Qt**: For the Qt framework

## Support

- **Issues**: Report bugs or request features on the [issue tracker](https://github.com/yourusername/deduplikate/issues)
- **Discussions**: Join discussions on the [GitHub Discussions](https://github.com/yourusername/deduplikate/discussions)

## See Also

- [czkawka](https://github.com/qarmin/czkawka) - The original GTK4 application and CLI
- [krokiet](https://github.com/qarmin/czkawka/tree/master/krokiet) - The Slint-based GUI for czkawka
- [KDE Frameworks](https://kde.org/products/frameworks/) - Application framework documentation

---

**Note**: This is an independent project and is not officially affiliated with the czkawka or KDE projects.
