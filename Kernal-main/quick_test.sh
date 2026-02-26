#!/bin/bash

# Quick Setup and Test Script for Character Device Driver
# This script automates the build, load, test, and cleanup process

set -e  # Exit on error

COLOR_GREEN='\033[0;32m'
COLOR_RED='\033[0;31m'
COLOR_YELLOW='\033[0;33m'
COLOR_BLUE='\033[0;34m'
COLOR_RESET='\033[0m'

print_header() {
    echo -e "${COLOR_BLUE}======================================${COLOR_RESET}"
    echo -e "${COLOR_BLUE}$1${COLOR_RESET}"
    echo -e "${COLOR_BLUE}======================================${COLOR_RESET}"
}

print_success() {
    echo -e "${COLOR_GREEN}[✓] $1${COLOR_RESET}"
}

print_error() {
    echo -e "${COLOR_RED}[✗] $1${COLOR_RESET}"
}

print_info() {
    echo -e "${COLOR_YELLOW}[i] $1${COLOR_RESET}"
}

# Check if running with sudo
check_sudo() {
    if [ "$EUID" -ne 0 ]; then
        print_error "This script requires sudo privileges"
        echo "Please run with: sudo ./quick_test.sh"
        exit 1
    fi
}

# Build kernel module and test application
build_all() {
    print_header "Building Kernel Module and Test Application"
    
    print_info "Building kernel module..."
    make clean > /dev/null 2>&1 || true
    make
    print_success "Kernel module built successfully"
    
    print_info "Building test application..."
    make test
    print_success "Test application built successfully"
    
    echo ""
}

# Load the kernel module
load_module() {
    print_header "Loading Kernel Module"
    
    # Unload if already loaded
    if lsmod | grep -q chardev; then
        print_info "Module already loaded, unloading first..."
        rmmod chardev
    fi
    
    print_info "Loading chardev module..."
    insmod chardev.ko
    print_success "Module loaded successfully"
    
    # Set permissions
    if [ -e /dev/chardev ]; then
        chmod 666 /dev/chardev
        print_success "Device permissions set"
        ls -l /dev/chardev
    else
        print_error "Device file not created"
        exit 1
    fi
    
    echo ""
}

# Show module information
show_module_info() {
    print_header "Module Information"
    
    echo "Loaded module:"
    lsmod | grep chardev
    
    echo ""
    echo "Device file:"
    ls -l /dev/chardev
    
    echo ""
    print_info "Recent kernel messages:"
    dmesg | tail -10
    
    echo ""
}

# Run tests
run_tests() {
    print_header "Running Tests"
    
    # Make test executable
    chmod +x test_chardev
    
    print_info "Running automated test suite..."
    ./test_chardev auto
    
    echo ""
}

# Show kernel logs
show_logs() {
    print_header "Kernel Messages"
    dmesg | tail -30
    echo ""
}

# Unload module
unload_module() {
    print_header "Unloading Module"
    
    if lsmod | grep -q chardev; then
        print_info "Unloading chardev module..."
        rmmod chardev
        print_success "Module unloaded successfully"
    else
        print_info "Module not loaded"
    fi
    
    echo ""
}

# Cleanup
cleanup() {
    print_header "Cleaning Up"
    
    print_info "Removing build artifacts..."
    make cleanall > /dev/null 2>&1 || true
    print_success "Cleanup complete"
    
    echo ""
}

# Main menu
show_menu() {
    echo ""
    echo -e "${COLOR_BLUE}=== Character Device Driver Quick Test ===${COLOR_RESET}"
    echo "1. Build All"
    echo "2. Load Module"
    echo "3. Show Module Info"
    echo "4. Run Tests"
    echo "5. Show Kernel Logs"
    echo "6. Unload Module"
    echo "7. Cleanup"
    echo "8. Full Test (Build -> Load -> Test -> Unload)"
    echo "0. Exit"
    echo -e "${COLOR_BLUE}===========================================${COLOR_RESET}"
    echo -n "Enter choice: "
}

# Full automated test
full_test() {
    print_header "Full Automated Test"
    
    build_all
    load_module
    show_module_info
    run_tests
    show_logs
    unload_module
    
    print_success "Full test completed successfully!"
}

# Main script
main() {
    check_sudo
    
    echo -e "${COLOR_GREEN}"
    echo "╔════════════════════════════════════════╗"
    echo "║    Character Device Driver Testing    ║"
    echo "║         Quick Setup Script            ║"
    echo "╚════════════════════════════════════════╝"
    echo -e "${COLOR_RESET}"
    
    if [ $# -eq 1 ] && [ "$1" == "auto" ]; then
        # Automated mode
        full_test
        exit 0
    fi
    
    # Interactive mode
    while true; do
        show_menu
        read choice
        
        case $choice in
            1) build_all ;;
            2) load_module ;;
            3) show_module_info ;;
            4) run_tests ;;
            5) show_logs ;;
            6) unload_module ;;
            7) cleanup ;;
            8) full_test ;;
            0)
                echo -e "${COLOR_GREEN}Exiting. Goodbye!${COLOR_RESET}"
                exit 0
                ;;
            *)
                print_error "Invalid choice"
                ;;
        esac
        
        echo ""
        echo -e "${COLOR_YELLOW}Press Enter to continue...${COLOR_RESET}"
        read
    done
}

main "$@"
