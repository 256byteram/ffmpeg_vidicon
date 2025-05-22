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

For best results, use 50 or 60 FPS video.

`/path/to/ffmpeg -i input.mp4 -crf 18 -vf "vidicon" -pix_fmt yuv420p output.mp4`

the `"vidicon"` parameter has options to change the gain and fade time:

* `fade` (default 0.5) is from 0.0 to 1.0, where 1.0 gives the longest fade time.
* `gain` (default 1.0) is from 0.0 to 2.0, which adjusts how much input gain there is. Background noise can accumulate if the gain is too high.
* `burn` (default 0.0) is from -1.0 to 1.0 which determines how easily the vidicon tubes are 'burned' - i.e. after being exposed to high brightness, how much that bright spot persists. A value of 0.05 will give a bright spot after exposure, mimicking a good tube. A value of -0.05 will show a dark spot, looking like a worn tube.

Additionally, each option can be adjusted per channel:

* `fade_r`, `fade_g` and `fade_b` for red, green and blue.
* `gain_r`, `gain_g` and `gain_b`.
* `burn_r`, `burn_g` and `burn_b`.

## Examples

`"vidicon=gain=1.1:fade_r=0.6:fade_g=0.6:fade_b=0.7"`
Will set the gain of all channels to 1.1 and the fade of the channels separately, giving slightly blue trails.

`"vidicon=gain=1.0:fade_r=0.6:fade_g=0.6:fade_b=0.7:burn_r=0.1:burn_g=0.3:burn_b=0.1"`
Will give the same blue glow and green comet tails, mimicking a single tube Saticon camera.

`"vidicon=gain=1.0:fade_r=0.6:fade_g=0.6:fade_b=0.7:burn_r=-0.1:burn_g=-0.2:burn_b=-0.1"`
Reduces the gain of the burnt areas of the tube, mimicking a very worn out Saticon camera.

The defaults give a neutral appearance.

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
