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

int main(int argc, char *argv[])
{
    byte debug = 0;
    string filename;
    string parityfile;

    if (argc < 2)
    {
        cout << "Syntax Error: m1play filename [parityfile]" << endl;
        return 1;
    }

    for (int i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            while (argv[i][debug] == 'v' && argv[i][debug] != 0x00)
            {
                debug++;
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

    std::ifstream file;
    file.open(filename, ios_base::binary);
    if (!file.is_open())
        return 0;
    std::vector<char> paritybuf;
    if (!parityfile.empty())
    {
        std::ifstream parity;
        parity.open(parityfile, ios_base::binary);
        file.seekg(0, std::ios_base::end);
        if (!parity.is_open())
            return 0;
        std::streampos fileSizeParity = file.tellg();
        paritybuf.resize(fileSizeParity, 0);
        parity.seekg(0, ios::beg);
        parity.read(&paritybuf[0], fileSizeParity);
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
    byte *frame = (byte *)calloc(0, somevideo.buffersize);

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

    for (int framecount = 0; run; framecount++)
    {
        time = timestamp_ms();
        system("clear");
        run = somevideo.next_frame(frame);

        line = somevideo.width;
        pixels = somevideo.framesize;
        int parityPixel = 0;
        for (int i = 0; i < somevideo.buffersize; i++)
        {
            byte x = frame[i];
            for (int j = 0; j < 8; j++)
            {
                byte bit = x & 0x80;
                if (bit == 128)
                {
                    if (!paritybuf.empty() && paritybuf[parityPixel] != 1)
                    {
                        cout << framecount;
                        return -1;
                    }
                    cout << "██";
                }
                else
                {
                    if (!paritybuf.empty() && paritybuf[parityPixel] != 0)
                    {
                        cout << framecount;
                        return -1;
                    }
                    cout << "  ";
                }
                parityPixel++;
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
        cout << "Frame " << framecount << endl;

        usleep((unsigned int)((1000.0 / somevideo.framerate - (float)(timestamp_ms() - time))) * 1000);
    }
}