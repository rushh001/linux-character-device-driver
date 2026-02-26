# Linux Character Device Driver Project

A comprehensive Linux kernel module demonstrating character device driver implementation with read/write/ioctl interface and mutex synchronization.

## 📋 Project Overview

This project implements:
- **Kernel Module**: Character device driver with complete file operations
- **Synchronization**: Mutex-based thread-safe operations
- **IOCTL Interface**: Custom commands for device control
- **User-Space Application**: Comprehensive test suite with interactive menu

## 🎯 Features

### Kernel Module Features
- ✅ Character device registration and management
- ✅ Read/Write operations with buffer management
- ✅ IOCTL commands for device control
- ✅ Mutex synchronization for thread safety
- ✅ Proper error handling and cleanup
- ✅ Kernel logging for debugging

### IOCTL Commands
1. **IOCTL_RESET**: Reset device buffer and flag
2. **IOCTL_GET_SIZE**: Get current buffer data size
3. **IOCTL_SET_FLAG**: Set device flag value
4. **IOCTL_GET_FLAG**: Get device flag value

### Test Application Features
- ✅ Interactive menu-driven interface
- ✅ Automated test suite mode
- ✅ Color-coded output for better readability
- ✅ Comprehensive test coverage:
  - Open/Close operations
  - Write/Read operations
  - IOCTL command testing
  - Multiple sequential operations
  - Data verification

## 📁 Project Structure

```
.
├── chardev.c          # Kernel module source code
├── Makefile           # Build system for kernel module and test app
├── test_chardev.c     # User-space test application
└── README.md          # This file
```

## 🔧 Prerequisites

### Required Packages
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install build-essential linux-headers-$(uname -r) gcc make

# Fedora/RHEL/CentOS
sudo dnf install kernel-devel kernel-headers gcc make

# Arch Linux
sudo pacman -S linux-headers base-devel
```

### System Requirements
- Linux kernel 3.10 or higher
- GCC compiler
- Root/sudo privileges for module loading
- Kernel build headers matching your running kernel

## 🚀 Building the Project

### 1. Build the Kernel Module
```bash
make
```

This will generate:
- `chardev.ko` - The kernel module
- `chardev.mod.c` - Generated module source
- `chardev.o` - Object files
- Other build artifacts

### 2. Build the Test Application
```bash
make test
```

This creates the `test_chardev` executable.

## 📦 Loading the Kernel Module

### Load the Module
```bash
sudo insmod chardev.ko
```

### Verify Module is Loaded
```bash
lsmod | grep chardev
```

### Check Device Creation
```bash
ls -l /dev/chardev
```

### Set Permissions (if needed)
```bash
sudo chmod 666 /dev/chardev
```

### Or use the Makefile shortcut:
```bash
make load
```

## 🧪 Running Tests

### Interactive Mode
```bash
./test_chardev
```

You'll see a menu:
```
=== Character Device Driver Test Menu ===
1. Test Open/Close
2. Test Write/Read
3. Test IOCTL Reset
4. Test IOCTL Get Size
5. Test IOCTL Set/Get Flag
6. Test Multiple Operations
7. Run All Tests
0. Exit
```

### Automated Test Mode
Run all tests automatically:
```bash
./test_chardev auto
```

## 📊 Monitoring Kernel Messages

### View Recent Kernel Logs
```bash
dmesg | tail -20
```

### Or use the Makefile shortcut:
```bash
make log
```

### Monitor Logs in Real-Time
```bash
sudo dmesg -w
```

## 🗑️ Unloading the Module

### Unload the Module
```bash
sudo rmmod chardev
```

### Or use the Makefile shortcut:
```bash
make unload
```

### Verify Unload
```bash
lsmod | grep chardev
ls /dev/chardev  # Should not exist
```

## 🧹 Cleaning Up

### Clean Build Artifacts
```bash
make clean
```

### Clean Everything (including test application)
```bash
make cleanall
```

## 📖 Usage Example

Complete workflow:

```bash
# 1. Build everything
make
make test

# 2. Load the module
make load

# 3. Run tests
./test_chardev auto

