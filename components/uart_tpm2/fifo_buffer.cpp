#include "fifo_buffer.h"

FIFOBuffer::FIFOBuffer(size_t max_size) : buffer(max_size) {}

size_t FIFOBuffer::write(const unsigned char* data, size_t len) {
    size_t bytes_to_write = std::min(len, buffer.size() - size);
    
    for (size_t i = 0; i < bytes_to_write; ++i) {
        buffer[write_pos] = data[i];
        write_pos = (write_pos + 1) % buffer.size();
    }
    size += bytes_to_write;
    return bytes_to_write;
}

unsigned char FIFOBuffer::read() {
    if (size == 0) return 0; // oder throw exception
    
    unsigned char byte = buffer[read_pos];
    read_pos = (read_pos + 1) % buffer.size();
    size--;
    return byte;
}

bool FIFOBuffer::available() const {
    return size > 0;
}

size_t FIFOBuffer::getSize() const {
    return size;
}

size_t FIFOBuffer::getMaxSize() const {
    return buffer.size();
}