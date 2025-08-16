import os, fcntl, mmap, struct

FBIOGET_VSCREENINFO = 0x4600  # ioctl request code

# Linux framebuffer var_screeninfo struct (simplified)
fmt = "I" * 39  # 39 unsigned ints

with open("/dev/fb0", "r+b") as f:
    # Get screen info
    vinfo = fcntl.ioctl(f, FBIOGET_VSCREENINFO, b"\0" * struct.calcsize(fmt))
    vals = struct.unpack(fmt, vinfo)
    xres, yres, bpp = vals[0], vals[1], vals[6]
    print(f"Resolution: {xres}x{yres}, {bpp} bpp")

    screensize = xres * yres * bpp // 8
    fb = mmap.mmap(f.fileno(), screensize, mmap.MAP_SHARED, mmap.PROT_WRITE|mmap.PROT_READ)

    # Draw a 100x100 red square at top-left
    for y in range(100):
        for x in range(100):
            offset = (y * xres + x) * (bpp // 8)
            fb[offset:offset+4] = b"\x00\x00\xff\x00"  # BGRA (blue=0, green=0, red=255, alpha=0)

    fb.close()
