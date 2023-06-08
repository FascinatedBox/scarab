#include <getopt.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libpng16/png.h>

#include <xcb/xcb.h>
#include <xcb/xcb_image.h>

#include "xpick.h"

typedef enum {
    opt_display,
    opt_delay = 'd',
    opt_help = 'h',
    opt_output = 'o',
    opt_window = 'w',
} optlist_t;

struct option longopts[] = {
    { "delay", required_argument, NULL, opt_delay },
    { "display", required_argument, NULL, opt_display },
    { "output", required_argument, NULL, opt_output },
    { "window", required_argument, NULL, opt_window },
    { "help", no_argument, NULL, opt_help },
    { 0, 0, 0, 0 },
};

static const char *usage_text =
    "Usage: scarab [options]\n"
    "    --display <dpy>            connect to <dpy> instead of $DISPLAY\n"
    "-d, --delay <seconds>          wait <seconds> before taking shot\n"
    "-o, --output <filename>        specify an output filename\n"
    "                               (default: screenshot.png)\n"
    "-w, --window <wid>             select window with id <wid>\n"
    "-h, --help                     display this help and exit\n"
    ;


int s_delay = 0;
char *s_conn_name = NULL;
char *s_filename = NULL;
xcb_window_t s_window = XCB_WINDOW_NONE;
xcb_connection_t *s_conn = NULL;

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

    if (s_conn)
        xcb_disconnect(s_conn);

    exit(EXIT_FAILURE);
}

void parse_options(int argc, char **argv)
{
    int c, option_index = 0;

    while ((c = getopt_long_only(argc, argv, "d:ho:w:",
                                 longopts, &option_index)) != -1) {
        switch (c) {
            case opt_delay:
                s_delay = atoi(optarg);
                if (s_delay < 0)
                    fail("Invalid delay '%s'.", optarg);

                break;
            case opt_display:
                s_conn_name = optarg;
                break;
            case opt_output:
                s_filename = strdup(optarg);
                break;
            case opt_window:
                s_window = strtol(optarg, NULL, 0);

                if (s_window == 0)
                    fail("'%s' is not a window id for -w/--window.", optarg);

                break;
            case opt_help:
                usage();
                break;
            default:
                /* Getopt automatically reports the invalid option. */
                exit(EXIT_FAILURE);
        }
    }

    if (s_conn_name == NULL)
        s_conn_name = getenv("DISPLAY");

    s_conn = xcb_connect(s_conn_name, NULL);

    if (s_window == XCB_WINDOW_NONE) {
        xpick_state_t *s = xpick_state_new(s_conn);

        if (xpick_cursor_grab(s, 0) == 0)
            fail("No window provided and can't grab the cursor.");

        puts("scarab: Left click a window to take a screenshot of it.");
        xpick_cursor_pick_window(s);
        xpick_cursor_ungrab(s);
        s_window = xpick_window_get(s);
        xpick_state_free(s);

        if (s_window == XCB_WINDOW_NONE)
            fail("No window provided and window selection canceled.");
    }

    if (s_filename == NULL)
        s_filename = strdup("screenshot.png");
}

void write_png_for_ximage(xcb_image_t *image)
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
    int x, y;

    for (y = 0; y < image->height; y++) {
        png_byte *p_byte = p_row;

        for (x = 0; x < image->width; x++) {
            uint32_t pixel = xcb_image_get_pixel(image, x, y);

            *p_byte = pixel >> 16; p_byte++;
            *p_byte = pixel >> 8;  p_byte++;
            *p_byte = pixel;       p_byte++;
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

    xcb_get_geometry_cookie_t c = xcb_get_geometry(s_conn, s_window);
    xcb_get_geometry_reply_t *r = xcb_get_geometry_reply(s_conn, c, NULL);

    if (r == NULL)
        fail("Window 0x%.8x cannot be found on '%s'.", s_window, s_conn_name);

    if (s_delay)
        sleep(s_delay);

    xcb_image_t *image = xcb_image_get(s_conn,
            s_window,
            /* Okay, give me the entire window... */
            0,
            0,
            r->width,
            r->height,
            /* ...in the best quality you've got. */
            ~0,
            XCB_IMAGE_FORMAT_Z_PIXMAP);

    if (image == NULL)
        fail("Window 0x%.8x on '%s' does not have pixels to grab.", s_window,
             s_conn_name);

    write_png_for_ximage(image);
    xcb_image_destroy(image);
    free(r);
    free(s_filename);
    xcb_disconnect(s_conn);
    return EXIT_SUCCESS;
}
