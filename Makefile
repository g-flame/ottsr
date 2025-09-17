# Project configuration
PROJECT_NAME = ottsr
VERSION = 1.0.0
SRC_DIR = src
BUILD_DIR = build
DIST_DIR = dist

# Compiler settings
CC = cl.exe
CFLAGS = /std:c11 /W4 /nologo
RELEASE_FLAGS = /O2 /MD /DNDEBUG
DEBUG_FLAGS = /Zi /Od /MDd /D_DEBUG
LIBS = user32.lib gdi32.lib kernel32.lib comctl32.lib winmm.lib

# Source files
SOURCES = $(SRC_DIR)/ottsr.c
HEADERS = $(SRC_DIR)/ottsr.h

# Output files
RELEASE_EXE = $(BUILD_DIR)/$(PROJECT_NAME).exe
DEBUG_EXE = $(BUILD_DIR)/$(PROJECT_NAME)_debug.exe

# Default target
all: release

# Create directories
$(BUILD_DIR):
	@if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"

$(DIST_DIR):
	@if not exist "$(DIST_DIR)" mkdir "$(DIST_DIR)"

# Release build
release: $(BUILD_DIR) $(RELEASE_EXE)

$(RELEASE_EXE): $(SOURCES) $(HEADERS)
	@echo Building release version...
	$(CC) $(CFLAGS) $(RELEASE_FLAGS) /Fe:$(RELEASE_EXE) $(SOURCES) /link $(LIBS)
	@echo Release build completed: $(RELEASE_EXE)

# Debug build
debug: $(BUILD_DIR) $(DEBUG_EXE)

$(DEBUG_EXE): $(SOURCES) $(HEADERS)
	@echo Building debug version...
	$(CC) $(CFLAGS) $(DEBUG_FLAGS) /Fe:$(DEBUG_EXE) $(SOURCES) /link $(LIBS)
	@echo Debug build completed: $(DEBUG_EXE)

# Both builds
both: release debug

# Create distribution package
dist: release $(DIST_DIR)
	@echo Creating distribution package...
	@copy "$(RELEASE_EXE)" "$(DIST_DIR)\"
	@copy "README.md" "$(DIST_DIR)\" 2>nul || echo README.md not found
	@copy "LICENSE" "$(DIST_DIR)\" 2>nul || echo LICENSE not found
	@copy "example-ottsr.conf" "$(DIST_DIR)\ottsr.conf" 2>nul || echo example config not found
	@echo Distribution package created in $(DIST_DIR)

# Clean build artifacts
clean:
	@if exist "$(BUILD_DIR)" rmdir /s /q "$(BUILD_DIR)"
	@if exist "$(DIST_DIR)" rmdir /s /q "$(DIST_DIR)"
	@if exist "*.obj" del "*.obj"
	@if exist "*.pdb" del "*.pdb"
	@if exist "*.ilk" del "*.ilk"
	@echo Cleaned build artifacts

# Install (copy to system directory - requires admin)
install: release
	@echo Installing $(PROJECT_NAME)...
	@copy "$(RELEASE_EXE)" "C:\Program Files\$(PROJECT_NAME)\" 2>nul || (mkdir "C:\Program Files\$(PROJECT_NAME)" && copy "$(RELEASE_EXE)" "C:\Program Files\$(PROJECT_NAME)\")
	@echo Installation completed

# Uninstall
uninstall:
	@echo Uninstalling $(PROJECT_NAME)...
	@if exist "C:\Program Files\$(PROJECT_NAME)" rmdir /s /q "C:\Program Files\$(PROJECT_NAME)"
	@echo Uninstallation completed

# Run release version
run: release
	@$(RELEASE_EXE)

# Run debug version
run-debug: debug
	@$(DEBUG_EXE)

# Help
help:
	@echo Over The Top Study Reminder Build System
	@echo.
	@echo Available targets:
	@echo   all        - Build release version (default)
	@echo   release    - Build optimized release version
	@echo   debug      - Build debug version with symbols
	@echo   both       - Build both release and debug versions
	@echo   dist       - Create distribution package
	@echo   clean      - Remove all build artifacts
	@echo   install    - Install to system directory
	@echo   uninstall  - Remove from system directory
	@echo   run        - Build and run release version
	@echo   run-debug  - Build and run debug version
	@echo   help       - Show this help message
	@echo.
	@echo Example usage:
	@echo   nmake release
	@echo   nmake debug
	@echo   nmake dist

.PHONY: all release debug both dist clean install uninstall run run-debug help
