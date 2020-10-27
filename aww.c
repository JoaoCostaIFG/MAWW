#define _POSIX_C_SOURCE 199309L
#define DEBUG

#include <Imlib2.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>

#include <dirent.h>
#include <getopt.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define SEC2MILI 1000
#define MILI2NANO 1000000

volatile int STOP = False;

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
  Display *display;
  int screen_count;
  Monitor *monitors;
} Video;

typedef struct {
  int count;
  Imlib_Image *imgs;
} Images;

void debugLog(const char *fmt, ...) {
#ifdef DEBUG
  va_list ap;
  va_start(ap, fmt);
  vfprintf(stdout, fmt, ap);
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
      images->imgs[images->count] = imlib_load_image(imgPath);
      ++images->count;
      free(imgPath);
    }
  }

  closedir(dirp);
  return 0;
}

int setupMonitors(Video *video) {
  debugLog("Loading monitors\n");
  video->display = XOpenDisplay(NULL);
  if (!video->display)
    return -1;

  video->screen_count = ScreenCount(video->display);
  debugLog("Found %d screens\n", video->screen_count);

  video->monitors = (Monitor *)malloc(sizeof(Monitor) * video->screen_count);
  if (!video->monitors)
    return -1;
  for (int curr_screen = 0; curr_screen < video->screen_count; ++curr_screen) {
    debugLog("Running screen %d\n", curr_screen);
    Monitor *mon = &video->monitors[curr_screen];

    mon->width = DisplayWidth(video->display, curr_screen);
    mon->height = DisplayHeight(video->display, curr_screen);
    const int depth = DefaultDepth(video->display, curr_screen);
    Visual *vis = DefaultVisual(video->display, curr_screen);
    const int cm = DefaultColormap(video->display, curr_screen);

    debugLog("Screen %d: width: %d, height: %d, depth: %d\n", curr_screen,
             mon->width, mon->height, depth);

    mon->root = RootWindow(video->display, curr_screen);
    mon->pixmap = XCreatePixmap(video->display, mon->root, mon->width,
                                mon->height, depth);

    mon->render_context = imlib_context_new();
    imlib_context_push(mon->render_context);
    imlib_context_set_display(video->display);
    imlib_context_set_visual(vis);
    imlib_context_set_colormap(cm);
    imlib_context_set_drawable(mon->pixmap);
    imlib_context_pop();
  }

  debugLog("Loaded %d screens\n", video->screen_count);
  return 0;
}

void printUsage() {
  printf("aww -d <bmp-directory> -s <interval between imgs in ms>\n");
  exit(1);
}

void parseArgs(int argc, char **argv, Args *args) {
  if (argc < 3)
    printUsage();

  int gotDir = False, gotSpeed = False;
  static struct option long_options[] = {
      {"directory", required_argument, 0, 'd'},
      {"speed", required_argument, 0, 's'},
      {0, 0, 0, 0}};
  int c, option_index = 0;
  while ((c = getopt_long(argc, argv, "d:s:", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'd':
      gotDir = True;
      args->dirPath = optarg;
      break;
    case 's':
      gotSpeed = True;
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

void sig_handler(int signo) {
  if (signo != SIGINT)
    return;
  debugLog("SIGINT received.\n");
  STOP = True;
}

int main(int argc, char *argv[]) {
  Args args;
  parseArgs(argc, argv, &args);

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1) // doesn't kill
    fprintf(stderr, "Failed setting SIGINT handler.\n");

  Images images;
  if (loadImages(&images, args.dirPath) < 0) {
    fprintf(stderr, "Failed loading images.\n");
    exit(1);
  }

  Video video;
  if (setupMonitors(&video) < 0) {
    fprintf(stderr, "Failed setting up monitors.\n");
    exit(1);
  }

  debugLog("Starting render loop\n");
  struct timespec timeout;
  timeout.tv_sec = args.speed / SEC2MILI;
  timeout.tv_nsec = (args.speed % SEC2MILI) * MILI2NANO;

  for (unsigned int cycle = 0; !STOP; ++cycle) {
    Imlib_Image *curr_img = &images.imgs[cycle % images.count];
    for (int monitor = 0; monitor < video.screen_count; ++monitor) {
      Monitor *mon = &video.monitors[monitor];
      imlib_context_push(mon->render_context);
      imlib_context_set_image(*curr_img);

      /* imlib_render_image_on_drawable(0, 0); */
      imlib_context_set_anti_alias(1);
      imlib_render_image_on_drawable_at_size(0, 0, 1920, 1080);

      setRootAtoms(video.display, mon); // only needed when switching screens
      XSetCloseDownMode(video.display, RetainTemporary);
      XKillClient(video.display, AllTemporary);
      XSetWindowBackgroundPixmap(video.display, mon->root, mon->pixmap);
      XClearWindow(video.display, mon->root);
      XFlush(video.display);
      imlib_context_pop();
    }
    nanosleep(&timeout, NULL);
  }

  free(images.imgs);
  for (int monitor = 0; monitor < video.screen_count; ++monitor) {
    imlib_context_free(video.monitors[monitor].render_context);
  }
  free(video.monitors);
}
