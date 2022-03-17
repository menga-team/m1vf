# m1enc
Converts a video file or a folder of images into an m1vf video.
---

## Usage
```
m1enc 0.1.0

USAGE:
    m1enc [OPTIONS] --input <INPUT> --output <OUTPUT> --width <WIDTH> --height <HEIGHT>

OPTIONS:
    -h, --height <HEIGHT>          Height of the output video
        --help                     Print help information
    -i, --input <INPUT>            The source, can be either a folder with frames, or a video which
                                   will get processed to a folder with frames by ffmpeg.
    -o, --output <OUTPUT>          The output file, the same path, but with a different file
                                   extension will be used for files like parity check.
    -p, --parity                   Generates a parity file along with the m1vf file. Will use the
                                   file specified in --output, but with an different file extension.
    -r, --framerate <FRAMERATE>    The encoded framerate [default: 30]
    -t, --text                     Generates a text file, similar to an flipbook (which can come in
                                   handy when debugging), along with the m1vf file. Will use the
                                   file specified in --output, but with an different file extension.
    -V, --version                  Print version information
    -w, --width <WIDTH>            Width of the output video      
```

## Example
```
m1enc -i "BadApple!!.mp4" -w 20 -h 16 -r 30 -o badapple.m1vf
```
Converts `BadApple!!.mp4` to an 20x16@30 m1vf encoded video.