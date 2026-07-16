"""
Color & Pixel Inspector
------------------------
A small desktop tool for reverse-engineering UI specs (colors + dimensions)
from a screenshot -- e.g. matching Windows File Explorer pixel-for-pixel.

Requirements:
    pip install pillow

Run:
    python color_pixel_inspector.py

Usage:
    - "Load Image..."     load a screenshot (PNG/JPG) from disk
    - "Capture Screen"     hides this window, waits N seconds, grabs the
                            whole screen, then reopens (so you can arrange
                            the real File Explorer window first)
    - Scroll wheel          zoom in/out, centered on the cursor
    - Middle-click drag     pan around
    - Mode: Pick Color      left-click drops a pin and records the exact
                            RGB/HEX of that pixel
    - Mode: Measure         click two points to measure the pixel distance
                            (dx, dy, straight-line) between them, both in
                            raw screenshot pixels and normalized by the
                            "Scale factor" box (e.g. set to 1.5 if your
                            display was at 150% scaling when you took the
                            screenshot, to get back to 96-DPI/logical px --
                            the same units your dpiScale-based constants use)
    - "Export Log..."       saves every sampled color + measurement to a
                            .txt file you can paste into your code/notes
    - "Clear"               clears all pins/measurements (keeps the image)

A live magnifier in the top-right always shows a zoomed-in grid of pixels
around your cursor with a crosshair, so you can land on the exact pixel
you want even when a color gradient (e.g. Mica tint) is involved.
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
from PIL import Image, ImageTk, ImageGrab
import math
import time
import os

MAGNIFIER_SIZE = 220     # canvas pixel size of the magnifier widget
MAGNIFIER_PIXELS = 11    # how many source pixels wide/tall it shows (odd number)


def rgb_to_hex(rgb):
    r, g, b = rgb[:3]
    return f"#{r:02X}{g:02X}{b:02X}"


class ColorPixelInspector(tk.Tk):
    def __init__(self):
        super().__init__()
        self.title("Color & Pixel Inspector")
        self.geometry("1280x820")
        self.configure(bg="#1e1e1e")

        self.image = None            # PIL.Image, full resolution, RGB
        self.zoom = 1.0
        self.min_zoom = 0.05
        self.max_zoom = 32.0

        self.mode = tk.StringVar(value="pick")   # "pick" or "measure"
        self.scale_factor = tk.DoubleVar(value=1.0)

        self.pins = []            # list of dicts: {x, y, rgb, hex}
        self.measurements = []    # list of dicts: {ax, ay, bx, by, dx, dy, dist}
        self.pending_measure_point = None

        self._pan_start = None
        self._canvas_image_id = None
        self._tk_photo = None

        self._build_ui()
        self._bind_events()

    # ---------------------------------------------------------- UI setup
    def _build_ui(self):
        style = ttk.Style(self)
        try:
            style.theme_use("clam")
        except Exception:
            pass

        toolbar = tk.Frame(self, bg="#252525")
        toolbar.pack(side="top", fill="x")

        def tbtn(text, cmd):
            b = tk.Button(toolbar, text=text, command=cmd, bg="#333333", fg="white",
                          activebackground="#444444", activeforeground="white",
                          relief="flat", padx=10, pady=4)
            b.pack(side="left", padx=4, pady=4)
            return b

        tbtn("Load Image...", self.load_image)
        tbtn("Capture Screen (3s)", lambda: self.capture_screen(3))
        tbtn("Zoom In", lambda: self.zoom_at_center(1.25))
        tbtn("Zoom Out", lambda: self.zoom_at_center(0.8))
        tbtn("Zoom 100%", self.zoom_reset)
        tbtn("Clear", self.clear_all)
        tbtn("Export Log...", self.export_log)

        mode_frame = tk.Frame(toolbar, bg="#252525")
        mode_frame.pack(side="left", padx=16)
        tk.Radiobutton(mode_frame, text="Pick Color", variable=self.mode, value="pick",
                       bg="#252525", fg="white", selectcolor="#333333",
                       activebackground="#252525", activeforeground="white").pack(side="left")
        tk.Radiobutton(mode_frame, text="Measure", variable=self.mode, value="measure",
                       bg="#252525", fg="white", selectcolor="#333333",
                       activebackground="#252525", activeforeground="white").pack(side="left")

        scale_frame = tk.Frame(toolbar, bg="#252525")
        scale_frame.pack(side="left", padx=16)
        tk.Label(scale_frame, text="Scale factor (DPI):", bg="#252525", fg="white").pack(side="left")
        tk.Entry(scale_frame, textvariable=self.scale_factor, width=6).pack(side="left", padx=4)
        tk.Label(scale_frame, text="(e.g. 1.5 for 150% display scaling)",
                bg="#252525", fg="#888888").pack(side="left")

        # Main body: canvas (left) + side panel (right)
        body = tk.Frame(self, bg="#1e1e1e")
        body.pack(fill="both", expand=True)

        canvas_frame = tk.Frame(body, bg="#1e1e1e")
        canvas_frame.pack(side="left", fill="both", expand=True)

        self.canvas = tk.Canvas(canvas_frame, bg="#111111", highlightthickness=0)
        self.canvas.pack(fill="both", expand=True)

        # Magnifier + status, overlaid top-right of the window body
        side = tk.Frame(body, bg="#252525", width=340)
        side.pack(side="right", fill="y")
        side.pack_propagate(False)

        tk.Label(side, text="Magnifier", bg="#252525", fg="white",
                font=("Segoe UI", 10, "bold")).pack(pady=(10, 2))
        self.mag_canvas = tk.Canvas(side, width=MAGNIFIER_SIZE, height=MAGNIFIER_SIZE,
                                    bg="#000000", highlightthickness=1,
                                    highlightbackground="#444444")
        self.mag_canvas.pack(pady=4)

        self.status_label = tk.Label(side, text="No image loaded.", bg="#252525", fg="#cccccc",
                                     justify="left", anchor="w", wraplength=320,
                                     font=("Consolas", 10))
        self.status_label.pack(fill="x", padx=10, pady=8)

        tk.Label(side, text="Sampled Colors", bg="#252525", fg="white",
                font=("Segoe UI", 10, "bold")).pack(pady=(10, 2))
        self.pins_listbox = tk.Listbox(side, bg="#1a1a1a", fg="white", height=10,
                                       font=("Consolas", 9), selectbackground="#3a6ea5")
        self.pins_listbox.pack(fill="x", padx=10)

        tk.Label(side, text="Measurements", bg="#252525", fg="white",
                font=("Segoe UI", 10, "bold")).pack(pady=(10, 2))
        self.meas_listbox = tk.Listbox(side, bg="#1a1a1a", fg="white", height=10,
                                       font=("Consolas", 9), selectbackground="#3a6ea5")
        self.meas_listbox.pack(fill="x", padx=10, pady=(0, 10))

    def _bind_events(self):
        self.canvas.bind("<Motion>", self.on_motion)
        self.canvas.bind("<Button-1>", self.on_click)
        self.canvas.bind("<ButtonPress-2>", self.on_pan_start)
        self.canvas.bind("<B2-Motion>", self.on_pan_move)
        self.canvas.bind("<MouseWheel>", self.on_mousewheel)     # Windows
        self.canvas.bind("<Button-4>", lambda e: self.on_mousewheel(e, delta=120))  # Linux
        self.canvas.bind("<Button-5>", lambda e: self.on_mousewheel(e, delta=-120))
        self.bind("<Escape>", lambda e: self.reset_measure_pending())

    # ---------------------------------------------------------- Image loading
    def load_image(self):
        path = filedialog.askopenfilename(
            filetypes=[("Images", "*.png *.jpg *.jpeg *.bmp *.gif"), ("All files", "*.*")]
        )
        if not path:
            return
        img = Image.open(path).convert("RGB")
        self._set_image(img)

    def capture_screen(self, delay_seconds=3):
        self.withdraw()
        self.update()

        def do_capture():
            img = ImageGrab.grab().convert("RGB")
            self._set_image(img)
            self.deiconify()

        # simple countdown using after() so the UI thread isn't blocked
        def countdown(n):
            if n <= 0:
                do_capture()
                return
            self.after(1000, lambda: countdown(n - 1))

        countdown(delay_seconds)

    def _set_image(self, img):
        self.image = img
        self.zoom = 1.0
        self.pins.clear()
        self.measurements.clear()
        self.pending_measure_point = None
        self._refresh_lists()
        self.after(50, self._fit_to_window)
        self.render()

    def _fit_to_window(self):
        if not self.image:
            return
        cw = max(self.canvas.winfo_width(), 100)
        ch = max(self.canvas.winfo_height(), 100)
        iw, ih = self.image.size
        self.zoom = max(self.min_zoom, min(cw / iw, ch / ih, 1.0))
        self.render()

    def zoom_reset(self):
        self.zoom = 1.0
        self.render()

    def zoom_at_center(self, factor):
        self.zoom = max(self.min_zoom, min(self.max_zoom, self.zoom * factor))
        self.render()

    def clear_all(self):
        self.pins.clear()
        self.measurements.clear()
        self.pending_measure_point = None
        self._refresh_lists()
        self.render()

    # ---------------------------------------------------------- Rendering
    def render(self):
        self.canvas.delete("all")
        if not self.image:
            self.canvas.create_text(20, 20, anchor="nw", fill="#888888",
                                    text="Load an image or capture the screen to begin.")
            return

        iw, ih = self.image.size
        dw, dh = max(1, int(iw * self.zoom)), max(1, int(ih * self.zoom))
        resample = Image.NEAREST if self.zoom >= 1 else Image.BILINEAR
        disp = self.image.resize((dw, dh), resample)
        self._tk_photo = ImageTk.PhotoImage(disp)
        self.canvas.config(scrollregion=(0, 0, dw, dh))
        self._canvas_image_id = self.canvas.create_image(0, 0, anchor="nw", image=self._tk_photo)

        # draw pins
        for i, p in enumerate(self.pins):
            cx, cy = p["x"] * self.zoom, p["y"] * self.zoom
            r = 5
            self.canvas.create_oval(cx - r, cy - r, cx + r, cy + r,
                                    outline="white", width=2)
            self.canvas.create_text(cx + 10, cy - 10, anchor="w", fill="white",
                                    text=f"{i+1}", font=("Segoe UI", 9, "bold"))

        # draw measurement lines
        for m in self.measurements:
            ax, ay = m["ax"] * self.zoom, m["ay"] * self.zoom
            bx, by = m["bx"] * self.zoom, m["by"] * self.zoom
            self.canvas.create_line(ax, ay, bx, by, fill="#ffcc00", width=2)
            self.canvas.create_oval(ax - 4, ay - 4, ax + 4, ay + 4, fill="#ffcc00", outline="")
            self.canvas.create_oval(bx - 4, by - 4, bx + 4, by + 4, fill="#ffcc00", outline="")
            mx, my = (ax + bx) / 2, (ay + by) / 2
            self.canvas.create_text(mx, my - 12, fill="#ffcc00",
                                    text=f'{m["dist"]:.1f}px', font=("Segoe UI", 9, "bold"))

        if self.pending_measure_point:
            px, py = self.pending_measure_point
            cx, cy = px * self.zoom, py * self.zoom
            self.canvas.create_oval(cx - 4, cy - 4, cx + 4, cy + 4, fill="#ffcc00", outline="")

    # ---------------------------------------------------------- Coordinate mapping
    def _canvas_to_image_coords(self, event):
        cx = self.canvas.canvasx(event.x)
        cy = self.canvas.canvasy(event.y)
        return cx / self.zoom, cy / self.zoom

    def _clamp_to_image(self, x, y):
        if not self.image:
            return None
        iw, ih = self.image.size
        if 0 <= x < iw and 0 <= y < ih:
            return int(x), int(y)
        return None

    # ---------------------------------------------------------- Mouse events
    def on_motion(self, event):
        if not self.image:
            return
        ix, iy = self._canvas_to_image_coords(event)
        pt = self._clamp_to_image(ix, iy)
        if pt is None:
            return
        x, y = pt
        rgb = self.image.getpixel((x, y))
        hexcode = rgb_to_hex(rgb)
        self.update_magnifier(x, y)

        scale = self._safe_scale()
        norm_x, norm_y = x / scale, y / scale
        self.status_label.config(
            text=(f"Cursor (raw px): {x}, {y}\n"
                  f"Cursor (÷{scale:g} scale): {norm_x:.1f}, {norm_y:.1f}\n"
                  f"RGB: {rgb}\nHEX: {hexcode}\n"
                  f"Zoom: {self.zoom*100:.0f}%")
        )

    def _safe_scale(self):
        try:
            s = float(self.scale_factor.get())
            return s if s > 0 else 1.0
        except Exception:
            return 1.0

    def on_click(self, event):
        if not self.image:
            return
        ix, iy = self._canvas_to_image_coords(event)
        pt = self._clamp_to_image(ix, iy)
        if pt is None:
            return
        x, y = pt

        if self.mode.get() == "pick":
            rgb = self.image.getpixel((x, y))
            self.pins.append({"x": x, "y": y, "rgb": rgb, "hex": rgb_to_hex(rgb)})
            self._refresh_lists()
            self.render()
        else:  # measure mode
            if self.pending_measure_point is None:
                self.pending_measure_point = (x, y)
                self.render()
            else:
                ax, ay = self.pending_measure_point
                dx, dy = x - ax, y - ay
                dist = math.hypot(dx, dy)
                self.measurements.append({"ax": ax, "ay": ay, "bx": x, "by": y,
                                          "dx": dx, "dy": dy, "dist": dist})
                self.pending_measure_point = None
                self._refresh_lists()
                self.render()

    def reset_measure_pending(self):
        self.pending_measure_point = None
        self.render()

    def on_pan_start(self, event):
        self._pan_start = (event.x, event.y)
        self.canvas.scan_mark(event.x, event.y)

    def on_pan_move(self, event):
        self.canvas.scan_dragto(event.x, event.y, gain=1)

    def on_mousewheel(self, event, delta=None):
        if not self.image:
            return
        d = delta if delta is not None else event.delta
        factor = 1.15 if d > 0 else (1 / 1.15)

        # zoom centered on cursor: keep the image point under the cursor fixed
        ix, iy = self._canvas_to_image_coords(event)
        old_zoom = self.zoom
        self.zoom = max(self.min_zoom, min(self.max_zoom, self.zoom * factor))
        self.render()

        new_cx, new_cy = ix * self.zoom, iy * self.zoom
        self.canvas.xview_moveto(0)
        self.canvas.yview_moveto(0)
        # shift view so the same image point stays under the cursor
        self.canvas.scan_mark(0, 0)
        target_x = new_cx - event.x
        target_y = new_cy - event.y
        self.canvas.xview_moveto(max(0, target_x) / max(1, int(self.image.size[0] * self.zoom)))
        self.canvas.yview_moveto(max(0, target_y) / max(1, int(self.image.size[1] * self.zoom)))

    # ---------------------------------------------------------- Magnifier
    def update_magnifier(self, x, y):
        self.mag_canvas.delete("all")
        if not self.image:
            return
        iw, ih = self.image.size
        half = MAGNIFIER_PIXELS // 2
        cell = MAGNIFIER_SIZE / MAGNIFIER_PIXELS

        for row in range(-half, half + 1):
            for col in range(-half, half + 1):
                sx, sy = x + col, y + row
                if 0 <= sx < iw and 0 <= sy < ih:
                    rgb = self.image.getpixel((sx, sy))
                else:
                    rgb = (30, 30, 30)
                x0 = (col + half) * cell
                y0 = (row + half) * cell
                self.mag_canvas.create_rectangle(
                    x0, y0, x0 + cell, y0 + cell,
                    fill=rgb_to_hex(rgb), outline="#000000"
                )

        # crosshair on the center pixel
        cx0 = half * cell
        cy0 = half * cell
        self.mag_canvas.create_rectangle(cx0, cy0, cx0 + cell, cy0 + cell,
                                         outline="#ff0000", width=2)

    # ---------------------------------------------------------- Lists / export
    def _refresh_lists(self):
        self.pins_listbox.delete(0, "end")
        for i, p in enumerate(self.pins):
            self.pins_listbox.insert(
                "end", f"{i+1}. ({p['x']},{p['y']})  {p['hex']}  rgb{p['rgb']}"
            )

        self.meas_listbox.delete(0, "end")
        scale = self._safe_scale()
        for i, m in enumerate(self.measurements):
            self.meas_listbox.insert(
                "end",
                f"{i+1}. dx={m['dx']}px dy={m['dy']}px dist={m['dist']:.1f}px "
                f"(÷{scale:g} -> {m['dx']/scale:.1f}, {m['dy']/scale:.1f}, {m['dist']/scale:.1f})"
            )

    def export_log(self):
        if not self.pins and not self.measurements:
            messagebox.showinfo("Nothing to export", "No sampled colors or measurements yet.")
            return
        path = filedialog.asksaveasfilename(
            defaultextension=".txt",
            filetypes=[("Text file", "*.txt")],
            initialfile="pixel_inspector_log.txt",
        )
        if not path:
            return

        scale = self._safe_scale()
        lines = [f"Color & Pixel Inspector export -- {time.strftime('%Y-%m-%d %H:%M:%S')}",
                f"Scale factor used for normalization: {scale}", ""]

        if self.pins:
            lines.append("=== Sampled Colors ===")
            for i, p in enumerate(self.pins):
                lines.append(
                    f"{i+1}. pixel=({p['x']},{p['y']})  hex={p['hex']}  rgb={p['rgb']}"
                )
            lines.append("")

        if self.measurements:
            lines.append("=== Measurements ===")
            for i, m in enumerate(self.measurements):
                lines.append(
                    f"{i+1}. A=({m['ax']},{m['ay']}) B=({m['bx']},{m['by']})  "
                    f"raw: dx={m['dx']}px dy={m['dy']}px dist={m['dist']:.2f}px  "
                    f"normalized(/{scale:g}): dx={m['dx']/scale:.2f} dy={m['dy']/scale:.2f} "
                    f"dist={m['dist']/scale:.2f}"
                )

        with open(path, "w", encoding="utf-8") as f:
            f.write("\n".join(lines))

        messagebox.showinfo("Exported", f"Saved to:\n{path}")


if __name__ == "__main__":
    app = ColorPixelInspector()
    app.mainloop()
