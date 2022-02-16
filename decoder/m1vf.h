#include <cmath>
#include <cstring>
#define bytes_to_u16(MSB,LSB) (((uint16_t) (MSB)) & 255)<<8 | ((LSB)&255)
#define byte_to_rep(byte) (byte & 0b00011111)
#define byte_to_algo(byte) (byte >> 5)
#define msg(message) if (_debug >= 1) std::cout << message << std::endl
#define msg2(message) if (_debug >= 2) std::cout << "    " << message << std::endl
#define msg3(message) if (_debug >= 3) std::cout << "        " << message << std::endl

using byte = unsigned char;

size_t ceil_div(size_t x, size_t y) {
    return (x % y) ? x / y + 1 : x / y;
}

int intpow (int x, int p) {
    int i = 1;
    for (int j = 1; j <= p; j++)  i *= x;
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

    void set_byte_bit(size_t byte_index, byte bit_index, bool color) {
        msg3("sbb> byte_index: " << byte_index << "   bit_index: " << int(bit_index));
        if (color) _buffer[byte_index] |= 128/intpow(2, bit_index);
        else _buffer[byte_index] &= 255 - 128/intpow(2, bit_index);
    }

    void set_column_row(size_t column_index, size_t row_index, bool color) {
        msg3("scr> column_index: " << column_index << "   row_index: " << row_index);
        size_t totalbits = (row_index*width)+column_index;
        size_t byte_index = totalbits / 8;
        byte bit_index = totalbits % 8;
        set_byte_bit(byte_index, bit_index, color);
    }

    void decode_uncompressed() {
        memcpy(_buffer, &_data[_index], _buffer_size);
        _index += _buffer_size;
    }

    void decode_identical_frame() {}

    void decode_run_length_row() {
        msg2("rlr> start");
        int totalbits = 0;
        int totalbytes = 0;
        while(totalbits < framesize) {
            // read from data
            bool color = _data[_index] & 0b10000000;
            byte repeatbits = (_data[_index] & 0b01111111)+1;
            _index++;
            msg2("rlr> color: " << color << "   repeatbits: " << int(repeatbits));

            // write to buffer
            totalbytes--;
            for (int i = totalbits%8; i < totalbits%8+repeatbits; i++) {
                byte bitindex = i % 8;
                if (bitindex == 0) totalbytes++;
                msg2("rlr> writer: " << i << "   byte_index: " << totalbytes <<  "   bit_index: " << int(bitindex));
                set_byte_bit(totalbytes, bitindex, color);
            }
            totalbits += repeatbits;
            totalbytes++;
        }
        msg2("rlr> finish");
    }

    void decode_run_length_column() {
        msg2("rlc> start");
        int column_index = 0;
        int row_index = 0;

        while(column_index+1 != width && row_index+1 != height) {
            // read from data
            bool color = _data[_index] & 0b10000000;
            byte repeatbits = (_data[_index] & 0b01111111)+1;
            _index++;
            msg2("rlc> color: " << color << "   repeatbits: " << int(repeatbits));

            // write to buffer
            for (int i = 0; i < repeatbits; i++) {
                if (row_index >= height) {
                    row_index = 0;
                    column_index++;
                }
                msg2("rlc> writer iteration start: " << i << "   column_index: " << int(column_index) << "   row_index: " << int(row_index) );
                set_column_row(column_index, row_index, color);
                row_index++;
            }
        }
        msg2("rlc> finish");
    }

    void decode_changed_rows_uncompressed() {}
    void decode_changed_rows_run_length() {}
    void decode_changed_columns_uncompressed() {}
    void decode_changed_columns_run_length() {}

public:
    uint16_t width;
    uint16_t height;
    uint8_t framerate;
    size_t framesize;
    size_t buffersize;
    size_t frameindex;

    M1VF(byte *data, size_t size, byte debug=0) {
        _data = data;
        _size = size;
        _index = 0;
        _repeat = 0;
        _algorithm = 0;
        frameindex = 0;
        _debug = debug;
        width = bytes_to_u16(_data[0], _data[1]);
        height = bytes_to_u16(_data[2], _data[3]);
        framerate = _data[4];
        _buffer_size = ceil_div(width * height, 8.0);
        buffersize = _buffer_size;
        framesize = width*height;
        _buffer = (byte*) malloc(_buffer_size);
        for (int i = 0; i < _buffer_size; i++) {
            _buffer[i] = 0x00;
        }
        _index = 5;
    }

    bool next_frame(byte *frame) {
        msg("nxt> frame: " << frameindex);
        frameindex++;
        if (_repeat == 0) {
            _algorithm = byte_to_algo(_data[_index]);
            _repeat = byte_to_rep(_data[_index]);
            _index++;
            msg("nxt> new algorithm: " << int(_algorithm) << "   repeat: " << int(_repeat));
        } else _repeat--;
        msg("nxt> algorithm: " << int(_algorithm));
        switch (_algorithm) {
            case 0:
                decode_uncompressed();
                break;
            case 1:
                decode_identical_frame();
                break;
            case 2:
                decode_run_length_row();
                break;
            case 3:
                decode_run_length_column();
                break;
            case 4:
                decode_changed_rows_uncompressed();
                break;
            case 5:
                decode_changed_rows_run_length();
                break;
            case 6:
                decode_changed_columns_uncompressed();
                break;
            case 7:
                decode_changed_columns_run_length();
                break;
            default:
                return false;
        }
        memcpy(frame, _buffer, _buffer_size);
        if (_index >= _size) return false;
        return true;
    }
};
