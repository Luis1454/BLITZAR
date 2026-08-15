"""Screen-level smoke test for the native Qt client on Windows."""

from __future__ import annotations

import argparse
import ctypes
import subprocess
import time
from ctypes import wintypes
from pathlib import Path
from tempfile import gettempdir

from PIL import ImageGrab


class Rect(ctypes.Structure):
    _fields_ = [("left", wintypes.LONG), ("top", wintypes.LONG),
                ("right", wintypes.LONG), ("bottom", wintypes.LONG)]


class Point(ctypes.Structure):
    _fields_ = [("x", wintypes.LONG), ("y", wintypes.LONG)]


USER32 = ctypes.windll.user32
ENUM_PROC = ctypes.WINFUNCTYPE(ctypes.c_bool, wintypes.HWND, wintypes.LPARAM)
USER32.GetWindowThreadProcessId.argtypes = [wintypes.HWND, ctypes.POINTER(wintypes.DWORD)]
USER32.GetWindowThreadProcessId.restype = wintypes.DWORD
SM_XVIRTUALSCREEN = 76
SM_YVIRTUALSCREEN = 77


def enable_per_monitor_dpi() -> None:
    try:
        ctypes.windll.shcore.SetProcessDpiAwareness(2)
    except (AttributeError, OSError):
        pass


def find_window(title: str | tuple[str, ...]) -> wintypes.HWND | None:
    titles = {title} if isinstance(title, str) else set(title)
    result: list[wintypes.HWND] = []

    def collect(hwnd: wintypes.HWND, _lparam: int) -> bool:
        if not USER32.IsWindowVisible(hwnd):
            return True
        length = USER32.GetWindowTextLengthW(hwnd)
        text = ctypes.create_unicode_buffer(length + 1)
        USER32.GetWindowTextW(hwnd, text, length + 1)
        if text.value in titles:
            result.append(hwnd)
            return False
        return True

    USER32.EnumWindows(ENUM_PROC(collect), 0)
    return result[0] if result else None


def client_bounds(hwnd: wintypes.HWND) -> tuple[int, int, int, int]:
    rect = Rect()
    USER32.GetClientRect(hwnd, ctypes.byref(rect))
    origin = Point(0, 0)
    USER32.ClientToScreen(hwnd, ctypes.byref(origin))
    return origin.x, origin.y, rect.right, rect.bottom


