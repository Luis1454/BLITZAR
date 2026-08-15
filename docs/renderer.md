# Desktop Renderer

The Qt desktop client uses an OpenGL 3.3 core point renderer by default. Projection, point sizing and the temperature/pressure color mapping run in the vertex and fragment shaders; the four synchronized views keep their own GPU vertex buffer so one view cannot stall another context.

The CPU QImage renderer remains available as a deterministic fallback. Force it when diagnosing a driver or when running on a machine without OpenGL 3.3:

```powershell
$env:BLITZAR_RENDERER = "cpu"
.\dist\gui-current\blitzar-gui.exe --config .\tests\data\scene_cosmology_preview.ini
```

Unset `BLITZAR_RENDERER`, or set it to `gpu`, to use the OpenGL path. If context creation or shader compilation fails, the viewport switches all four tiles to the CPU renderer instead of leaving an empty view.

The telemetry pipeline reports `renderer=opengl` or `renderer=qimage-cpu`, the displayed point count, the last frame submission time and the last vertex upload time. These are client-side timings; they are not simulation kernel timings.

The screen-level smoke test exercises the same four-tile assertion with both backends:

```powershell
$env:BLITZAR_RENDERER = "gpu"
python .\scripts\gui_smoke_windows.py --executable .\build-gui-check\blitzar-gui.exe --config .\tests\data\scene_cosmology_preview.ini --minimum-pixels 1 --foreground
$env:BLITZAR_RENDERER = "cpu"
python .\scripts\gui_smoke_windows.py --executable .\build-gui-check\blitzar-gui.exe --config .\tests\data\scene_cosmology_preview.ini --minimum-pixels 1 --foreground
```
