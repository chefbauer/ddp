#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <array>
#include <cstddef>

class FIFOBuffer {
private:
    static constexpr size_t MaxSize = 16384; // Festgelegte Größe
    std::array<unsigned char, MaxSize> buffer;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t size = 0;

public:
    FIFOBuffer(); // Konstruktor, der alle Elemente initialisiert
    size_t write(const unsigned char* data, size_t len);
    size_t read(unsigned char* data, size_t len);
    unsigned char read();
    bool available() const;
    size_t getSize() const;
    size_t getMaxSize() const;
    unsigned char readAt(size_t index) const;
    void advanceReadPos(size_t amount);
    void clear();
    size_t delete(size_t len);
};

#endif // FIFO_BUFFER_H