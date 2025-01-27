#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <array>
#include <cstddef>
#include <memory>

template<size_t MaxSize>
class FIFOBuffer {
private:
    std::array<unsigned char, MaxSize> buffer;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t size = 0;

public:
    FIFOBuffer() : buffer{} {}
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

#endif // FIFO_BUFFER_H