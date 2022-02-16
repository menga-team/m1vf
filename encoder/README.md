# m1enc
Converts a video file or a folder of images into an m1vf video.
---

## Usage
```
m1enc 0.1.0

USAGE:
    m1enc [OPTIONS] --input <INPUT> --output <OUTPUT> --width <WIDTH> --height <HEIGHT>

OPTIONS:
    -h, --height <HEIGHT>          
        --help                     Print help information
    -i, --input <INPUT>            
    -o, --output <OUTPUT>          
    -r, --framerate <FRAMERATE>    [default: 30]
    -V, --version                  Print version information
    -w, --width <WIDTH>            
```

## Example
```
m1enc -i "BadApple!!.mp4" -w 20 -h 16 -r 30 -o badapple.m1vf
```
Converts `BadApple!!.mp4` to an 20x16@30 m1vf encoded video.