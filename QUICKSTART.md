# Quick Start Guide

Get up and running with Deduplikate in 5 minutes!

## Step 1: Install Dependencies

### Ubuntu/Debian
```bash
sudo apt install cmake extra-cmake-modules qtbase6-dev \
    libkf6coreaddons-dev libkf6i18n-dev libkf6xmlgui-dev \
    libkf6configwidgets-dev libkf6widgetsaddons-dev \
    libkf6kio-dev cargo rustc

cargo install cbindgen
```

### Fedora
```bash
sudo dnf install cmake extra-cmake-modules qt6-qtbase-devel \
    kf6-kcoreaddons-devel kf6-ki18n-devel kf6-kxmlgui-devel \
    kf6-kconfigwidgets-devel kf6-kwidgetsaddons-devel \
    kf6-kio-devel cargo rust

cargo install cbindgen
```

### Arch Linux
```bash
sudo pacman -S cmake extra-cmake-modules qt6-base \
    kf6-kcoreaddons kf6-ki18n kf6-kxmlgui \
    kf6-kconfigwidgets kf6-kwidgetsaddons \
    kf6-kio rust cargo

cargo install cbindgen
```

## Step 2: Clone Repositories

```bash
# Create a workspace directory
mkdir ~/deduplikate-workspace
cd ~/deduplikate-workspace

# Clone czkawka (the backend)
git clone https://github.com/qarmin/czkawka.git

# Clone deduplikate (this repo)
git clone <your-deduplikate-repo-url> deduplikate
```

## Step 3: Build

```bash
cd deduplikate
./build.sh
```

## Step 4: Run

```bash
cd build
./deduplikate
```

## Using the Application

1. **Add Directories**: Click "Add" under "Include" to add folders to scan
2. **Choose Method**: Select "Hash (Most Accurate)" for best results
3. **Click Scan**: Start the duplicate detection
4. **Review Results**: Duplicates appear grouped in the center panel
5. **Select Files**: Check the boxes next to files you want to delete
6. **Delete**: Click "Delete Selected" (coming soon)

## Tips

- **First scan is slower**: The app builds a cache for faster subsequent scans
- **Use hash method**: Most accurate, especially Blake3
- **Filter by size**: Set minimum size to skip tiny files
- **Exclude system dirs**: Add /sys, /proc, /dev to exclude list

## Troubleshooting

### "cbindgen not found"
```bash
cargo install cbindgen
```

### "czkawka not found"
Make sure czkawka is cloned next to deduplikate:
```
parent/
├── czkawka/
└── deduplikate/
```

### "KF6 not found"
Install KDE Frameworks 6 development packages (see Step 1)

### Build fails with Qt errors
Make sure you have Qt6 (not Qt5):
```bash
# Ubuntu/Debian
sudo apt install qtbase6-dev qt6-base-dev

# Check Qt version
qmake6 --version  # or qmake -v
```

## Next Steps

- Read the full [README.md](README.md) for detailed information
- Check the [Architecture](#) section to understand how it works
- Contribute features from the [Roadmap](README.md#roadmap)

Enjoy finding those duplicate files! 🔍
