# Linux GitHub App Store

A GTK4-based Linux application for discovering, installing, and managing open-source applications from GitHub releases.

## Features

- 🔍 Search GitHub repositories with rich filtering
- 📂 Manage your installed applications
- ⚙️ GitHub token authentication with validation
- 👤 View your GitHub profile (when authenticated)
- 🎨 Clean, modern GTK4 interface
- 💾 Persistent settings

## Build Requirements

### Ubuntu/Debian
```bash
sudo apt-get install build-essential cmake
sudo apt-get install libgtk-4-dev
sudo apt-get install libcurl4-openssl-dev
sudo apt-get install libjson-c-dev
```

### Fedora
```bash
sudo dnf install cmake gcc
sudo dnf install gtk4-devel
sudo dnf install libcurl-devel
sudo dnf install json-c-devel
```

### Arch Linux
```bash
sudo pacman -S base-devel cmake
sudo pacman -S gtk4
sudo pacman -S curl
sudo pacman -S json-c
```

## Building

```bash
mkdir -p build && cd build
cmake ..
make
```

## Running

```bash
./linux-github-appstore
```

Or from the build directory:
```bash
cd build
./linux-github-appstore
```

## Configuration

Settings are stored in `~/.config/linux-github-appstore/settings.json`

Example:
```json
{
  "github_token": "ghp_...",
  "dark_mode": false,
  "auto_check_updates": true,
  "download_folder": "/home/user/Downloads"
}
```

## Pages

- **Home**: Search GitHub apps and repositories
- **Library**: View installed applications
- **Profile**: View GitHub account information
- **Settings**: Configure token and preferences

## Development

Built with:
- **GTK 4** - Modern Linux GUI framework
- **libcurl** - GitHub API requests
- **json-c** - JSON parsing

## License

MIT
