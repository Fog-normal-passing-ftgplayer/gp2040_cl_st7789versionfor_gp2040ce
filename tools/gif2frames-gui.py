#!/usr/bin/env python3
"""GUI GIF → RGB565 frame converter for GP2040-CE displays."""

import tkinter as tk
from tkinter import filedialog, messagebox, ttk
from PIL import Image, ImageTk
import os

PRESETS = {"240x135 (ST7789)": (240, 135), "160x80 (ST7735)": (160, 80), "135x240": (135, 240)}

class App:
    def __init__(self):
        self.root = tk.Tk()
        self.root.title("GIF → RGB565 Frame Converter")
        self.root.geometry("1100x700")
        self.root.configure(bg="#1a1a2e")

        self.gif = None
        self.frames = []
        self.selected = set()
        self.W, self.H = 240, 135
        self.delay_ms = 33
        self._preview_job = None
        self._preview_idx = 0

        self.build_ui()

    def build_ui(self):
        # Main layout: left (controls + frames), right (preview)
        main = tk.Frame(self.root, bg="#1a1a2e")
        main.pack(fill=tk.BOTH, expand=True, padx=10, pady=5)

        left = tk.Frame(main, bg="#1a1a2e", width=700)
        left.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        left.pack_propagate(False)

        right = tk.Frame(main, bg="#16213e", width=360)
        right.pack(side=tk.RIGHT, fill=tk.BOTH, padx=(10, 0))

        # === TOP BAR ===
        top = tk.Frame(left, bg="#1a1a2e"); top.pack(fill=tk.X, pady=2)

        tk.Button(top, text="Load GIF", command=self.load_gif, bg="#e94560", fg="white",
                  font=("", 12), padx=16).pack(side=tk.LEFT, padx=3)

        tk.Label(top, text=" Res:", bg="#1a1a2e", fg="#aaa").pack(side=tk.LEFT)
        self.res_var = tk.StringVar(value="240x135 (ST7789)")
        res_menu = ttk.Combobox(top, textvariable=self.res_var, values=list(PRESETS.keys()),
                                width=20, state="readonly")
        res_menu.pack(side=tk.LEFT, padx=2)
        res_menu.bind("<<ComboboxSelected>>", lambda e: self.update_res())

        self.custom_var = tk.StringVar()
        tk.Entry(top, textvariable=self.custom_var, width=8, bg="#0f3460", fg="#ddd",
                 insertbackground="#ddd").pack(side=tk.LEFT, padx=3)
        tk.Button(top, text="Set", command=self.update_res, bg="#0f3460", fg="#ddd",
                  padx=4).pack(side=tk.LEFT, padx=2)

        tk.Button(top, text="All", command=self.select_all, bg="#0f3460", fg="#ddd",
                  padx=4).pack(side=tk.RIGHT, padx=2)
        tk.Button(top, text="None", command=self.deselect_all, bg="#0f3460", fg="#ddd",
                  padx=4).pack(side=tk.RIGHT, padx=2)

        # === FRAME GRID ===
        self.frame_canvas = tk.Canvas(left, bg="#12121a", highlightthickness=0)
        self.frame_scroll = tk.Scrollbar(left, orient=tk.VERTICAL, command=self.frame_canvas.yview)
        self.frame_canvas.configure(yscrollcommand=self.frame_scroll.set)
        self.frame_inner = tk.Frame(self.frame_canvas, bg="#12121a")
        self.frame_canvas.create_window((0, 0), window=self.frame_inner, anchor="nw")
        self.frame_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.frame_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # === RIGHT PANEL: Preview + Speed ===
        tk.Label(right, text="Animation Preview", bg="#16213e", fg="#e94560",
                 font=("", 13, "bold")).pack(pady=(15, 5))

        self.preview_canvas = tk.Canvas(right, width=320, height=240, bg="#000",
                                        highlightthickness=1, highlightbackground="#333")
        self.preview_canvas.pack(pady=5)

        speed_frame = tk.Frame(right, bg="#16213e")
        speed_frame.pack(fill=tk.X, padx=15, pady=10)

        tk.Label(speed_frame, text="Speed:", bg="#16213e", fg="#aaa").pack(side=tk.LEFT)
        self.speed_var = tk.IntVar(value=30)
        self.speed_scale = tk.Scale(speed_frame, from_=1, to=60, orient=tk.HORIZONTAL,
                                    variable=self.speed_var, bg="#16213e", fg="#ddd",
                                    highlightbackground="#16213e", length=180,
                                    command=self._on_speed_change)
        self.speed_scale.pack(side=tk.LEFT, padx=5)

        self.speed_label = tk.Label(speed_frame, text="30 fps (33ms)", bg="#16213e", fg="#ddd")
        self.speed_label.pack(side=tk.LEFT, padx=5)

        btn_frame = tk.Frame(right, bg="#16213e")
        btn_frame.pack(fill=tk.X, padx=15, pady=5)

        self.play_btn = tk.Button(btn_frame, text="▶ Play Preview", command=self.toggle_preview,
                                  bg="#0f3460", fg="#ddd", font=("", 11))
        self.play_btn.pack(side=tk.LEFT, padx=3)

        tk.Button(btn_frame, text="Generate Header", command=self.generate, bg="#e94560",
                  fg="white", font=("", 12)).pack(side=tk.RIGHT, padx=3)

        # Status in right panel
        self.preview_status = tk.Label(right, text="", bg="#16213e", fg="#888", font=("", 9))
        self.preview_status.pack(pady=5)

        # === BOTTOM BAR ===
        bot = tk.Frame(self.root, bg="#1a1a2e"); bot.pack(fill=tk.X, padx=10, pady=5)
        tk.Button(bot, text="Copy to Clipboard", command=self.copy_out, bg="#0f3460",
                  fg="#ddd").pack(side=tk.LEFT, padx=5)
        tk.Button(bot, text="Save .h File", command=self.save_out, bg="#0f3460",
                  fg="#ddd").pack(side=tk.LEFT, padx=5)

        self.status = tk.Label(bot, text="Load a GIF to begin", bg="#1a1a2e", fg="#888")
        self.status.pack(side=tk.RIGHT, padx=10)

        self.output = tk.Text(self.root, height=6, bg="#0a0a1a", fg="#0f0",
                              font=("monospace", 10), insertbackground="#0f0")
        self.output.pack(fill=tk.X, padx=10, pady=5)

    def _on_speed_change(self, v):
        fps = int(v)
        self.delay_ms = max(16, 1000 // fps)
        self.speed_label.config(text=f"{fps} fps ({self.delay_ms}ms)")

    def update_res(self):
        custom = self.custom_var.get().strip()
        if custom and "x" in custom:
            try:
                w, h = map(int, custom.split("x"))
                self.W, self.H = w, h
            except: pass
        else:
            self.W, self.H = PRESETS.get(self.res_var.get(), (240, 135))
        self.render_frames()

    def load_gif(self):
        path = filedialog.askopenfilename(filetypes=[("GIF files", "*.gif")])
        if not path: return
        self.gif = Image.open(path)
        self.frames = []
        try:
            while True:
                self.gif.seek(len(self.frames))
                self.frames.append(self.gif.convert("RGB").copy())
        except EOFError: pass
        self.selected = set(range(min(len(self.frames), 5)))
        self.stop_preview()
        self.status.config(text=f"Loaded: {len(self.frames)} frames from {os.path.basename(path)}")
        self.render_frames()

    def _resize_frame(self, frame):
        fw, fh = frame.size
        scale = max(self.W / fw, self.H / fh)
        nw, nh = int(fw * scale), int(fh * scale)
        resized = frame.resize((nw, nh), Image.LANCZOS)
        left = (nw - self.W) // 2; top = (nh - self.H) // 2
        return resized.crop((left, top, left + self.W, top + self.H))

    def render_frames(self):
        for w in self.frame_inner.winfo_children(): w.destroy()
        if not self.frames: return

        thumb_w = 110
        row_frame = None
        for i, frame in enumerate(self.frames):
            if i % 5 == 0:
                row_frame = tk.Frame(self.frame_inner, bg="#12121a")
                row_frame.pack(fill=tk.X, pady=2)

            fw, fh = frame.size
            scale = thumb_w / fw
            th = int(fh * scale)
            thumb = frame.resize((thumb_w, th), Image.LANCZOS)
            photo = ImageTk.PhotoImage(thumb)

            fr = tk.Frame(row_frame, bg="#12121a")
            fr.pack(side=tk.LEFT, padx=3)

            border = "#0f0" if i in self.selected else "#333"
            lbl = tk.Label(fr, image=photo, bg=border, borderwidth=2, relief="solid",
                           cursor="hand2")
            lbl.image = photo
            lbl.pack()
            lbl.bind("<Button-1>", lambda e, idx=i: self.toggle_click(idx))

            chk_var = tk.BooleanVar(value=i in self.selected)
            chk = tk.Checkbutton(fr, text=f"#{i+1}", variable=chk_var, bg="#12121a", fg="#ddd",
                                 selectcolor="#12121a",
                                 command=lambda idx=i, var=chk_var: self.toggle(idx, var))
            chk.pack()

        self.frame_canvas.update_idletasks()
        self.frame_canvas.configure(scrollregion=self.frame_canvas.bbox("all"))

    def toggle(self, idx, var):
        if var.get(): self.selected.add(idx)
        else: self.selected.discard(idx)
        self.render_frames()

    def toggle_click(self, idx):
        if idx in self.selected: self.selected.discard(idx)
        else: self.selected.add(idx)
        self.render_frames()

    def select_all(self): self.selected = set(range(len(self.frames))); self.render_frames()
    def deselect_all(self): self.selected.clear(); self.render_frames()

    def toggle_preview(self):
        if self._preview_job:
            self.stop_preview()
        else:
            self.start_preview()

    def start_preview(self):
        indices = sorted(self.selected)
        if not indices:
            messagebox.showwarning("Error", "Select frames first!")
            return
        self._preview_frames = [self._resize_frame(self.frames[i]) for i in indices]
        self._preview_idx = 0
        self.play_btn.config(text="⏸ Stop")
        self._play_next()

    def stop_preview(self):
        if self._preview_job:
            self.root.after_cancel(self._preview_job)
            self._preview_job = None
        self._preview_frames = []
        self._preview_idx = 0
        self.play_btn.config(text="▶ Play Preview")
        self.preview_canvas.delete("all")
        self.preview_status.config(text="")

    def _play_next(self):
        if not self._preview_frames:
            self.stop_preview()
            return
        frame = self._preview_frames[self._preview_idx]
        # Scale preview to fit canvas
        pw, ph = 320, 240
        scale = min(pw / self.W, ph / self.H)
        dw, dh = int(self.W * scale), int(self.H * scale)
        ox, oy = (pw - dw) // 2, (ph - dh) // 2

        preview = frame.resize((dw, dh), Image.NEAREST)
        photo = ImageTk.PhotoImage(preview)
        self.preview_canvas.delete("all")
        self.preview_canvas.create_image(ox, oy, anchor=tk.NW, image=photo)
        self.preview_canvas.image = photo

        self.preview_idx_label = self._preview_idx
        self.preview_status.config(
            text=f"Frame {self._preview_idx + 1}/{len(self._preview_frames)}  "
                 f"{self.W}x{self.H}  {self.delay_ms}ms")

        self._preview_idx = (self._preview_idx + 1) % len(self._preview_frames)
        self._preview_job = self.root.after(self.delay_ms, self._play_next)

    def generate(self):
        if not self.frames: return messagebox.showwarning("Error", "Load GIF first!")
        indices = sorted(self.selected)
        if not indices: return messagebox.showwarning("Error", "Select at least 1 frame!")

        self.stop_preview()
        W, H = self.W, self.H
        lines = [f"// {len(indices)} frames of {len(self.frames)} at {W}x{H}  delay={self.delay_ms}ms"]
        lines.append(f"#define NYANCAT_FRAMES {len(indices)}")
        lines.append(f"#define NYANCAT_WIDTH {W}")
        lines.append(f"#define NYANCAT_HEIGHT {H}")
        lines.append(f"#define NYANCAT_DELAY_MS {self.delay_ms}")
        lines.append("#include <stdint.h>")
        lines.append("")

        for out_i, fi in enumerate(indices):
            cropped = self._resize_frame(self.frames[fi])
            px = cropped.load()
            row = f"static const uint16_t _nf{out_i}[{W*H}] = {{\n    "
            count = 0
            for y in range(H):
                for x in range(W):
                    r, g, b = px[x, y][:3]
                    rgb = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3)
                    row += f"0x{rgb:04X}"
                    if y < H - 1 or x < W - 1: row += ", "
                    count += 1
                    if count % 16 == 0: row += "\n    "
            row += "\n};"
            lines.append(row)
            lines.append("")

        ptrs = f"const uint16_t* nyancat_frames[{len(indices)}] = {{"
        ptrs += ", ".join(f"_nf{i}" for i in range(len(indices)))
        ptrs += "};"
        lines.append(ptrs)

        out = "\n".join(lines)
        self.output.delete("1.0", tk.END)
        self.output.insert("1.0", out)
        bin_sz = len(indices) * W * H * 2
        self.status.config(text=f"Generated: {len(indices)} frames, {W}x{H}, {bin_sz}B, {self.delay_ms}ms")

    def copy_out(self):
        out = self.output.get("1.0", tk.END).strip()
        if out: self.root.clipboard_clear(); self.root.clipboard_append(out); self.status.config(text="Copied!")

    def save_out(self):
        out = self.output.get("1.0", tk.END).strip()
        if not out: return
        path = filedialog.asksaveasfilename(defaultextension=".h", filetypes=[("Header files", "*.h")])
        if path:
            with open(path, "w") as f: f.write(out)
            self.status.config(text=f"Saved: {path}")

    def run(self): self.root.mainloop()

if __name__ == "__main__":
    App().run()
