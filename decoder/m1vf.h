#ifndef M1VF_H
#define M1VF_H

#include <cmath>
#include <cstring>
#include <iostream>

#define bytes_to_u16(MSB, LSB) (((uint16_t)(MSB)) & 255) << 8 | ((LSB)&255)
#define byte_to_rep(byte) (byte & 0b00011111)
#define byte_to_algo(byte) (byte >> 5)
#define msg(message) if (_debug >= 1) std::cout << message << std::endl
#define msg2(message) if (_debug >= 2) std::cout << "    " << message << std::endl
#define msg3(message) if (_debug >= 3) std::cout << "        " << message << std::endl

using byte = unsigned char;

inline size_t ceil_div(size_t x, size_t y) {
    return (x % y) ? x / y + 1 : x / y;
}

inline int intpow(int x, int p) {
    int i = 1;
    for (int j = 1; j <= p; j++)
        i *= x;
    return i;
}

struct M1VF {
private:
    byte *_data;
    size_t _index;
    byte *_buffer;
    size_t _buffer_size;
    size_t _size;
    byte _algorithm;
    byte _repeat;
    byte _debug;

    // utility
    void set_byte_bit(size_t byte_index, byte bit_index, bool color);
    void set_column_row(size_t column_index, size_t row_index, bool color);

    // algorithms
    void decode_uncompressed();
    void decode_identical_frame();
    void decode_run_length_row();
    void decode_run_length_column();
    void decode_changed_rows_uncompressed();
    void decode_changed_rows_run_length();
    void decode_changed_columns_uncompressed();
    void decode_changed_columns_run_length();

public:
    uint16_t width;
    uint16_t height;
    uint8_t framerate;
    size_t framesize;
    size_t buffersize;
    size_t frameindex;

    M1VF(byte *data, size_t size, byte debug = 0);

    bool next_frame(byte *frame);
};

#endif  // M1VF_H