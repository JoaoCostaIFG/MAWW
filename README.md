# MAWW (More Animated Wallpapers)

## TODO

- image loading order
- This looks cool: `./paperview FOLDER SPEED X Y W H FOLDER SPEED X Y W H`

## Creating a scene (from paperview)

Creating a custom BMP scene folder from a GIF requires imagemagick.
For example, to create a castle scene folder from a castle.gif:

```bash
  mkdir castle
  mv castle.gif castle
  cd castle
  convert -coalesce castle.gif out.bmp
  rm castle.gif
```

## Inspirations and source of many things

[paperview](https://github.com/glouw/paperview)
[X11 only alternative](https://gist.github.com/AlecsFerra/ef1cc008990319f3b676eb2d8aa89903)
