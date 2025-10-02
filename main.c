/*
MIO Library Test Suite
File: main.c
Description: Test for all MIO library functions
Author: Subhajit Halder
*/

#include "mio.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>

// Test utility functions
void print_test_result(const char *test_name, int result) {
    if (result == 0) {
        printf("%s: PASSED\n", test_name);
    } else {
        printf("%s: FAILED (error code: %d)\n", test_name, result);
    }
}

void create_test_file(const char *filename, const char *content) {
    int fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) {
        write(fd, content, strlen(content));
        close(fd);
    }
}

// Test functions
int test_file_open_close() {
    printf("\nTesting File Open/Close\n");
    
    int result = 0;
    
    // Create test file first
    create_test_file("test_read.txt", "Test content for reading");
    
    //Open file for reading
    MIO *file = myopen("test_read.txt", MODE_R);
    if (!file) {
        printf("Failed to open existing file for reading\n");
        result = -1;
    } else {
        myclose(file);
    }
    
    //Open file for write/truncate
    file = myopen("test_write.txt", MODE_WT);
    if (!file) {
        printf("Failed to open file for write/truncate\n");
        result = -1;
    } else {
        myclose(file);
    }
    
    //Open file for write/append
    file = myopen("test_append.txt", MODE_WA);
    if (!file) {
        printf("Failed to open file for write/append\n");
        result = -1;
    } else {
        myclose(file);
    }
    
    //Open non-existent file for reading (should fail)
    file = myopen("nonexistent.txt", MODE_R);
    if (file) {
        printf("Failed to open non-existent file\n");
        myclose(file);
        result = -1;
    }
    
    print_test_result("File Open/Close", result);
    return result;
}

int test_myread_mygetc() {
    printf("\nTesting myread and mygetc\n");
    
    // Create test file
    create_test_file("test_read.txt", "Hello World! This is a test file.\nLine 2\nLine 3");
    
    MIO *file = myopen("test_read.txt", MODE_R);
    if (!file) {
        printf("Failed to open test file for reading\n");
        return -1;
    }
    
    //Read multiple bytes
    char buffer[20];
    int bytes_read = myread(file, buffer, 12);
    buffer[12] = '\0';
    printf("Read %d bytes: '%s'\n", bytes_read, buffer);
    
    //Read single characters
    char ch;
    printf("Next 5 characters: ");
    for (int i = 0; i < 5; i++) {
        if (mygetc(file, &ch) == 1) {
            printf("%c", ch);
        }
    }
    printf("\n");
    
    //Read until EOF
    int total_bytes = bytes_read + 5;
    while (mygetc(file, &ch) == 1) {
        total_bytes++;
    }
    printf("Total bytes read from file: %d\n", total_bytes);
    
    myclose(file);
    print_test_result("myread and mygetc", 0);
    return 0;
}

int test_mygets() {
    printf("\nTesting mygets\n");
    
    // Create test file with whitespace
    create_test_file("test_strings.txt", "   First  Second\nThird\tFourth  Fifth");
    
    MIO *file = myopen("test_strings.txt", MODE_R);
    if (!file) {
        printf("Failed to open test file for reading\n");
        return -1;
    }
    
    // Read strings one by one
    int len;
    char *str;
    int string_count = 0;
    
    while ((str = mygets(file, &len)) != NULL) {
        printf("String %d (length %d): '%s'\n", ++string_count, len, str);
        free(str);
    }
    
    printf("Total strings read: %d\n", string_count);
    
    myclose(file);
    print_test_result("mygets", 0);
    return 0;
}

int test_mywrite_myputc() {
    printf("\nTesting mywrite and myputc\n");
    
    MIO *file = myopen("test_output.txt", MODE_WT);
    if (!file) {
        printf("Failed to open test file for writing\n");
        return -1;
    }
    
    //Write multiple bytes
    const char *text1 = "Hello, World!\n";
    int written = mywrite(file, text1, strlen(text1));
    printf("Written %d bytes: '%s'\n", written, text1);
    
    //Write single characters
    const char *chars = "ABC";
    for (int i = 0; i < 3; i++) {
        if (myputc(file, chars[i]) != 1) {
            printf("Failed to write character: %c\n", chars[i]);
        }
    }
    myputc(file, '\n');
    
    //Write data that exceeds buffer size (10 bytes)
    const char *long_text = "This is a longer text that should trigger buffer flushing multiple times.";
    written = mywrite(file, long_text, strlen(long_text));
    printf("Written %d bytes of long text\n", written);
    
    myclose(file);
    print_test_result("mywrite and myputc", 0);
    return 0;
}

