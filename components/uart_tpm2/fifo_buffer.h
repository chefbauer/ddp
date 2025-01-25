#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <vector>

class FIFOBuffer {
private:
    std::vector<unsigned char> buffer;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t size = 0;

public:
    FIFOBuffer(size_t max_size);
    size_t write(const unsigned char* data, size_t len);
    unsigned char read();
    bool available() const;
    size_t getSize() const;
    size_t getMaxSize() const;
};

#endif // FIFO_BUFFER_H