#!/bin/bash

set -e

echo "Study Timer Pro - Build Script"
echo "=============================="

# Detect OS
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    OS="linux"
elif [[ "$OSTYPE" == "msys" || "$OSTYPE" == "cygwin" ]]; then
    OS="windows"
else
    echo "Unsupported OS: $OSTYPE"
    exit 1
fi

echo "Detected OS: $OS"

# Install dependencies
install_deps() {
    echo "Installing dependencies..."
    
    if [[ "$OS" == "linux" ]]; then
        # Update package list
        sudo apt-get update
        
        # Install basic build tools
        sudo apt-get install -y \
            build-essential \
            cmake \
            pkg-config \
            libgtk-3-dev \
            libglib2.0-dev \
            libnotify-dev
        
        # Try to install JSON-GLib (handle different package names)
        echo "Installing JSON-GLib..."
        if ! sudo apt-get install -y libjson-glib-1.0-dev; then
            echo "libjson-glib-1.0-dev not found, trying alternatives..."
            if ! sudo apt-get install -y json-glib-dev; then
                echo "Package not found, building from source..."
                build_json_glib
            fi
        fi
        
    elif [[ "$OS" == "windows" ]]; then
        # Assume MSYS2 environment
        pacman -S --noconfirm \
            mingw-w64-x86_64-toolchain \
            mingw-w64-x86_64-cmake \
            mingw-w64-x86_64-ninja \
            mingw-w64-x86_64-pkg-config \
            mingw-w64-x86_64-gtk3 \
            mingw-w64-x86_64-json-glib \
            mingw-w64-x86_64-glib2 \
            mingw-w64-x86_64-libnotify
    fi
}

# Build JSON-GLib from source if needed
build_json_glib() {
    echo "Building JSON-GLib from source..."
    
    TEMP_DIR=$(mktemp -d)
    cd "$TEMP_DIR"
    
    # Download and extract
    wget -q https://download.gnome.org/sources/json-glib/1.8/json-glib-1.8.0.tar.xz
    tar -xf json-glib-1.8.0.tar.xz
    cd json-glib-1.8.0
    
    # Build with meson (preferred) or autotools
    if command -v meson &> /dev/null; then
        meson setup build --prefix=/usr/local
        ninja -C build
        sudo ninja -C build install
    else
        # Fallback to autotools if available
        if [ -f configure ]; then
            ./configure --prefix=/usr/local
            make -j$(nproc)
            sudo make install
        else
            echo "Neither meson nor autotools available, trying cmake..."
            mkdir build && cd build
            cmake .. -DCMAKE_INSTALL_PREFIX=/usr/local
            make -j$(nproc)
            sudo make install
        fi
    fi
    
    # Update library cache
    sudo ldconfig
    
    # Clean up
    cd /
    rm -rf "$TEMP_DIR"
    echo "JSON-GLib built and installed successfully"
}

# Build the application
build_app() {
    echo "Building Study Timer Pro..."
    
    # Create build directory
    if [ -d "build" ]; then
        echo "Cleaning previous build..."
        rm -rf build
    fi
    
    mkdir build
    cd build
    
    # Configure
    echo "Configuring build..."
    if [[ "$OS" == "windows" ]]; then
        cmake .. -G Ninja -DCMAKE_BUILD_TYPE=Release
    else
        cmake .. -DCMAKE_BUILD_TYPE=Release
    fi
    
    # Build
    echo "Compiling..."
    if [[ "$OS" == "windows" ]]; then
        ninja
    else
        make -j$(nproc)
    fi
    
    echo "Build completed successfully!"
    echo "Executable location: $(pwd)/ottsr$([ "$OS" == "windows" ] && echo ".exe")"
}

# Package the application
package_app() {
    echo "Packaging application..."
    
    if [[ "$OS" == "linux" ]]; then
        # Linux packaging
        mkdir -p ottsr-linux
        cp ottsr ottsr-linux/
        
        # Create run script
        cat > ottsr-linux/run.sh << 'EOF'
#!/bin/bash
cd "$(dirname "$0")"
export LD_LIBRARY_PATH="$LD_LIBRARY_PATH:$(pwd)/lib"
./ottsr "$@"
EOF
        chmod +x ottsr-linux/run.sh
        
        # Create README
        cat > ottsr-linux/README.txt << 'EOF'
Study Timer Pro for Linux

To run the application:
1. Open terminal in this directory
2. Run: ./run.sh

Requirements:
- GTK3
- JSON-GLib
- GLib2
- libnotify

If you get dependency errors, install with:
sudo apt-get install libgtk-3-0 libglib2.0-0 libnotify4

Configuration is stored in: ~/.config/ottsr/settings.json
EOF
        
        tar czf ../ottsr-linux.tar.gz ottsr-linux/
        echo "Linux package created: ottsr-linux.tar.gz"
        
    elif [[ "$OS" == "windows" ]]; then
        # Windows packaging
        mkdir -p ottsr-windows
        cp ottsr.exe ottsr-windows/
        
        # Copy required DLLs
        echo "Copying Windows dependencies..."
        ldd ottsr.exe | grep mingw64 | awk '{print $3}' | while read dll; do
            if [ -f "$dll" ]; then
                cp "$dll" ottsr-windows/ 2>/dev/null || true
            fi
        done
        
        # Create batch file
        cat > ottsr-windows/ottsr.bat << 'EOF'
@echo off
cd /d "%~dp0"
ottsr.exe %*
pause
EOF
        
        # Create README
        cat > ottsr-windows/README.txt << 'EOF'
Study Timer Pro for Windows

To run the application:
1. Double-click ottsr.exe
   OR
2. Double-click ottsr.bat (keeps window open)

This is a portable version - no installation required.
Configuration is stored in: %APPDATA%\ottsr\settings.json

If you encounter issues, try running from Command Prompt
to see error messages.
EOF
        
        # Create ZIP archive
        if command -v zip &> /dev/null; then
            zip -r ../ottsr-windows.zip ottsr-windows/
        else
            # Fallback to 7z if available
            7z a ../ottsr-windows.zip ottsr-windows/
        fi
        
        echo "Windows package created: ottsr-windows.zip"
    fi
}

# Main execution
main() {
    case "${1:-all}" in
        deps)
            install_deps
            ;;
        build)
            build_app
            ;;
        package)
            package_app
            ;;
        all)
            install_deps
            build_app
            package_app
            ;;
        clean)
            echo "Cleaning build artifacts..."
            rm -rf build ottsr-linux ottsr-windows
            rm -f ottsr-linux.tar.gz ottsr-windows.zip
            echo "Clean completed"
            ;;
        *)
            echo "Usage: $0 [deps|build|package|all|clean]"
            echo ""
            echo "Commands:"
            echo "  deps    - Install dependencies only"
            echo "  build   - Build application only"  
            echo "  package - Package application only"
            echo "  all     - Install deps, build, and package (default)"
            echo "  clean   - Remove build artifacts"
            exit 1
            ;;
    esac
}

main "$@"
