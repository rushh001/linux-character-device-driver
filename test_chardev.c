/*
 * User-space test application for Character Device Driver
 * Tests read, write, and ioctl operations
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>

#define DEVICE_PATH "/dev/chardev"
#define BUFFER_SIZE 1024

/* IOCTL commands (must match kernel module) */
#define IOCTL_RESET     _IO('c', 1)
#define IOCTL_GET_SIZE  _IOR('c', 2, int)
#define IOCTL_SET_FLAG  _IOW('c', 3, int)
#define IOCTL_GET_FLAG  _IOR('c', 4, int)

/* Color codes for output */
#define COLOR_GREEN  "\033[0;32m"
#define COLOR_RED    "\033[0;31m"
#define COLOR_YELLOW "\033[0;33m"
#define COLOR_BLUE   "\033[0;34m"
#define COLOR_RESET  "\033[0m"

void print_separator(void)
{
    printf("\n%s", COLOR_BLUE);
    printf("========================================\n");
    printf("%s", COLOR_RESET);
}

void print_test_header(const char *test_name)
{
    print_separator();
    printf("%s[TEST] %s%s\n", COLOR_YELLOW, test_name, COLOR_RESET);
    print_separator();
}

void print_success(const char *message)
{
    printf("%s[✓] %s%s\n", COLOR_GREEN, message, COLOR_RESET);
}

void print_error(const char *message)
{
    printf("%s[✗] %s%s\n", COLOR_RED, message, COLOR_RESET);
}

int test_open_close(void)
{
    int fd;

    print_test_header("Test 1: Open and Close Device");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        perror("Error");
        return -1;
    }

    print_success("Device opened successfully");
    
    close(fd);
    print_success("Device closed successfully");

    return 0;
}

int test_write_read(void)
{
    int fd;
    char write_buffer[BUFFER_SIZE];
    char read_buffer[BUFFER_SIZE];
    ssize_t bytes_written, bytes_read;

    print_test_header("Test 2: Write and Read Data");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        return -1;
    }

    /* Prepare test data */
    strcpy(write_buffer, "Hello from user-space! This is a test message for the character device driver.");
    printf("Writing: \"%s\"\n", write_buffer);

    /* Write data to device */
    bytes_written = write(fd, write_buffer, strlen(write_buffer));
    if (bytes_written < 0) {
        print_error("Failed to write to device");
        perror("Error");
        close(fd);
        return -1;
    }

    printf("Bytes written: %zd\n", bytes_written);
    print_success("Write operation successful");

    /* Reset read buffer */
    memset(read_buffer, 0, BUFFER_SIZE);

    /* Seek to beginning */
    lseek(fd, 0, SEEK_SET);

    /* Read data from device */
    bytes_read = read(fd, read_buffer, BUFFER_SIZE);
    if (bytes_read < 0) {
        print_error("Failed to read from device");
        perror("Error");
        close(fd);
        return -1;
    }

    printf("Bytes read: %zd\n", bytes_read);
    printf("Read data: \"%s\"\n", read_buffer);

    /* Verify data */
    if (strcmp(write_buffer, read_buffer) == 0) {
        print_success("Data verification successful - Read matches Write");
    } else {
        print_error("Data verification failed - Read does not match Write");
    }

    close(fd);
    return 0;
}

int test_ioctl_reset(void)
{
    int fd;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;

    print_test_header("Test 3: IOCTL Reset Command");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        return -1;
    }

    /* Write some data first */
    write(fd, "Test data before reset", 22);
    print_success("Wrote test data to device");

    /* Reset the device buffer */
    if (ioctl(fd, IOCTL_RESET) < 0) {
        print_error("IOCTL_RESET failed");
        perror("Error");
        close(fd);
        return -1;
    }

    print_success("IOCTL_RESET executed successfully");

    /* Try to read - should get nothing or zeros */
    lseek(fd, 0, SEEK_SET);
    memset(buffer, 0, BUFFER_SIZE);
    bytes_read = read(fd, buffer, 10);
    
    printf("Bytes read after reset: %zd\n", bytes_read);
    if (bytes_read == 0) {
        print_success("Buffer is empty after reset");
    }

    close(fd);
    return 0;
}

int test_ioctl_get_size(void)
{
    int fd;
    int size;
    char test_data[] = "Testing buffer size calculation";

    print_test_header("Test 4: IOCTL Get Size Command");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        return -1;
    }

    /* Reset first */
    ioctl(fd, IOCTL_RESET);

    /* Write some data */
    write(fd, test_data, strlen(test_data));
    printf("Wrote %zu bytes to device\n", strlen(test_data));

    /* Get buffer size */
    if (ioctl(fd, IOCTL_GET_SIZE, &size) < 0) {
        print_error("IOCTL_GET_SIZE failed");
        perror("Error");
        close(fd);
        return -1;
    }

    printf("Buffer size returned: %d bytes\n", size);
    
    if (size == strlen(test_data)) {
        print_success("Buffer size matches written data size");
    } else {
        printf("%sBuffer size (%d) differs from written data (%zu)%s\n",
               COLOR_YELLOW, size, strlen(test_data), COLOR_RESET);
    }

    close(fd);
    return 0;
}

