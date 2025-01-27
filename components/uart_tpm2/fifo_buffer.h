#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <array>
#include <cstddef>
#include <cstring> // Für memcpy und memset
#include "esp_log.h" // Für ESP_LOGE

template<size_t MaxSize>
class FIFOBuffer {
private:
    std::array<unsigned char, MaxSize> buffer;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t size = 0;

public:
    FIFOBuffer() : buffer{} {} // Alle Elemente initialisieren
    size_t write(const unsigned char* data, size_t len);
    size_t read(unsigned char* data, size_t len);
    unsigned char read();
    bool available() const;
    size_t getSize() const;
    size_t getMaxSize() const;
    unsigned char readAt(size_t index) const;
    void advanceReadPos(size_t amount);
    void clear();
};

template<size_t MaxSize>
size_t FIFOBuffer<MaxSize>::write(const unsigned char* data, size_t len) {
    if (data == nullptr) {
        ESP_LOGE("FIFOBuffer", "Data pointer is null");
        return 0; // Fehler, Rückgabe von 0
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
        ESP_LOGE("FIFOBuffer", "Data pointer is null");
        return 0; // Fehler, Rückgabe von 0
    }

    size_t bytes_to_read = std::min(len, size);

    if (bytes_to_read > 0) {
        if (read_pos + bytes_to_read <= MaxSize) {
            std::memcpy(data, &buffer[read_pos], bytes_to_read);
            std::memset(&buffer[read_pos], 0, bytes_to_read);
        } else {
            size_t first_part = MaxSize - read_pos;
            std::memcpy(data, &buffer[read_pos], first_part);
            std::memcpy(data + first_part, &buffer[0], bytes_to_read - first_part);
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
        ESP_LOGE("FIFOBuffer", "Buffer is empty");
        return 0; // Fehler, Rückgabe von 0
    }
    
    unsigned char byte = buffer[read_pos];
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
        ESP_LOGE("FIFOBuffer", "Index out of range");
        return 0; // Fehler, Rückgabe von 0
    }
    return buffer[(read_pos + index) % MaxSize];
}

template<size_t MaxSize>
void FIFOBuffer<MaxSize>::advanceReadPos(size_t amount) {
    if (amount > size) {
        ESP_LOGE("FIFOBuffer", "Cannot advance read position beyond buffer size");
        amount = size; // Setzen der Menge auf die verfügbare Größe, um Fehler zu vermeiden
    }
    read_pos = (read_pos + amount) % MaxSize;
    size -= amount;
}

template<size_t MaxSize>
void FIFOBuffer<MaxSize>::clear() {
    read_pos = 0;
    write_pos = 0;
    size = 0;
    std::memset(buffer.data(), 0, MaxSize);
}

#endif // FIFO_BUFFER_H