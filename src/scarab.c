#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include <X11/Xutil.h>

#include <libpng16/png.h>

typedef enum {
    opt_display,
    opt_output,
    opt_window,
    opt_help,
} optlist_t;

struct option longopts[] = {
    { "display", required_argument, NULL, opt_display },
    { "output", required_argument, NULL, opt_output },
    { "window", required_argument, NULL, opt_window },
    { "help", no_argument, NULL, opt_help },
    { 0, 0, 0, 0 },
};

static const char *usage_text =
    "Usage: scarab [options]\n"
    "-d, --display <dpy>            connect to <dpy> instead of $DISPLAY\n"
    "-o, --output <filename>        specify an output filename\n"
    "                               (default: screenshot.png)\n"
    "-w, --window <wid>             select window with id <wid>\n"
    "-h, --help                     display this help and exit\n"
    ;

#define INVALID_WINDOW (~0)

char *s_dpy_name = NULL;
char *s_filename = NULL;
Window s_window = INVALID_WINDOW;
Display *s_dpy = NULL;

void usage()
{
    puts(usage_text);
    exit(EXIT_SUCCESS);
}

void fail(const char *msg, ...)
{
    va_list args;

    fputs("scarab: Error: ", stderr);
    va_start(args, msg);
    vfprintf(stderr, msg, args);
    va_end(args);
    fputc('\n', stderr);

    if (s_dpy)
        XCloseDisplay(s_dpy);

    exit(EXIT_FAILURE);
}

void parse_options(int argc, char **argv)
{
    int c, option_index = 0;

    while ((c = getopt_long_only(argc, argv, "d:ho:w:",
                                 longopts, &option_index)) != -1) {
        switch (c) {
            case 'd':
            case opt_display:
                s_dpy_name = strdup(optarg);
                break;
            case 'o':
            case opt_output:
                s_filename = strdup(optarg);
                break;
            case 'w':
            case opt_window:
                s_window = strtol(optarg, NULL, 0);
                if (s_window == 0)
                    fail("Invalid window id '%s'.", optarg);

                break;
            case 'h':
            case opt_help:
                usage();
                break;
            default:
                /* Getopt automatically reports the invalid option. */
                exit(EXIT_FAILURE);
        }
    }

    if (s_window == INVALID_WINDOW)
        fail("A window must be provided with '-w <wid>'.");

    if (s_dpy_name == NULL)
        s_dpy_name = getenv("DISPLAY");

    if (s_filename == NULL)
        s_filename = strdup("screenshot.png");
}

void write_png_for_ximage(XImage *image)
{
    FILE *f = fopen(s_filename, "wb");

    if (f == NULL)
        fail("Can't write to '%s'.", s_filename);

    png_structp p_struct = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL,
                                                   NULL, NULL);
    png_infop p_info = png_create_info_struct(p_struct);

    if (!p_struct || !p_info || setjmp(png_jmpbuf(p_struct))) {
        fclose(f);
        fail("Cannot initialize libpng.");
    }

    /* Output will be written to this file later. */
    png_init_io(p_struct, f);

    /* Set important parts: 8 bytes per channel, outputting RGB, and the size
       of the resulting image. */
    png_set_IHDR(p_struct, p_info, image->width, image->height, 8,
                 PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
                 PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
    png_write_info(p_struct, p_info);

    /* Each byte is R, G, B, so make space for 3 * width * byte. */
    png_bytep p_row = malloc(3 * image->width * sizeof(png_byte));
    unsigned long red_mask = image->red_mask;
    unsigned long green_mask = image->green_mask;
    unsigned long blue_mask = image->blue_mask;
    int x, y;

    for (y = 0; y < image->height; y++) {
        png_byte *p_byte = p_row;

        for (x = 0; x < image->width; x++) {
            unsigned long pixel = XGetPixel(image, x, y);

            *p_byte = (pixel & red_mask) >> 16;  p_byte++;
            *p_byte = (pixel & green_mask) >> 8; p_byte++;
            *p_byte = (pixel & blue_mask);       p_byte++;
        }

        png_write_row(p_struct, p_row);
    }

    free(p_row);
    png_write_end(p_struct, NULL);
    png_free_data(p_struct, p_info, PNG_FREE_ALL, -1);
    png_destroy_write_struct(&p_struct, NULL);
    free(p_info);
    fclose(f);
}

int main(int argc, char **argv)
{
    parse_options(argc, argv);
    s_dpy = XOpenDisplay(s_dpy_name);

    XWindowAttributes attr;

    XGetWindowAttributes(s_dpy, s_window, &attr);

    XImage *image = XGetImage(s_dpy, s_window, 0, 0, attr.width, attr.height,
                              AllPlanes, ZPixmap);

    write_png_for_ximage(image);
    XDestroyImage(image);
    XCloseDisplay(s_dpy);
    free(s_filename);
    return EXIT_SUCCESS;
}
