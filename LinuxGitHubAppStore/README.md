# Linux GitHub App Store

A modern GTK4-based desktop application for Linux that allows users to discover, search, and manage GitHub repositories.

## Features

- **Modern GTK4 Interface**: Responsive and clean UI with sidebar navigation
- **GitHub Repository Search**: Search repositories with live results
- **Token-Based Authentication**: Secure authentication using GitHub Personal Access Tokens
- **User Profile Display**: View GitHub account information
- **Settings Management**: Store and manage authentication tokens persistently
- **Application Library**: Manage installed applications

## Architecture

```
LinuxGitHubAppStore/
├── CMakeLists.txt              # Build configuration
├── README.md                   # Documentation
├── .gitignore
└── src/
    ├── main.c                  # Application entry point
    ├── models/
    │   ├── app.h/c             # Data structures
    ├── services/
    │   ├── github.h/c          # GitHub API integration
    │   └── settings.h/c        # Settings persistence
    └── ui/
        ├── window.h/c          # Main window
        ├── sidebar.h/c         # Navigation sidebar
        └── pages/
            ├── home.h/c        # Search repositories
            ├── library.h/c     # Installed apps
            ├── settings.h/c    # Token configuration
            └── profile.h/c     # GitHub profile display
```

## Build Requirements

### System Dependencies

- GTK4
- libcurl
- json-c
- CMake
- Build essentials (gcc, make)

### Installation

#### Ubuntu/Debian (WSL)
```bash
sudo apt-get update
sudo apt-get install -y \
    libgtk-4-dev \
    libcurl4-openssl-dev \
    libjson-c-dev \
    cmake \
    build-essential
```

#### Fedora/RHEL
```bash
sudo dnf install -y \
    gtk4-devel \
    libcurl-devel \
    json-c-devel \
    cmake \
    gcc make
```

#### Arch
```bash
sudo pacman -S gtk4 libcurl json-c cmake base-devel
```

## Build Instructions

### Clone and Navigate
```bash
cd LinuxGitHubAppStore
```

### Build
```bash
mkdir -p build
cd build
cmake ..
make
```

### Run
```bash
./linux-github-appstore
```

### For WSL with Display
1. Install an X-server (VcXsrv, X410, Xming)
2. Set the DISPLAY variable:
```bash
export DISPLAY=:0
# or for WSL2
export DISPLAY=$(grep -m 1 nameserver /etc/resolv.conf | awk '{print $2}'):0
```
3. Run the application:
```bash
./linux-github-appstore
```

## Configuration

Settings are stored in:
```
~/.config/linux-github-appstore/settings.json
```

Settings include:
- `github_token`: Personal Access Token for GitHub authentication
- `dark_mode`: Enable/disable dark theme
- `auto_check_updates`: Automatic update checking
- `download_folder`: Default download location

## GitHub API

The application uses the following GitHub API endpoints:

- `GET /search/repositories` - Search repositories
- `GET /user` - Get authenticated user information

### Rate Limits

- Unauthenticated: 60 requests/hour
- Authenticated: 5000 requests/hour

## Creating a Personal Access Token

1. Go to https://github.com/settings/tokens
2. Click "Generate new token"
3. Select scopes (no special scopes required for public repository access)
4. Copy the token and paste it in the Settings page

## Development

### Code Organization

- **Models** (`src/models/`): Data structures for repositories and apps
- **Services** (`src/services/`): Business logic for GitHub API and settings
- **UI** (`src/ui/`): GTK4 interface components

### Adding New Features

1. Add models in `src/models/app.h/c` if needed
2. Implement service functions in `src/services/`
3. Create UI components in `src/ui/pages/`
4. Update `src/main.c` or `src/ui/window.c` to integrate

## Testing

Compile and run:
```bash
./build/linux-github-appstore
```

Test the following:
1. Search functionality (without token)
2. Token validation in Settings
3. Profile loading (with valid token)
4. Settings persistence

## License

MIT License
