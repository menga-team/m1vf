# m1vf
The Menga 1-Bit Video Format.

A video codec designed for storing Bad Apple (or any other 1-bit video) space-efficiently.
The format is intended to store small resolution videos, neither the encoder nor the specification are optimized for larger resolutions.

* [Tools](#tools)
* [Installation](#installation)
* [Specification](#specification)

## Tools

* **m1play**: in-terminal m1vf player, for documentation see [player](player/)
* **m1enc**: m1vf encoder, for documentation see [encoder](encoder/)

## Installation

To install m1vf-tools, clone the repository and install via makefile:
```
git clone https://github.com/menga-team/m1vf
cd m1vf
```
```
sudo make install
```
To uninstall m1vf-tools, run
```
sudo make uninstall
```

## Specification


The first 4 bytes are 2 u16 and contain the width and height of the video.

The next byte (u8) is the framerate in fps.

The remaining bytes contain the frames:

The decoder is expected to start with a buffer filled with black (0).
The first 3 bits of the first byte in a frame specifies the algorithm used to decode the frame data, the remaining 5 bits indicate
how many times the same algorithm is applied. Therefore, a total of 8 different algorithms and 32 algorithm
repetitions are possible.

Algorithms:

* `0x0` uncompressed: 1bpp.
* `0x1` identical-frame: The frame is identical to the last one.
* `0x2` run-length-row: The first bit is the color and the remaining 7 bits indicate the number of horizontal repetitons. The frame is completed when the sum of all bits is width * height.
* `0x3` run-length-column: The first bit is the color and the remaining 7 bits indicate the number of vertical repetitions. The frame is completed when the sum of all bits is width * height.
* `0x4` changed-rows-uncompressed: only encodes changed rows in 1bpp, the rows get indicated in a bitmap where 0 = unchanged and 1 = changed.
* `0x5` changed-rows-run-length: only encodes changed rows using run-length-row, the rows get indicated in a bitmap where 0 = unchanged and 1 = changed.
* `0x6` changed-columns-uncompressed: only encodes changed columns in 1bpp, the rows get indicated in a bitmap where 0 = unchanged and 1 = changed.
* `0x7` changed-columns-run-length: only encodes changed columns using run-length-column, the rows get indicated in a bitmap where 0 = unchanged and 1 = changed.
