#include <cmath>
#include <cstring>

#include <m1vf.h>

void M1VF::set_byte_bit(size_t byte_index, byte bit_index, bool color) {
    msg3("sbb> byte_index: " << byte_index << "   bit_index: " << int(bit_index));
    if (color)
        _buffer[byte_index] |= 128 / intpow(2, bit_index);
    else
        _buffer[byte_index] &= 255 - 128 / intpow(2, bit_index);
}

void M1VF::set_column_row(size_t column_index, size_t row_index, bool color) {
    msg3("scr> column_index: " << column_index << "   row_index: " << row_index);
    size_t totalbits = (row_index * width) + column_index;
    size_t byte_index = totalbits / 8;
    byte bit_index = totalbits % 8;
    set_byte_bit(byte_index, bit_index, color);
}

void M1VF::decode_uncompressed() {
    memcpy(_buffer, &_data[_index], _buffer_size);
    _index += _buffer_size;
}

void M1VF::decode_identical_frame() {}

void M1VF::decode_run_length_row() {
    msg2("rlr> start");
    int totalbits = 0;
    while (totalbits < framesize) {
        // read from data
        bool color = _data[_index] & 0b10000000;
        byte repeatbits = (_data[_index] & 0b01111111) + 1;
        _index++;
        msg2("rlr> color: " << color << "   repeatbits: " << int(repeatbits));

        for (int i = totalbits; i < totalbits + repeatbits; i++) {
            int byte = i / 8;
            set_byte_bit(byte, i % 8, color);
        }
        totalbits += repeatbits;
    }
    msg2("rlr> finish");
}

void M1VF::decode_run_length_column() {
    int totalbits = 0;
    while (totalbits < framesize) {
        // read from data
        bool color = _data[_index] & 0b10000000;
        byte repeatbits = (_data[_index] & 0b01111111) + 1;
        _index++;
        for (int i = totalbits; i < totalbits + repeatbits; i++)
        {
            int index = (i % height) * width + (i / height);
            set_byte_bit(index / 8, index % 8, color);
        }
        totalbits += repeatbits;
    }
}

void M1VF::decode_changed_rows_uncompressed() {}

void M1VF::decode_changed_rows_run_length() {}

void M1VF::decode_changed_columns_uncompressed() {}

void M1VF::decode_changed_columns_run_length() {}

M1VF::M1VF(byte *data, size_t size, byte debug) {
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
    framesize = width * height;
    _buffer = (byte *)malloc(_buffer_size);
    for (int i = 0; i < _buffer_size; i++) {
        _buffer[i] = 0x00;
    }
    _index = 5;
}

bool M1VF::next_frame(byte *frame) {
    msg("nxt> frame: " << frameindex);
    frameindex++;
    if (_repeat == 0) {
        _algorithm = byte_to_algo(_data[_index]);
        _repeat = byte_to_rep(_data[_index]);
        _index++;
        msg("nxt> new algorithm: " << int(_algorithm) << "   repeat: " << int(_repeat));
    }
    else
        _repeat--;
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
