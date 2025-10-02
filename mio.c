/*
MIO STANDARD I/O LIBRARY IMPLEMENTATION
File: mio.c
Default buffer size: 10 bytes (MBSIZE)
Description: Custom standard I/O library implementation using low-level POSIX I/O functions
Features: Buffered I/O, multiple file modes (read, write/append, write/truncate), 
          string and character I/O operations, dynamic buffer management
Author: Subhajit Halder
*/

#include "mio.h"
#include "dprint.h"
#include <errno.h>
#include <string.h>

// Open file with specified mode
MIO *myopen(const char *name, const int mode) {
    // Allocate memory for MIO structure
    MIO *mio = malloc(sizeof(MIO));
    if (!mio) {
        DPRINT("Failed to allocate MIO structure\n");
        return NULL;
    }
    
    // Initialize MIO structure to zero
    memset(mio, 0, sizeof(MIO));
    mio->fd = -1;  // Mark as invalid initially
    
    int flags = 0;
    int create_mode = 0644;  // Default file permissions
    
    // Set flags based on requested mode
    switch (mode) {
        case MODE_R:
            flags = O_RDONLY;
            break;
        case MODE_WA:
            flags = O_WRONLY | O_CREAT | O_APPEND;
            break;
        case MODE_WT:
            flags = O_WRONLY | O_CREAT | O_TRUNC;
            break;
        default:
            DPRINT("Invalid mode specified: %d\n", mode);
            free(mio);
            return NULL;
    }
    
    // Open the file using low-level system call
    mio->fd = open(name, flags, create_mode);
    if (mio->fd < 0) {
        DPRINT("Failed to open file '%s': %s\n", name, strerror(errno));
        free(mio);
        return NULL;
    }
    
    // Allocate read and write buffers
    mio->rb = malloc(MBSIZE);
    mio->wb = malloc(MBSIZE);
    if (!mio->rb || !mio->wb) {
        DPRINT("Failed to allocate buffers\n");
        if (mio->rb) free(mio->rb);
        if (mio->wb) free(mio->wb);
        close(mio->fd);
        free(mio);
        return NULL;
    }
    
    // Initialize MIO structure fields
    mio->rw = mode;
    mio->rsize = MBSIZE;
    mio->wsize = MBSIZE;
    mio->rs = 0;  // Read buffer start position
    mio->re = 0;  // Read buffer end position (amount of valid data)
    mio->ws = 0;  // Write buffer current position
    mio->we = 0;  // Write buffer end position
    
    DPRINT("Successfully opened file '%s' in mode %d\n", name, mode);
    return mio;
}

// Close file and free resources
int myclose(MIO *m) {
    if (!m) {
        DPRINT("Attempt to close NULL MIO pointer\n");
        return -1;
    }
    
    int result = 0;
    
    // If file was opened for writing, flush any remaining data
    if (M_ISMW(m->rw) && m->ws > 0) {
        DPRINT("Flushing write buffer before close\n");
        if (myflush(m) < 0) {
            DPRINT("Failed to flush buffer during close\n");
            result = -1;
        }
    }
    
    // Close the file descriptor
    if (close(m->fd) < 0) {
        DPRINT("Failed to close file descriptor: %s\n", strerror(errno));
        result = -1;
    }
    
    // Free buffers and MIO structure
    if (m->rb) free(m->rb);
    if (m->wb) free(m->wb);
    free(m);
    
    DPRINT("File closed successfully\n");
    return result;
}

// Read data from file into buffer
int myread(MIO *m, char *b, const int size) {
    if (!m || !b || size < 0) {
        DPRINT("Invalid parameters to myread\n");
        return -1;
    }
    
    if (m->rw != MODE_R) {
        DPRINT("File not opened for reading\n");
        return -1;
    }
    
    int total_read = 0;
    
    while (total_read < size) {
        // If read buffer is empty, refill it from file
        if (m->rs >= m->re) {
            m->re = read(m->fd, m->rb, m->rsize);
            if (m->re < 0) {
                DPRINT("Read error: %s\n", strerror(errno));
                return -1;
            }
            if (m->re == 0) {
                // End of file reached
                DPRINT("EOF reached, read %d bytes\n", total_read);
                return total_read > 0 ? total_read : -1;
            }
            m->rs = 0;
        }
        
        // Calculate how many bytes we can copy from buffer
        int available = m->re - m->rs;
        int needed = size - total_read;
        int to_copy = (available < needed) ? available : needed;
        
        // Copy data from read buffer to user buffer
        memcpy(b + total_read, m->rb + m->rs, to_copy);
        m->rs += to_copy;
        total_read += to_copy;
    }
    
    DPRINT("Read %d bytes from file\n", total_read);
    return total_read;
}

