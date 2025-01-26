#ifndef FIFO_BUFFER_H
#define FIFO_BUFFER_H

#include <vector>
#include <cstddef>

class FIFOBuffer {
private:
    std::vector<unsigned char> buffer;
    size_t write_pos = 0;
    size_t read_pos = 0;
    size_t size = 0;

public:
    FIFOBuffer(size_t max_size);
    size_t write(const unsigned char* data, size_t len);
    size_t read(unsigned char* data, size_t len); // Neue Methode
    unsigned char read(); // Behält die alte read-Methode bei
    bool available() const;
    size_t getSize() const;
    size_t getMaxSize() const;
    unsigned char readAt(size_t index) const; // Für den Zugriff auf ein Byte ohne Leseposition zu verändern
    void advanceReadPos(size_t amount); // Erhöht die Leseposition
    void clear(); // Lösche puffer, neustart
};

#endif // FIFO_BUFFER_H