# decoder
C++ header to read m1vf videos.

## Documentation

To create a new M1VF instance, provide a bytearray containing a video, its size and optionally a level of debug:

```
M1VF somevideo(bytes, size, debug);
```

`debug` has a default value of 0, where no debug messages are printed. It ranges up to a level of 3.

To then get the first frame, call `next_frame`:

```
somevideo.next_frame(frame);
```
This fills the bytearray `frame` with the respective next frame, 1bpp. Starts at frame 0.
`next_frame` returns false if the requested frame was the last one.

Some useful information is also contained in public variables:

* `uint16_t width` width in pixels.
* `uint16_t height` height in pixels.
* `uint8_t framerate` framerate in fps.
* `size_t framesize` frame area in pixels
* `size_t buffersize` frame size in bytes
* `size_t frameindex` index of the last requested frame.
