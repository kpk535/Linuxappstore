#!/bin/bash

# Linux App Store - https://github.com/kpk535/Linuxappstore
# License: MIT License - https://github.com/kpk535/Linuxappstore/blob/main/LICENSE

# Set the GitHub repository URL for updates
GITHUB_REPO="https://api.github.com/repos/kpk535/Linuxappstore/releases/latest"

# Function to display the app store header
show_header() {
    dialog --title "Linux App Store" --msgbox "Welcome to the Linux App Store!" 10 30
}

# Function to check for app store updates
check_for_updates() {
    dialog --title "App Store Update" --msgbox "Checking for app store updates..." 10 30

    # Fetch the latest release from the GitHub repository
    latest_version=$(curl -s $GITHUB_REPO | jq -r '.tag_name')
    current_version=$(cat /tmp/app_store_version 2>/dev/null || echo "none")

    if [ "$latest_version" == "$current_version" ]; then
        dialog --title "App Store Update" --msgbox "You are already using the latest version of the app store!" 10 30
    else
        dialog --title "App Store Update" --msgbox "A new version is available: $latest_version. Updating now..." 10 30
        download_and_update_store "$latest_version"
    fi
}

# Function to download and update the app store
download_and_update_store() {
    latest_version=$1
    dialog --title "App Store Update" --msgbox "Downloading app store update script for version $latest_version..." 10 30

    # Fetch the assets (assuming the update is a Bash script for the app store itself)
    update_url=$(curl -s $GITHUB_REPO | jq -r ".assets[] | select(.name | test(\"$latest_version.*.sh\")) | .browser_download_url")

    if [[ -z "$update_url" ]]; then
        dialog --title "App Store Update" --msgbox "No update script found for version $latest_version!" 10 30
        return
    fi

    dialog --title "App Store Update" --msgbox "Downloading update script from $update_url..." 10 30
    wget -q "$update_url" -O /tmp/linux_app_store_update.sh

    dialog --title "App Store Update" --msgbox "Executing update script..." 10 30
    bash /tmp/linux_app_store_update.sh

    # Clean up
    rm /tmp/linux_app_store_update.sh

    # Update the version file
    echo "$latest_version" > /tmp/app_store_version
    dialog --title "App Store Update" --msgbox "App store updated successfully to version $latest_version!" 10 30
}

# Function to install a Snap app
install_snap_app() {
    app_name=$(dialog --title "Install Snap App" --inputbox "Enter the name of the Snap app you want to install:" 10 50 3>&1 1>&2 2>&3)
    exit_status=$?

    if [ $exit_status -eq 0 ]; then
        dialog --title "Install Snap App" --msgbox "Searching for Snap app '$app_name'..." 10 30

        # Check if the Snap package exists using 'snap info'
        snap_info=$(snap info "$app_name" 2>&1)

        # If the app is not found or there's an error
        if [[ "$snap_info" == *"error:"* ]]; then
            dialog --title "Install Snap App" --msgbox "App '$app_name' not found in the Snap store!" 10 30
            return
        fi

        # Extract information about the Snap app (e.g., publisher)
        publisher=$(echo "$snap_info" | grep -i "publisher" | cut -d ':' -f2 | xargs)

        dialog --title "Install Snap App" --msgbox "App found in Snap Store. Publisher: $publisher. Do you want to continue installation?" 10 30
        
        # Ask user for confirmation before installing
        user_choice=$(dialog --title "Install Snap App" --menu "Do you want to install $app_name from Snap Store?" 15 50 2 \
            1 "Yes" \
            2 "No" \
            3>&1 1>&2 2>&3)

        if [ "$user_choice" -eq 1 ]; then
            dialog --title "Install Snap App" --msgbox "Installing Snap app '$app_name'..." 10 30
            
            # Prompt for password using sudo and install Snap app
            sudo -v  # Ensures sudo session is alive and prompts for password if necessary
            sudo snap install "$app_name"  # Prompt for password if needed

            if [ $? -eq 0 ]; then
                dialog --title "Install Snap App" --msgbox "Snap app '$app_name' installed successfully!" 10 30
            else
                dialog --title "Install Snap App" --msgbox "Failed to install Snap app '$app_name'. Please check your permissions." 10 30
            fi
        else
            dialog --title "Install Snap App" --msgbox "Installation of '$app_name' was cancelled." 10 30
        fi
    fi
}

# Main menu loop
while true; do
    choice=$(dialog --title "Linux App Store" --menu "Please select an option" 15 50 3 \
    1 "Check for App Store Updates" \
    2 "Install a Snap App" \
    3 "Exit" \
    3>&1 1>&2 2>&3)

    exit_status=$?

    if [ $exit_status -eq 1 ]; then
        break
    fi

    case $choice in
        1)
            check_for_updates
            ;;
        2)
            install_snap_app
            ;;
        3)
            dialog --title "Exit" --msgbox "Exiting the app store..." 10 30
            exit 0
            ;;
        *)
            dialog --title "Error" --msgbox "Invalid option, please try again." 10 30
            ;;
    esac
done

