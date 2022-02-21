#include <iostream>
#include <fstream>
#include <vector>
#include <unistd.h>
#include <sys/time.h>
#include "../decoder/m1vf.h"
#include <cstring>

using namespace std;

long long timestamp_ms()
{
    struct timeval te;
    gettimeofday(&te, NULL);
    long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000;
    return milliseconds;
}

void notify_parity_mismatch(byte parity, byte read, unsigned int frame)
{
    cout << "Decoded value does not match with parity file. (Frame " << frame << ")" << endl
         << "Read " << read << ", but should be " << parity << endl
         << "0  = Should be black, but is white." << endl
         << "1  = Should be white, but is black.";
}

int main(int argc, char *argv[])
{
    byte debug = 0;
    string filename;
    string parityfile;
    bool stop_on_mismatch = false;

    if (argc < 2)
    {
        cout << "Syntax Error: m1play [-vvv] [--stop-on-mismatch] filename [parityfile]" << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            if (argv[i][1] == 'v')
            {
                while (argv[i][debug] == 'v' && argv[i][debug] != 0x00)
                {
                    debug++;
                }
            }
            else if (strcmp(argv[i], "--stop-on-mismatch") == 0)
            {
                stop_on_mismatch = true;
            }
        }
        else if (!filename.empty())
        {
            parityfile = argv[i];
        }
        else
        {
            filename = argv[i];
        }
    }

    if (stop_on_mismatch && parityfile.empty())
    {
        cout << "Can not stop on mismatch when no parity file is given." << endl;
        return -1;
    }

    std::ifstream file;
    file.open(filename, ios_base::binary);
    if (!file.is_open())
        return 0;
    std::vector<byte> paritybuf;
    if (!parityfile.empty())
    {
        std::ifstream parity;
        parity.open(parityfile, ios_base::binary);
        if (!parity.is_open())
            return 0;
        parity.seekg(0, std::ios_base::end);
        size_t fileSizeParity = parity.tellg();
        parity.seekg(0, ios::beg);
        paritybuf.resize(fileSizeParity, 1);
        parity.read(reinterpret_cast<char *>(&paritybuf[0]), fileSizeParity);
        parity.close();
    }
    file.seekg(0, ios::end);
    size_t fileSize = file.tellg();
    file.seekg(0, ios::beg);
    std::vector<byte> data(fileSize, 0);
    file.read(reinterpret_cast<char *>(&data[0]), fileSize);
    file.close();

    byte *bytes = &data[0];
    int size = data.size();

    M1VF somevideo(bytes, size, debug);

    int line;
    int pixels;
    bool run = true;
    byte *frame = (byte *)malloc(somevideo.buffersize);

    long long time;

    if (debug > 0)
    {
        while (run)
        {
            time = timestamp_ms();
            run = somevideo.next_frame(frame);
            usleep((unsigned int)((1000.0 / somevideo.framerate - (float)(timestamp_ms() - time))) * 1000);
        }
        return 0;
    }
    int parity_pixel = 0;
    for (int framecount = 0; run; framecount++)
    {
        time = timestamp_ms();
        system("clear");
        run = somevideo.next_frame(frame);

        line = somevideo.width;
        pixels = somevideo.framesize;
        for (int i = 0; i < somevideo.buffersize; i++)
        {
            byte x = frame[i];
            for (int j = 0; j < 8; j++)
            {
                byte bit = x & 0x80;
                if (bit == 128)
                {
                    if (!paritybuf.empty() && paritybuf[parity_pixel] != 1)
                    {
                        cout << "\x1B[41m" << "0 " << "\e[0m";
                        if (stop_on_mismatch)
                        {
                            run = false;
                        }
                    }
                    else
                        cout << "██";
                }
                else
                {
                    if (!paritybuf.empty() && paritybuf[parity_pixel] != 0)
                    {
                        cout << "\x1B[42m" << "1 " << "\e[0m";
                        if (stop_on_mismatch)
                        {
                            run = false;
                        }
                    }
                    else
                        cout << "  ";
                }
                parity_pixel++;
                x <<= 1;
                line--;
                if (line == 0)
                {
                    cout << endl;
                    line = somevideo.width;
                }
                pixels--;
                if (pixels == 0)
                    break;
            }
        }

        for (int i = 0; i < somevideo.width; i++)
        {
            cout << "==";
        }
        cout << endl;
        cout << "Frame " << framecount << " ";
        if (!paritybuf.empty())
            cout << "Parity index " << parity_pixel;
        cout << endl;

        usleep((unsigned int)((1000.0 / somevideo.framerate - (float)(timestamp_ms() - time))) * 1000);
    }
}