# 4. Check kernel logs
make log

# 5. Unload module
make unload

# 6. Clean up
make cleanall
```

## 🔍 Code Explanation

### Key Components

#### 1. Device Structure
```c
struct chardev_data {
    struct cdev cdev;           // Character device structure
    char buffer[BUFFER_SIZE];   // Data buffer
    size_t buffer_size;         // Current data size
    int flag;                   // User-controlled flag
    struct mutex lock;          // Synchronization mutex
};
```

#### 2. File Operations
- **open**: Opens device and initializes private data
- **release**: Closes device
- **read**: Reads data from device buffer to user space
- **write**: Writes data from user space to device buffer
- **ioctl**: Handles custom control commands

#### 3. Synchronization
The driver uses mutex locks to ensure thread-safe operations:
```c
mutex_lock_interruptible(&data->lock);
// Critical section
mutex_unlock(&data->lock);
```

#### 4. Module Registration
- Allocates device number dynamically
- Creates device class
- Initializes character device
- Creates device node in /dev

## 🐛 Troubleshooting

### Module won't load
- Check kernel version compatibility
- Ensure kernel headers are installed
- Check dmesg for error messages

### Permission denied on /dev/chardev
```bash
sudo chmod 666 /dev/chardev
```

### Module in use (can't unload)
- Close any open file descriptors
- Kill test applications
- Use `lsof /dev/chardev` to find processes

### Build errors
- Verify kernel headers: `ls /lib/modules/$(uname -r)/build`
- Update system: `sudo apt-get update && sudo apt-get upgrade`

## 📚 Learning Resources

### Concepts Demonstrated
1. **Character Device Drivers**: Basic kernel device driver architecture
2. **File Operations**: Implementation of standard file operations
3. **IOCTL Interface**: Custom command handling
4. **Kernel Memory**: Using kmalloc/kzalloc and copy_to_user/copy_from_user
5. **Synchronization**: Mutex locks for critical sections
6. **Module Parameters**: Dynamic device registration
7. **Error Handling**: Proper cleanup and error propagation

### Key Kernel APIs Used
- `alloc_chrdev_region()` - Dynamic device number allocation
- `class_create()` - Device class creation
- `cdev_init()`, `cdev_add()` - Character device initialization
- `device_create()` - Device node creation
- `copy_to_user()`, `copy_from_user()` - Safe kernel-user data transfer
- `mutex_init()`, `mutex_lock_interruptible()`, `mutex_unlock()` - Synchronization

## 🔒 Security Considerations

- Buffer overflow protection through size checks
- Safe user-space data transfer using copy_to/from_user
- Proper permission handling
- Mutex protection against race conditions
- Input validation in IOCTL commands

## 📝 License

This project is provided for educational purposes. Use at your own risk.
Module is licensed under GPL (as required for kernel modules).

## 👨‍💻 Development Notes

### Extending the Driver
To add new IOCTL commands:
1. Define command in both `chardev.c` and `test_chardev.c`
2. Add case in `chardev_ioctl()` function
3. Add test function in test application
4. Update menu options

### Debugging Tips
- Use `pr_info()`, `pr_err()` for kernel logging
- Monitor with `dmesg -w` while testing
- Use `strace` to debug user-space application
- Check `/sys/class/chardev_class/` for sysfs entries

## 🎓 Testing Checklist

- [x] Module loads without errors
- [x] Device file created successfully
- [x] Open/close operations work
- [x] Write operation stores data
- [x] Read operation retrieves data
- [x] Data integrity verified (write == read)
- [x] IOCTL_RESET clears buffer
- [x] IOCTL_GET_SIZE returns correct size
- [x] IOCTL_SET_FLAG sets flag value
- [x] IOCTL_GET_FLAG retrieves flag value
- [x] Multiple operations work correctly
- [x] Module unloads cleanly

## 📞 Support

For issues or questions:
- Check kernel logs: `dmesg`
- Review error messages
- Verify all prerequisites are installed
- Ensure proper permissions

---

**Note**: This is a kernel module. Improper use can cause system instability. Always test in a safe environment (VM recommended).
