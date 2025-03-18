#!/bin/bash

# Linux App Store - https://github.com/kpk535/Linuxappstore
# License: MIT License - https://github.com/kpk535/Linuxappstore/blob/main/LICENSE


REPO_URL="https://api.github.com/repos/kpk535/Linuxappstore/releases"

# Function to display the app store header
show_header() {
    echo "-------------------------------------------"
    echo "       Welcome to the Linux App Store      "
    echo "-------------------------------------------"
    echo "1. List Available Apps"
    echo "2. Install an App"
    echo "3. Update an App"
    echo "4. Exit"
    echo "-------------------------------------------"
}

# Function to display available apps
list_apps() {
    echo "Fetching available apps..."
    curl -s $REPO_URL | jq -r '.[].assets[].browser_download_url' | grep ".deb"
}

# Function to install an app
install_app() {
    echo "Enter the name of the app to install (from the available list): "
    read app_name
    app_url=$(curl -s $REPO_URL | jq -r ".[] | select(.name | test(\"$app_name\")) | .assets[0].browser_download_url")
    
    if [[ -z "$app_url" ]]; then
        echo "App not found!"
        return
    fi

    echo "Downloading and installing $app_name..."
    wget -q "$app_url" -O /tmp/$app_name.deb
    sudo dpkg -i /tmp/$app_name.deb
    sudo apt-get install -f -y # To fix dependencies if any
    rm /tmp/$app_name.deb
    echo "$app_name installed successfully!"
}

# Function to update an app
update_app() {
    echo "Enter the name of the app to update: "
    read app_name
    app_url=$(curl -s $REPO_URL | jq -r ".[] | select(.name | test(\"$app_name\")) | .assets[0].browser_download_url")

    if [[ -z "$app_url" ]]; then
        echo "App not found!"
        return
    fi

    echo "Updating $app_name..."
    wget -q "$app_url" -O /tmp/$app_name.deb
    sudo dpkg -i /tmp/$app_name.deb
    sudo apt-get install -f -y # To fix dependencies if any
    rm /tmp/$app_name.deb
    echo "$app_name updated successfully!"
}

# Main menu loop
while true; do
    show_header
    echo -n "Please select an option: "
    read choice

    case $choice in
        1)
            list_apps
            ;;
        2)
            install_app
            ;;
        3)
            update_app
            ;;
        4)
            echo "Exiting the app store..."
            exit 0
            ;;
        *)
            echo "Invalid option, please try again."
            ;;
    esac
done
