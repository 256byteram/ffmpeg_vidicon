FFmpeg with additional vidicon filter
=====================================

This is a fork of the git.ffmpeg.org release/6.1 repository.

I have added an additional filter which attempts to simulate a colour vidicon camera.

## Compile

My development system is Linux. Windows and Mac is documented elsewhere.

In the repository folder:

`./configure --enable-filter=vidicon --enable-libx264 --enable-gpl`
`make -j8`

Eventually, assuming everything works, you'll have an `ffmpeg` binary available.

## Usage

`/path/to/ffmpeg -i input.mp4 -crf 18 -vf "vidicon" -pix_fmt yuv420p output.mp4

the `"vidicon"` paramater has options to change the gain and fade time:

* `fade` is a number from 0.0 to 1.0, where 1.0 gives the longest fade time.
* `gain` is a number from 0.0 to 1.0, which adjusts how much input gain there is. Background noise can accumulate if the gain is too high.

Additionally, each option can be adjusted per channel:

* `fade_r`, `fade_g` and `fade_b` for red, green and blue.
* `gain_r`, `gain_g` and `gain_b`.

For example:

`"vidicon=gain=0.8:fade_r=0.5:fade_g=0.3:fade_b=0.6"`

Will set the gain of all channels to 0.8 and the fade of the channels separately.

The original FFmpeg README follows.

FFmpeg README
=============

FFmpeg is a collection of libraries and tools to process multimedia content
such as audio, video, subtitles and related metadata.

## Libraries

* `libavcodec` provides implementation of a wider range of codecs.
* `libavformat` implements streaming protocols, container formats and basic I/O access.
* `libavutil` includes hashers, decompressors and miscellaneous utility functions.
* `libavfilter` provides means to alter decoded audio and video through a directed graph of connected filters.
* `libavdevice` provides an abstraction to access capture and playback devices.
* `libswresample` implements audio mixing and resampling routines.
* `libswscale` implements color conversion and scaling routines.

## Tools

* [ffmpeg](https://ffmpeg.org/ffmpeg.html) is a command line toolbox to
  manipulate, convert and stream multimedia content.
* [ffplay](https://ffmpeg.org/ffplay.html) is a minimalistic multimedia player.
* [ffprobe](https://ffmpeg.org/ffprobe.html) is a simple analysis tool to inspect
  multimedia content.
* Additional small tools such as `aviocat`, `ismindex` and `qt-faststart`.

## Documentation

The offline documentation is available in the **doc/** directory.

The online documentation is available in the main [website](https://ffmpeg.org)
and in the [wiki](https://trac.ffmpeg.org).

### Examples

Coding examples are available in the **doc/examples** directory.

## License

FFmpeg codebase is mainly LGPL-licensed with optional components licensed under
GPL. Please refer to the LICENSE file for detailed information.

## Contributing

Patches should be submitted to the ffmpeg-devel mailing list using
`git format-patch` or `git send-email`. Github pull requests should be
avoided because they are not part of our review process and will be ignored.
