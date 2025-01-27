#include "fifo_buffer.h"
#include <cstring> // Für memcpy und memset
#include <stdexcept> // Für Ausnahmen

template<size_t MaxSize>
size_t FIFOBuffer<MaxSize>::write(const unsigned char* data, size_t len) {
    if (data == nullptr) {
        throw std::invalid_argument("data pointer is null");
    }

    size_t bytes_to_write = std::min(len, MaxSize - size);
    
    for (size_t i = 0; i < bytes_to_write; ++i) {
        buffer[write_pos] = data[i];
        write_pos = (write_pos + 1) % MaxSize;
    }
    size += bytes_to_write;
    return bytes_to_write;
}

template<size_t MaxSize>
size_t FIFOBuffer<MaxSize>::read(unsigned char* data, size_t len) {
    if (data == nullptr) {
        throw std::invalid_argument("data pointer is null");
    }

    size_t bytes_to_read = std::min(len, size);

    if (bytes_to_read > 0) {
        if (read_pos + bytes_to_read <= MaxSize) {
            memcpy(data, &buffer[read_pos], bytes_to_read);
            // Überschreibe die gelesenen Bytes mit 0
            std::memset(&buffer[read_pos], 0, bytes_to_read);
        } else {
            size_t first_part = MaxSize - read_pos;
            memcpy(data, &buffer[read_pos], first_part);
            memcpy(data + first_part, &buffer[0], bytes_to_read - first_part);
            // Überschreibe die gelesenen Bytes in zwei Teilen
            std::memset(&buffer[read_pos], 0, first_part);
            std::memset(&buffer[0], 0, bytes_to_read - first_part);
        }
        read_pos = (read_pos + bytes_to_read) % MaxSize;
        size -= bytes_to_read;
    }
    return bytes_to_read;
}

template<size_t MaxSize>
unsigned char FIFOBuffer<MaxSize>::read() {
    if (size == 0) {
        throw std::out_of_range("Buffer is empty");
    }
    
    unsigned char byte = buffer[read_pos];
    // Überschreibe das gelesene Byte mit 0
    buffer[read_pos] = 0;
    read_pos = (read_pos + 1) % MaxSize;
    size--;
    return byte;
}

template<size_t MaxSize>
bool FIFOBuffer<MaxSize>::available() const {
    return size > 0;
}

template<size_t MaxSize>
size_t FIFOBuffer<MaxSize>::getSize() const {
    return size;
}

template<size_t MaxSize>
size_t FIFOBuffer<MaxSize>::getMaxSize() const {
    return MaxSize;
}

template<size_t MaxSize>
unsigned char FIFOBuffer<MaxSize>::readAt(size_t index) const {
    if (index >= size) {
        throw std::out_of_range("Index out of range");
    }
    return buffer[(read_pos + index) % MaxSize];
}

template<size_t MaxSize>
void FIFOBuffer<MaxSize>::advanceReadPos(size_t amount) {
    if (amount > size) {
        throw std::out_of_range("Cannot advance read position beyond buffer size");
    }
    read_pos = (read_pos + amount) % MaxSize;
    size -= amount;
}

template<size_t MaxSize>
void FIFOBuffer<MaxSize>::clear() {
    // Setze alle Positionen zurück
    read_pos = 0;
    write_pos = 0;
    size = 0;
    
    // Initialisiere das gesamte Array mit 0
    std::memset(buffer.data(), 0, MaxSize);
}

// Da dies eine templatisierte Klasse ist, muss der Compiler die Definitionen der Methoden kennen. 
// In der Praxis müsste man diese Datei in einem Header oder in allen Dateien, die FIFOBuffer verwenden, einbinden.