int test_myputs() {
    printf("\nTesting myputs\n");
    
    MIO *file = myopen("test_strings_out.txt", MODE_WT);
    if (!file) {
        printf("Failed to open test file for writing\n");
        return -1;
    }
    
    // Write multiple strings
    const char *strings[] = {
        "First string",
        "Second string",
        "Third string",
        "Fourth string"
    };
    
    for (int i = 0; i < 4; i++) {
        int len = strlen(strings[i]);
        int written = myputs(file, strings[i], len);
        myputc(file, '\n');
        printf("Written string %d: '%s' (%d bytes)\n", i+1, strings[i], written);
    }
    
    myclose(file);
    print_test_result("myputs", 0);
    return 0;
}

int test_append_mode() {
    printf("\nTesting Append Mode\n");
    
    // First create a file with some content
    create_test_file("test_append.txt", "Initial content\n");
    
    // Open in append mode and add more content
    MIO *file = myopen("test_append.txt", MODE_WA);
    if (!file) {
        printf("Failed to open test file for append\n");
        return -1;
    }
    
    const char *append_text = "Appended content\n";
    mywrite(file, append_text, strlen(append_text));
    myclose(file);
    
    // Read back to verify
    file = myopen("test_append.txt", MODE_R);
    if (!file) {
        printf("Failed to open test file for reading\n");
        return -1;
    }
    
    char buffer[100];
    int total_read = 0;
    int bytes;
    while ((bytes = myread(file, buffer + total_read, 50)) > 0) {
        total_read += bytes;
    }
    buffer[total_read] = '\0';
    printf("Final file content:\n%s", buffer);
    
    myclose(file);
    print_test_result("Append Mode", 0);
    return 0;
}

int test_error_conditions() {
    printf("\nTesting Error Conditions\n");
    
    // Create a test file first
    create_test_file("test_errors.txt", "Test content");
    
    int result = 0;
    
    //Read from write-only file
    MIO *file = myopen("test_errors.txt", MODE_WT);
    if (!file) {
        printf("Failed to open test file for writing\n");
        return -1;
    }
    
    char buffer[10];
    int read_result = myread(file, buffer, 5);
    printf("Read from write-only file: %d (should be -1)\n", read_result);
    if (read_result != -1) result = -1;
    myclose(file);
    
    //Write to read-only file
    file = myopen("test_errors.txt", MODE_R);
    if (!file) {
        printf("Failed to open test file for reading\n");
        return -1;
    }
    
    int write_result = mywrite(file, "test", 4);
    printf("Write to read-only file: %d (should be -1)\n", write_result);
    if (write_result != -1) result = -1;
    myclose(file);
    
    //Close NULL pointer
    int close_result = myclose(NULL);
    printf("Close NULL pointer: %d (should be -1)\n", close_result);
    if (close_result != -1) result = -1;
    
    print_test_result("Error Conditions", result);
    return result;
}

int main() {
    printf("MIO Library Comprehensive Test Suite\n");    
    int all_passed = 0;    
    // Run all tests
    all_passed |= test_file_open_close();
    all_passed |= test_myread_mygetc();
    all_passed |= test_mygets();
    all_passed |= test_mywrite_myputc();
    all_passed |= test_myputs();
    all_passed |= test_append_mode();
    all_passed |= test_error_conditions();
    if (all_passed == 0) {
        printf("All tests PASSED!\n");
    } else {
        printf("Some tests FAILED!\n");
    }    
    // Cleanup test files
    unlink("test_read.txt");
    unlink("test_write.txt");
    unlink("test_append.txt");
    unlink("test_strings.txt");
    unlink("test_output.txt");
    unlink("test_strings_out.txt");
    unlink("test_errors.txt");
    
    return all_passed;
}
