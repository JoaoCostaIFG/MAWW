/* #define _POSIX_C_SOURCE 199309L */
#define _DEFAULT_SOURCE
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
  int isRandom;
  int speed;
  int monitorCount;
  int *monitorSettings;
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

void sig_handler(int signo) {
  if (signo != SIGINT)
    return;
  debugLog("SIGINT received.\n");
  STOP = True;
}

void die(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);

  vfprintf(stderr, fmt, ap);
  if (fmt[0] && fmt[strlen(fmt) - 1] == ':') {
    fputc(' ', stderr);
    perror(NULL);
  } else {
    fputc('\n', stderr);
  }

  va_end(ap);
  exit(1);
}

/* ARGS */
void printUsage() {
  printf("maww -d <bmp-directory> -s <interval between imgs in ms>\n"
         "\t[<monitor count> <x> <y> <width> <height>]\n");
  exit(1);
}

void parseArgs(int argc, char **argv, Args *args) {
  if (argc < 3)
    printUsage();

  /* load default values */
  args->isRandom = False;
  args->speed = 67; // 15fps based on number of frames
  // default drawing location (1920x1080 at 0;0)
  args->monitorCount = 1;
  args->monitorSettings = (int *)malloc(sizeof(int) * 4);
  if (!args->monitorSettings)
    die("parseArgs: Failed malloc operation.");
  args->monitorSettings[0] = 0;
  args->monitorSettings[1] = 0;
  args->monitorSettings[2] = 1920;
  args->monitorSettings[3] = 1080;

  /* parse arguments */
  int gotDir = False;
  static struct option long_options[] = {
      {"directory", required_argument, 0, 'd'},
      {"random", required_argument, 0, 'r'},
      {"speed", required_argument, 0, 's'},
      {0, 0, 0, 0}};
  int c, option_index = 0;
  while ((c = getopt_long(argc, argv, "d:r:s:", long_options, &option_index)) !=
         -1) {
    switch (c) {
    case 'r': // intended fallthrough
      args->isRandom = True;
    case 'd':
      gotDir = True;
      args->dirPath = optarg;
      break;
    case 's':
      args->speed = (int)strtol(optarg, NULL, 10);
      break;
    case '?': // invalid option
    case ':': // missing arg
      printUsage();
      break;
    default:
      fprintf(stderr, "getopt returned character code %#X\n", c);
      break;
    }
  }

  if (optind < argc) {
    args->monitorCount = (argc - optind) / 4;
    if (!args->monitorCount || argc - optind < args->monitorCount * 4)
      printUsage(); // TODO better err msg
    int *newMonitorSettings = (int *)realloc(
        args->monitorSettings, sizeof(int) * 4 * args->monitorCount);
    if (!newMonitorSettings)
      die("parseArgs: Failed realloc operation.");
    args->monitorSettings = newMonitorSettings;

    for (int i = 0; optind < argc;) {
      args->monitorSettings[i++] = (int)strtol(argv[optind++], NULL, 10);
      args->monitorSettings[i++] = (int)strtol(argv[optind++], NULL, 10);
      args->monitorSettings[i++] = (int)strtol(argv[optind++], NULL, 10);
      args->monitorSettings[i++] = (int)strtol(argv[optind++], NULL, 10);
    }

    // if there's still extras, we don't know what these are
    if (optind < argc) {
      printf("Unrecognized arguments: ");
      while (optind < argc)
        printf("%s ", argv[optind++]);
      printf("\n");
    }
  }

  if (!gotDir)
    printUsage();
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

int getRandomDir(Args *args) {
  debugLog("Choosing random image directory\n");
  const char *const dirPath = args->dirPath;

  DIR *const dirp = opendir(dirPath);
  if (dirp == NULL)
    return -1;

  int dirCount = 0;
  for (struct dirent *entry; (entry = readdir(dirp));) {
    const char *const entryName = entry->d_name;
    if (!strcmp(entryName, ".") || !strcmp(entryName, ".."))
      continue;
    if (entry->d_type == DT_DIR)
      ++dirCount;
  }

  if (dirCount == 0)
    die("getRandomDir: couldn't find any directory.");
  int chosenDirInd = (rand() % dirCount) + 1;

  rewinddir(dirp);
  for (struct dirent *entry; dirCount > 0 && (entry = readdir(dirp));) {
    const char *const entryName = entry->d_name;
    if (!strcmp(entryName, ".") || !strcmp(entryName, ".."))
      continue;

    if (entry->d_type == DT_DIR) {
      --chosenDirInd;
      // save chosen dir
      if (chosenDirInd == 0) {
        int chosenDirLen = strlen(dirPath) + strlen(entryName) + 1; // %s/%s
        args->dirPath = malloc(chosenDirLen + 1);                   // '\0'
        if (!args->dirPath)
          die("getRandomDir: Failed alloc operation.");
        sprintf(args->dirPath, "%s/%s", dirPath, entryName);
      }
    }
  }

  debugLog("getRandomDir: Choose ", args->dirPath);
  closedir(dirp);
  return 0;
}

int pathCompare(const void *a, const void *b) {
  const char *stra = *(const char **)a;
  const char *strb = *(const char **)b;

  char *sa = strstr(stra, ".bmp");
  while (*(sa - 1) != '-')
    --sa;
  long la = strtol(sa, NULL, 10);

  char *sb = strstr(strb, ".bmp");
  while (*(sb - 1) != '-')
    --sb;
  long lb = strtol(sb, NULL, 10);

  if (la > lb)
    return 1;
  else if (la < lb)
    return -1;
  else
    return 0;
}

int loadImages(Images *images, const char *const dirPath) {
  debugLog("Loading images\n");

  DIR *const dirp = opendir(dirPath);
  if (dirp == NULL)
    return -1;

  /* get images' paths */
  int maxPaths = 10, pathI = 0;
  char **imagePaths = (char **)malloc(sizeof(char *) * maxPaths);
  if (imagePaths == NULL)
    return -1;

  for (struct dirent *entry; (entry = readdir(dirp));) {
    const char *const imgName = entry->d_name;

    // save bitmap paths
    if (strstr(imgName, ".bmp")) {
      if (pathI >= maxPaths) {
        maxPaths *= 2;
        char **newImagePaths =
            (char **)realloc(imagePaths, sizeof(char *) * maxPaths);
        if (newImagePaths == NULL)
          return -1;
        imagePaths = newImagePaths;
      }

      imagePaths[pathI] =
          (char *)malloc(strlen(dirPath) + strlen("/") + strlen(imgName) + 1);
      if (imagePaths[pathI] == NULL)
        return -1;

      sprintf(imagePaths[pathI++], "%s/%s", dirPath, imgName);
    }
  }
  closedir(dirp);

  /* sort images */
  // Instead of qsort, function for sort on insert (get img num and use it as index maybe?)
  qsort(imagePaths, pathI, sizeof(char *), pathCompare);

  /* load sorted images */
  int max_images = 4;
  images->count = 0;
  images->imgs = (Imlib_Image *)malloc(sizeof(Imlib_Image) * max_images);
  if (images->imgs == NULL)
    return -1;

  for (int i = 0; i < pathI; ++i) {
    char *const imgPath = imagePaths[i];

    if (images->count + 1 > max_images) {
      max_images *= 2;
      Imlib_Image *new_images = (Imlib_Image *)realloc(
          images->imgs, sizeof(Imlib_Image) * max_images);
      if (new_images == NULL)
        return -1;
      images->imgs = new_images;
    }

    images->imgs[images->count] = imlib_load_image(imgPath);
    ++images->count;
    free(imgPath);
  }
  free(imagePaths);

  if (images->count == 0)
    die("loadImages: No images were loaded.");

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

int main(int argc, char *argv[]) {
  Args args;
  parseArgs(argc, argv, &args);

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  if (sigaction(SIGINT, &sa, NULL) == -1) // doesn't kill
    fprintf(stderr, "Failed setting SIGINT handler.\n");

  Video video;
  if (setupMonitors(&video) < 0)
    die("Failed setting up monitors.\n");

  if (args.isRandom) {
    srand(time(NULL));
    if (getRandomDir(&args)) // choose random directory
      die("Failed choosing a random directory.\n");
  }
  Images images;
  if (loadImages(&images, args.dirPath) < 0)
    die("Failed loading images.\n");

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

      /* imlib_context_set_anti_alias(1); */
      // draw all
      for (int i = 0; i < args.monitorCount * 4; i += 4) {
        imlib_render_image_on_drawable_at_size(
            args.monitorSettings[i], args.monitorSettings[i + 1],
            args.monitorSettings[i + 2], args.monitorSettings[i + 3]);
      }

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
