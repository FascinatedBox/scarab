scarab
======

![Demo image](/resources/demo.gif)

### Overview

Scarab is a very basic **SC**reen g**rab** tool for Linux. This tool is designed
to be run from a script instead of invoked directly, so it doesn't have many
options.

Some screenshot tools work by taking a picture of the desktop, then selecting
the area where a target window is. If the target window has another window on
top of it, then the screenshot will have another window's content inside of it.

Scarab works by using xcb and png to get the content of a window. As a result,
the screenshot will never include another window's content. The catch is that
Scarab is not able to access a window's titlebar or borders.

### Usage

```
    -d, --display <dpy>            connect to <dpy> instead of $DISPLAY
    -o, --output <filename>        specify an output filename
                                   (default: screenshot.png)
    -w, --window <wid>             select window with id <wid>
    -h, --help                     display this help and exit
```

If no window is provided with `-w` / `--window`, scarab will switch the cursor
to a crosshair. A window can then be selected using the left mouse button, after
which the cursor will revert back to how it was. Alternatively, pressing any
other mouse button will cancel the selection.

### Why

**Example Project**: This project demonstrates how to take a screenshot using
just xcb and libpng. It's currently light on error checking, since I do not
expect it to fail.

**Streaming**: I use this to take screenshots of a game while I'm playing it. I
use it to take screenshots of bugs, fun moments, and maps.