def image_bounds(image, bounds: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    """Translate screen coordinates into ImageGrab's virtual-screen coordinates."""
    virtual_left = USER32.GetSystemMetrics(SM_XVIRTUALSCREEN)
    virtual_top = USER32.GetSystemMetrics(SM_YVIRTUALSCREEN)
    left, top, width, height = bounds
    translated = (left - virtual_left, top - virtual_top, width, height)
    image_left, image_top = 0, 0
    image_right, image_bottom = image.size
    left = max(image_left, translated[0])
    top = max(image_top, translated[1])
    right = min(image_right, translated[0] + translated[2])
    bottom = min(image_bottom, translated[1] + translated[3])
    if right <= left or bottom <= top:
        raise RuntimeError(f"window bounds outside screenshot: bounds={bounds} image={image.size}")
    return left, top, right - left, bottom - top


def span(values: list[int]) -> tuple[int, int]:
    if not values:
        raise RuntimeError("no dark viewport region detected")
    return min(values), max(values)


def detect_viewport(image, bounds: tuple[int, int, int, int]) -> tuple[int, int, int, int]:
    left, top, width, height = bounds
    # Telemetry occupies the lower part of the client. Scan only the viewport
    # band so CPU and OpenGL backends use the same geometric assertion.
    scan_bottom = top + height * 3 // 5
    x_values: list[int] = []
    for x in range(left + width // 7, left + width - 20):
        dark = sum(
            max(image.getpixel((x, y))) < 45
            for y in range(top + height // 20, scan_bottom)
        )
        if dark > (scan_bottom - top) * 0.70:
            x_values.append(x)
    view_left, view_right = span(x_values)

    y_values: list[int] = []
    for y in range(top + height // 40, scan_bottom):
        dark = sum(
            max(image.getpixel((x, y))) < 45
            for x in range(view_left + 12, view_right - 12)
        )
        if dark > (view_right - view_left) * 0.70:
            y_values.append(y)
    view_top, view_bottom = span(y_values)
    return view_left, view_top, view_right + 1, view_bottom + 1


def particle_pixels(image, tile: tuple[int, int, int, int]) -> int:
    left, top, right, bottom = tile
    count = 0
    for y in range(top + 18, bottom - 18, 2):
        for x in range(left + 18, right - 18, 2):
            pixel = image.getpixel((x, y))
            if max(pixel) > 110 and max(pixel) - min(pixel) < 120:
                count += 1
    return count


def run(executable: Path, config: Path, minimum_pixels: int,
        screenshot_path: Path | None, foreground: bool) -> None:
    process = subprocess.Popen([str(executable), "--config", str(config)], cwd=executable.parent)
    try:
        deadline = time.monotonic() + 30.0
        hwnd = None
        while time.monotonic() < deadline:
            # The console-free launcher owns no GUI window; the Qt window is
            # created by its blitzar-client child process.
            hwnd = find_window(("N-Body Qt Client", "BLITZAR GUI"))
            if hwnd is not None:
                break
            if process.poll() is not None:
                raise RuntimeError(f"GUI launcher exited with code {process.returncode}")
            time.sleep(0.25)
        if hwnd is None:
            raise RuntimeError("native Qt window did not appear within 30 seconds")

        if foreground:
            USER32.ShowWindow(hwnd, 9)
            USER32.SetForegroundWindow(hwnd)
            time.sleep(1.0)
        # A million-particle case can need several seconds before the local server
        # finishes its initial allocation and starts accepting connections.
        time.sleep(18.0)
        image = ImageGrab.grab(all_screens=True).convert("RGB")
        if screenshot_path is not None:
            image.save(screenshot_path)
        bounds = client_bounds(hwnd)
        screenshot_bounds = image_bounds(image, bounds)
        viewport = detect_viewport(image, screenshot_bounds)
        middle_x = (viewport[0] + viewport[2]) // 2
        middle_y = (viewport[1] + viewport[3]) // 2
        tiles = [
            (viewport[0], viewport[1], middle_x, middle_y),
            (middle_x, viewport[1], viewport[2], middle_y),
            (viewport[0], middle_y, middle_x, viewport[3]),
            (middle_x, middle_y, viewport[2], viewport[3]),
        ]
        counts = [particle_pixels(image, tile) for tile in tiles]
        print(f"window_client={bounds}")
        print(f"screenshot={image.size} screenshot_client={screenshot_bounds}")
        print(f"viewport={viewport}")
        print("particle_pixels=" + ",".join(str(count) for count in counts))
        if any(count < minimum_pixels for count in counts):
            evidence = Path(gettempdir()) / "blitzar-gui-smoke-failure.png"
            image.save(evidence)
            raise AssertionError(f"one or more viewports are empty; evidence={evidence}")
    finally:
        subprocess.run(["taskkill", "/PID", str(process.pid), "/T", "/F"],
                       capture_output=True, check=False)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--executable", type=Path, required=True)
    parser.add_argument("--config", type=Path, required=True)
    parser.add_argument("--minimum-pixels", type=int, default=30)
    parser.add_argument("--screenshot", type=Path)
    parser.add_argument("--foreground", action="store_true",
                        help="Bring the GUI to the foreground for an interactive visual run")
    args = parser.parse_args()
    enable_per_monitor_dpi()
    run(args.executable.resolve(), args.config.resolve(), args.minimum_pixels,
        args.screenshot.resolve() if args.screenshot is not None else None, args.foreground)


if __name__ == "__main__":
    main()