int test_ioctl_flag(void)
{
    int fd;
    int set_flag = 42;
    int get_flag = 0;

    print_test_header("Test 5: IOCTL Set/Get Flag Commands");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        return -1;
    }

    /* Set flag */
    printf("Setting flag to: %d\n", set_flag);
    if (ioctl(fd, IOCTL_SET_FLAG, &set_flag) < 0) {
        print_error("IOCTL_SET_FLAG failed");
        perror("Error");
        close(fd);
        return -1;
    }

    print_success("IOCTL_SET_FLAG executed successfully");

    /* Get flag */
    if (ioctl(fd, IOCTL_GET_FLAG, &get_flag) < 0) {
        print_error("IOCTL_GET_FLAG failed");
        perror("Error");
        close(fd);
        return -1;
    }

    printf("Flag value returned: %d\n", get_flag);

    if (get_flag == set_flag) {
        print_success("Flag value matches - Set/Get operation successful");
    } else {
        print_error("Flag value mismatch");
    }

    close(fd);
    return 0;
}

int test_multiple_operations(void)
{
    int fd;
    char buffer1[] = "First write operation";
    char buffer2[] = " - Second write operation";
    char read_buffer[BUFFER_SIZE];

    print_test_header("Test 6: Multiple Sequential Operations");

    fd = open(DEVICE_PATH, O_RDWR);
    if (fd < 0) {
        print_error("Failed to open device");
        return -1;
    }

    /* Reset device */
    ioctl(fd, IOCTL_RESET);
    print_success("Device reset");

    /* First write */
    write(fd, buffer1, strlen(buffer1));
    printf("First write: %zu bytes\n", strlen(buffer1));

    /* Second write (append) */
    write(fd, buffer2, strlen(buffer2));
    printf("Second write: %zu bytes\n", strlen(buffer2));

    /* Read everything */
    lseek(fd, 0, SEEK_SET);
    memset(read_buffer, 0, BUFFER_SIZE);
    ssize_t bytes_read = read(fd, read_buffer, BUFFER_SIZE);
    
    printf("Total bytes read: %zd\n", bytes_read);
    printf("Data read: \"%s\"\n", read_buffer);

    print_success("Multiple operations completed successfully");

    close(fd);
    return 0;
}

void print_menu(void)
{
    printf("\n%s=== Character Device Driver Test Menu ===%s\n", COLOR_BLUE, COLOR_RESET);
    printf("1. Test Open/Close\n");
    printf("2. Test Write/Read\n");
    printf("3. Test IOCTL Reset\n");
    printf("4. Test IOCTL Get Size\n");
    printf("5. Test IOCTL Set/Get Flag\n");
    printf("6. Test Multiple Operations\n");
    printf("7. Run All Tests\n");
    printf("0. Exit\n");
    printf("%s=========================================%s\n", COLOR_BLUE, COLOR_RESET);
    printf("Enter your choice: ");
}

void run_all_tests(void)
{
    printf("\n%s=== Running All Tests ===%s\n", COLOR_GREEN, COLOR_RESET);
    
    test_open_close();
    test_write_read();
    test_ioctl_reset();
    test_ioctl_get_size();
    test_ioctl_flag();
    test_multiple_operations();
    
    printf("\n%s=== All Tests Completed ===%s\n", COLOR_GREEN, COLOR_RESET);
}

int main(int argc, char *argv[])
{
    int choice;

    printf("\n%s", COLOR_BLUE);
    printf("╔════════════════════════════════════════╗\n");
    printf("║  Character Device Driver Test Program ║\n");
    printf("║        User-Space Test Application    ║\n");
    printf("╚════════════════════════════════════════╝\n");
    printf("%s\n", COLOR_RESET);

    /* Check if device exists */
    if (access(DEVICE_PATH, F_OK) != 0) {
        print_error("Device file does not exist!");
        printf("Make sure the kernel module is loaded:\n");
        printf("  sudo insmod chardev.ko\n");
        printf("  sudo chmod 666 /dev/chardev\n");
        return 1;
    }

    if (argc > 1) {
        /* Command line mode - run all tests */
        if (strcmp(argv[1], "auto") == 0) {
            run_all_tests();
            return 0;
        }
    }

    /* Interactive mode */
    while (1) {
        print_menu();
        
        if (scanf("%d", &choice) != 1) {
            /* Clear invalid input */
            while (getchar() != '\n');
            print_error("Invalid input! Please enter a number.");
            continue;
        }

        switch (choice) {
            case 1:
                test_open_close();
                break;
            case 2:
                test_write_read();
                break;
            case 3:
                test_ioctl_reset();
                break;
            case 4:
                test_ioctl_get_size();
                break;
            case 5:
                test_ioctl_flag();
                break;
            case 6:
                test_multiple_operations();
                break;
            case 7:
                run_all_tests();
                break;
            case 0:
                printf("\n%sExiting test program. Goodbye!%s\n\n", COLOR_GREEN, COLOR_RESET);
                return 0;
            default:
                print_error("Invalid choice! Please select 0-7.");
                break;
        }
    }

    return 0;
}
