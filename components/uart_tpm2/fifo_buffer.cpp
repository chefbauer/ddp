// fifo_buffer.cpp

#include "fifo_buffer.h"
#include <cstring> // Für memcpy

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

// Neue Methode, die Daten direkt in ein Array kopiert
size_t FIFOBuffer::read(unsigned char* data, size_t len) {
    size_t bytes_to_read = std::min(len, size);

    if (bytes_to_read > 0) {
        // Wenn der Lesepuffer am Anfang des Arrays ist, können wir direkt kopieren
        if (read_pos + bytes_to_read <= buffer.size()) {
            memcpy(data, &buffer[read_pos], bytes_to_read);
        } else {
            // Leseposition ist am Ende, wir müssen in zwei Schritten kopieren
            size_t first_part = buffer.size() - read_pos;
            memcpy(data, &buffer[read_pos], first_part);
            memcpy(data + first_part, &buffer[0], bytes_to_read - first_part);
        }
        read_pos = (read_pos + bytes_to_read) % buffer.size();
        size -= bytes_to_read;
    }
    return bytes_to_read;
}

unsigned char FIFOBuffer::read() {
    if (size == 0) return 0;
    
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

// Direkter Zugriff auf ein Byte
unsigned char FIFOBuffer::readAt(size_t index) const {
    if (index >= size) return 0; // oder andere Fehlerbehandlung
    return buffer[(read_pos + index) % buffer.size()];
}

// Erhöht die Leseposition
void FIFOBuffer::advanceReadPos(size_t amount) {
    if (amount > size) amount = size;
    read_pos = (read_pos + amount) % buffer.size();
    size -= amount;
}