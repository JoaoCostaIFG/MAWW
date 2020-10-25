#define _POSIX_C_SOURCE 199309L
#define DEBUG

#include <Imlib2.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <dirent.h>
#include <getopt.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

typedef struct {
  char *dirPath;
  int speed;
} Args;

typedef struct {
  Window root;
  Pixmap pixmap;
  Imlib_Context *render_context;
  int width, height;
} Monitor;

typedef struct {
  int count;
  Imlib_Image *imgs;
} Images;

void debugLog(const char *fmt, ...) {
#ifdef DEBUG
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stderr, fmt, ap);
  va_end(ap);
#endif
}

void setRootAtoms(Display *display, Monitor *monitor) {
  Atom atom_root, atom_eroot, type;
  unsigned char *data_root, *data_eroot;
  int format;
  unsigned long length, after;

  atom_root = XInternAtom(display, "_XROOTMAP_ID", True);
  atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", True);

  // doing this to clean up after old background
  if (atom_root != None && atom_eroot != None) {
    XGetWindowProperty(display, monitor->root, atom_root, 0L, 1L, False,
                       AnyPropertyType, &type, &format, &length, &after,
                       &data_root);

    if (type == XA_PIXMAP) {
      XGetWindowProperty(display, monitor->root, atom_eroot, 0L, 1L, False,
                         AnyPropertyType, &type, &format, &length, &after,
                         &data_eroot);

      if (data_root && data_eroot && type == XA_PIXMAP &&
          *((Pixmap *)data_root) == *((Pixmap *)data_eroot))
        XKillClient(display, *((Pixmap *)data_root));
    }
  }

  atom_root = XInternAtom(display, "_XROOTPMAP_ID", False);
  atom_eroot = XInternAtom(display, "ESETROOT_PMAP_ID", False);

  // setting new background atoms
  XChangeProperty(display, monitor->root, atom_root, XA_PIXMAP, 32,
                  PropModeReplace, (unsigned char *)&monitor->pixmap, 1);
  XChangeProperty(display, monitor->root, atom_eroot, XA_PIXMAP, 32,
                  PropModeReplace, (unsigned char *)&monitor->pixmap, 1);
}

int loadImages(Images *images, const char *const dirPath) {
  debugLog("Loading images\n");

  DIR *const dirp = opendir(dirPath);
  if (dirp == NULL)
    return -1;

  int max_images = 4;
  images->count = 0;
  images->imgs = (Imlib_Image *)malloc(sizeof(Imlib_Image) * max_images);
  if (images->imgs == NULL)
    return -1;

  for (struct dirent *entry; (entry = readdir(dirp));) {
    const char *const imgName = entry->d_name;

    if (strstr(imgName, ".bmp")) {
      if (images->count + 1 > max_images) {
        max_images *= 2;
        Imlib_Image *new_images = (Imlib_Image *)realloc(
            images->imgs, sizeof(Imlib_Image) * max_images);
        if (new_images == NULL)
          return -1;
        images->imgs = new_images;
      }

      char *const imgPath =
          (char *)malloc(strlen(dirPath) + strlen("/") + strlen(imgName) + 1);
      if (imgPath == NULL)
        return -1;
      strcpy(imgPath, dirPath);
      strcat(imgPath, "/");
      strcat(imgPath, imgName);
      images->imgs[images->count++] = imlib_load_image(imgPath);
      free(imgPath);
    }
  }

  return 0;
}

void printUsage() {
  printf("aww -d <bmp-directory> -s <interval between imgs in ms>\n");
  exit(1);
}

void parseArgs(int argc, char **argv, Args *args) {
  if (argc < 3)
    printUsage();

  int gotDir = 0, gotSpeed = 0;
  static struct option long_options[] = {
      {"directory", required_argument, 0, 'd'},
      {"speed", required_argument, 0, 's'},
      {0, 0, 0, 0}};
  int c, option_index = 0;
  while ((c = getopt_long(argc, argv, "d:s:", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'd':
      gotDir = 1;
      args->dirPath = optarg;
      break;
    case 's':
      gotSpeed = 1;
      args->speed = (int)strtol(optarg, NULL, 10);
      break;
    case '?': // invalid option
      printUsage();
      break;
    default:
      fprintf(stderr, "getopt returned character code %#X\n", c);
      break;
    }
  }

  if (optind < argc) {
    printf("non-option ARGV-elements: ");
    while (optind < argc)
      printf("%s ", argv[optind++]);
    printf("\n");
  }

  if (!gotDir || !gotSpeed)
    printUsage();
}

int main(int argc, char *argv[]) {
  Args args;
  parseArgs(argc, argv, &args);

  Images images;
  if (loadImages(&images, args.dirPath) < 0) {
    debugLog("Failed loading images.\n");
    exit(1);
  }

  debugLog("Loading monitors\n");
  Display *display = XOpenDisplay(NULL);
  if (!display) {
    fprintf(stderr, "Could not  open XDisplay\n");
    exit(42);
  }

  const int screen_count = ScreenCount(display);
  debugLog("Found %d screens\n", screen_count);

  Monitor *monitors = malloc(sizeof(Monitor) * screen_count);
  for (int current_screen = 0; current_screen < screen_count;
       ++current_screen) {
    debugLog("Running screen %d\n", current_screen);

    const int width = DisplayWidth(display, current_screen);
    const int height = DisplayHeight(display, current_screen);
    const int depth = DefaultDepth(display, current_screen);
    Visual *vis = DefaultVisual(display, current_screen);
    const int cm = DefaultColormap(display, current_screen);

    debugLog("Screen %d: width: %d, height: %d, depth: %d\n", current_screen,
             width, height, depth);

    Window root = RootWindow(display, current_screen);
    Pixmap pixmap = XCreatePixmap(display, root, width, height, depth);

    monitors[current_screen].width = width;
    monitors[current_screen].height = height;
    monitors[current_screen].root = root;
    monitors[current_screen].pixmap = pixmap;
    monitors[current_screen].render_context = imlib_context_new();
    imlib_context_push(monitors[current_screen].render_context);
    imlib_context_set_display(display);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_drawable(pixmap);
    imlib_context_set_color_range(imlib_create_color_range());
    imlib_context_pop();
  }

  debugLog("Loaded %d screens\n", screen_count);
  debugLog("Starting render loop");

  struct timespec timeout;
  timeout.tv_sec = 0;
  timeout.tv_nsec = 100000000;

  for (unsigned int cycle = 0; /* true */; ++cycle) {
    Imlib_Image current = images.imgs[cycle % images.count];
    for (int monitor = 0; monitor < screen_count; ++monitor) {
      Monitor *c_monitor = &monitors[monitor];
      imlib_context_push(c_monitor->render_context);
      imlib_context_set_dither(1);
      imlib_context_set_blend(1);
      imlib_context_set_image(current);

      imlib_render_image_on_drawable(0, 0);

      setRootAtoms(display, c_monitor);
      XKillClient(display, AllTemporary);
      XSetCloseDownMode(display, RetainTemporary);
      XSetWindowBackgroundPixmap(display, c_monitor->root, c_monitor->pixmap);
      XClearWindow(display, c_monitor->root);
      XFlush(display);
      XSync(display, False);
      imlib_context_pop();
    }
    nanosleep(&timeout, NULL);
  }
}