// Read single character from file
int mygetc(MIO *m, char *c) {
    if (!m || !c) {
        DPRINT("Invalid parameters to mygetc\n");
        return -1;
    }
    
    // Use myread to get one character
    int result = myread(m, c, 1);
    if (result == 1) {
        DPRINT("Read character: %c\n", *c);
    } else {
        DPRINT("Failed to read character\n");
    }
    return result;
}

// Read string until whitespace
char *mygets(MIO *m, int *len) {
    if (!m) {
        DPRINT("Invalid MIO pointer to mygets\n");
        return NULL;
    }
    
    // Skip leading whitespace
    char ch;
    int result;
    do {
        result = mygetc(m, &ch);
        if (result == -1) {
            DPRINT("EOF reached while skipping whitespace\n");
            return NULL;
        }
    } while (M_ISWS(ch));
    
    // Allocate buffer for the string
    char *buffer = malloc(MBSIZE);
    if (!buffer) {
        DPRINT("Failed to allocate string buffer\n");
        return NULL;
    }
    
    int pos = 0;
    buffer[pos++] = ch;  // Add the first non-whitespace character
    
    // Read until next whitespace or buffer full
    while (pos < MBSIZE - 1) {
        result = mygetc(m, &ch);
        if (result != 1) {
            break;  // EOF or error
        }
        
        // Stop at whitespace
        if (M_ISWS(ch)) {
            break;
        }
        
        buffer[pos++] = ch;
    }
    
    buffer[pos] = '\0';  // Null terminate the string
    
    if (len) {
        *len = pos;
    }
    
    DPRINT("Read string: '%s' (length: %d)\n", buffer, pos);
    return buffer;
}

// Write data to file
int mywrite(MIO *m, const char *b, const int size) {
    if (!m || !b || size < 0) {
        DPRINT("Invalid parameters to mywrite\n");
        return -1;
    }
    
    if (!M_ISMW(m->rw)) {
        DPRINT("File not opened for writing\n");
        return -1;
    }
    
    int total_written = 0;
    
    while (total_written < size) {
        int available = m->wsize - m->ws;
        int remaining = size - total_written;
        int to_copy = (available < remaining) ? available : remaining;
        
        // Copy data to write buffer
        memcpy(m->wb + m->ws, b + total_written, to_copy);
        m->ws += to_copy;
        total_written += to_copy;
        
        // If buffer is full, flush it
        if (m->ws >= m->wsize) {
            if (myflush(m) < 0) {
                DPRINT("Failed to flush buffer during write\n");
                return -1;
            }
        }
    }
    
    DPRINT("Buffered %d bytes for writing\n", total_written);
    return total_written;
}

// Flush write buffer to file
int myflush(MIO *m) {
    if (!m) {
        DPRINT("Invalid MIO pointer to myflush\n");
        return -1;
    }
    
    if (!M_ISMW(m->rw)) {
        DPRINT("File opened for reading, nothing to flush\n");
        return 0;
    }
    
    if (m->ws == 0) {
        DPRINT("Write buffer is empty, nothing to flush\n");
        return 0;
    }
    
    // Write the entire buffer to file
    int written = write(m->fd, m->wb, m->ws);
    if (written < 0) {
        DPRINT("Write error during flush: %s\n", strerror(errno));
        return -1;
    }
    
    if (written != m->ws) {
        DPRINT("Partial write during flush: %d of %d bytes\n", written, m->ws);
        // Handle partial write by moving remaining data to front
        if (written > 0) {
            memmove(m->wb, m->wb + written, m->ws - written);
            m->ws -= written;
        }
        return written;
    }
    
    // Reset write position after successful flush
    m->ws = 0;
    DPRINT("Successfully flushed %d bytes to file\n", written);
    return written;
}

// Write single character to file
int myputc(MIO *m, const char c) {
    if (!m) {
        DPRINT("Invalid MIO pointer to myputc\n");
        return -1;
    }
    
    // Use mywrite to write one character
    int result = mywrite(m, &c, 1);
    if (result == 1) {
        DPRINT("Wrote character: %c\n", c);
    } else {
        DPRINT("Failed to write character\n");
    }
    return result;
}

// Write string to file
int myputs(MIO *m, const char *str, const int len) {
    if (!m || !str || len < 0) {
        DPRINT("Invalid parameters to myputs\n");
        return -1;
    }
    
    // Use mywrite to write the entire string
    int result = mywrite(m, str, len);
    if (result == len) {
        DPRINT("Successfully wrote string of length %d\n", len);
    } else {
        DPRINT("Failed to write complete string: %d of %d bytes\n", result, len);
    }
    return result;
}
