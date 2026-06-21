# Linux GitHub App Store - WSL Installer
# Run this script in PowerShell as Administrator

param(
    [string]$WslDistro = "",
    [switch]$SkipDeps = $false
)

Write-Host "Linux GitHub App Store - WSL Setup" -ForegroundColor Cyan
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""

# Check if WSL is available
if (-not (Get-Command wsl -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: WSL is not installed or not in PATH." -ForegroundColor Red
    Write-Host "Install WSL with: wsl --install" -ForegroundColor Yellow
    exit 1
}

# List available WSL distros if none specified
if ($WslDistro -eq "") {
    Write-Host "Available WSL distributions:" -ForegroundColor Yellow
    wsl --list --verbose
    Write-Host ""
    $WslDistro = Read-Host "Enter distro name (leave blank for default)"
}

$wslArgs = if ($WslDistro -ne "") { @("-d", $WslDistro) } else { @() }

Write-Host "Using WSL distro: $(if ($WslDistro -eq '') { 'default' } else { $WslDistro })" -ForegroundColor Green
Write-Host ""

# Step 1: Install dependencies
if (-not $SkipDeps) {
    Write-Host "[1/4] Installing build dependencies..." -ForegroundColor Yellow
    $installCmd = "sudo apt-get update -qq && sudo apt-get install -y libgtk-4-dev libcurl4-openssl-dev libjson-c-dev cmake build-essential pkg-config git"
    wsl @wslArgs -- bash -c $installCmd
    if ($LASTEXITCODE -ne 0) {
        Write-Host "ERROR: Failed to install dependencies." -ForegroundColor Red
        exit 1
    }
    Write-Host "  Dependencies installed" -ForegroundColor Green
} else {
    Write-Host "[1/4] Skipping dependency installation (--SkipDeps)" -ForegroundColor Gray
}

# Step 2: Clone/update the repository
Write-Host ""
Write-Host "[2/4] Cloning repository..." -ForegroundColor Yellow

$cloneCmd = @"
if [ -d ~/linux-github-appstore ]; then
    echo 'Directory exists, pulling latest...'
    cd ~/linux-github-appstore && git pull
else
    git clone https://github.com/kpk535/linuxappstore.git ~/linux-github-appstore
fi
"@
wsl @wslArgs -- bash -c $cloneCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Failed to clone repository." -ForegroundColor Red
    exit 1
}
Write-Host "  Repository ready" -ForegroundColor Green

# Step 3: Build the application
Write-Host ""
Write-Host "[3/4] Building the application..." -ForegroundColor Yellow

$buildCmd = @"
cd ~/linux-github-appstore/LinuxGitHubAppStore
rm -rf build
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j\$(nproc)
"@
wsl @wslArgs -- bash -c $buildCmd
if ($LASTEXITCODE -ne 0) {
    Write-Host "ERROR: Build failed." -ForegroundColor Red
    exit 1
}
Write-Host "  Build successful" -ForegroundColor Green

# Step 4: Create launcher script
Write-Host ""
Write-Host "[4/4] Creating desktop launcher..." -ForegroundColor Yellow

$launcherCmd = @"
cat > ~/launch-appstore.sh << 'LAUNCHER'
#!/bin/bash
export DISPLAY=\${DISPLAY:-:0}
# WSL software rendering - suppresses libEGL/MESA driver warnings
export LIBGL_ALWAYS_SOFTWARE=1
export MESA_GL_VERSION_OVERRIDE=3.3
export GALLIUM_DRIVER=softpipe

# Check for DISPLAY
if [ -z "\$DISPLAY" ]; then
    echo "No display found. Set DISPLAY variable or install an X server."
    echo "For WSL2: export DISPLAY=\$(grep nameserver /etc/resolv.conf | awk '{print \$2}'):0"
    exit 1
fi

cd ~/linux-github-appstore/LinuxGitHubAppStore/build
./linux-github-appstore
LAUNCHER
chmod +x ~/launch-appstore.sh
echo "Launcher created at ~/launch-appstore.sh"
"@
wsl @wslArgs -- bash -c $launcherCmd
Write-Host "  Launcher created" -ForegroundColor Green

# Done
Write-Host ""
Write-Host "====================================" -ForegroundColor Cyan
Write-Host "Setup Complete!" -ForegroundColor Green
Write-Host "====================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "To run the app:" -ForegroundColor White
Write-Host ""
Write-Host "  Option 1: From WSL terminal" -ForegroundColor Yellow
Write-Host "    wsl bash ~/launch-appstore.sh" -ForegroundColor Gray
Write-Host ""
Write-Host "  Option 2: Manually" -ForegroundColor Yellow
Write-Host "    1. Open your WSL terminal" -ForegroundColor Gray
Write-Host "    2. Make sure an X server is running (VcXsrv, X410, etc.)" -ForegroundColor Gray
Write-Host "    3. Run: ~/launch-appstore.sh" -ForegroundColor Gray
Write-Host ""
Write-Host "  X Server Setup (if needed):" -ForegroundColor Yellow
Write-Host "    - Download VcXsrv: https://sourceforge.net/projects/vcxsrv/" -ForegroundColor Gray
Write-Host "    - Run XLaunch, select 'Multiple windows', disable access control" -ForegroundColor Gray
Write-Host ""

# Offer to launch now
$launch = Read-Host "Launch the app now? (requires X server running) [y/N]"
if ($launch -eq "y" -or $launch -eq "Y") {
    Write-Host "Launching Linux GitHub App Store..." -ForegroundColor Cyan
    wsl @wslArgs -- bash -c "export DISPLAY=:0; ~/launch-appstore.sh &"
}
