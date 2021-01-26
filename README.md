# MAWW (More Animated Wallpapers)

## Usage

Basic usage:

```bash
maww -d <scene directory> [-s <interval between imgs in ms>]
```

In case you have multiple monitors, you may want to display the image more than once,
for example, 2 monitors at 1920x1080 could have an image displayed in each one instead
of having it stretched. You can specify multiple monitors at the end of the command
line like so:

```bash
[<x> <y> <width> <height>]
```

## Creating a scene (taken from paperview's README)

Creating a custom BMP scene folder from a GIF requires imagemagick.
For example, to create a castle scene folder from a castle.gif:

```bash
  mkdir castle
  mv castle.gif castle
  cd castle
  convert -coalesce castle.gif out.bmp
  rm castle.gif
```

This repository contains an example scene depicting Anor Londo from Dark Souls III.
[Source](https://reddit.com/r/gaming/comments/4jdw0t/pixel_dark_souls_3_irithyll_of_the_boreal_valley/)
for this artwork.

## Inspirations and source of many things

[paperview](https://github.com/glouw/paperview)
[X11 only alternative](https://gist.github.com/AlecsFerra/ef1cc008990319f3b676eb2d8aa89903)

## License

This work is licensed under the MIT license.
