# ⋆⁺₊✧ asciiscope ✧₊⁺⋆

https://mit-license.org/

```rust
// project version 0.1.0
// Soundemote terminal-native signal visuals
// real console output > fake mockups

📺 console renderer : dark terminal phosphor, colored glyph trails
〰️ signal frames    : renderer-neutral visual input
🌀 demo signals     : soemdsp phasors, attractors, envelopes, noise
🌃 visual modes     : bloom, tunnel, particle field, spectral ribbon
🎛️ future input     : synths, plugins, sandbox probes, live signal buses
```

![example.gif](https://github.com/soundemote/asciiscope/blob/main/example.gif)


Windows Run command:

```text
cmd /k "cd /d C:\Users\argit\Desktop\asciiscope && build\Release\asciiscope.exe"
```

## live controls

The normal console app opens in circle calibration mode with an in-canvas
control panel. Press `h` or `?` to hide or show it; capture modes such as
`--reel`, `--tour`, and `--canvas-only` stay clean.

```markdown
1 2 3 4 5    lock bloom / tunnel / particle / spectral / circle mode
0            return to automatic mode rotation
Space        pause or resume
+ -          speed up or slow down
Up Down      speed up or slow down
[ ]          reduce or increase signal density
Left Right   reduce or increase signal density
Mouse wheel  deep zoom without clearing current trails
Left drag    pan the visual center without clearing trails
z Z          keyboard zoom out / in
f F          slower or faster circle trace frequency, up to 10 Hz
e E          lower or raise terminal cell ratio for round circle calibration
b B          dim or brighten the trace
v V          lower or raise the black trail floor
< >          shorter or longer trails
t T          lower or raise trail memory for longer phosphor fades
g            cycle glyph style
p            cycle color palette
c            toggle color
o            recenter view
r or x       clear trails
h or ?       show control help
q or Esc     quit
```

By default Asciiscope uses ANSI truecolor for smooth trail fades. Use
`--native-color` to compare the faster 16-color Windows console path.
Use `--color-ramp` to print a static palette fade diagnostic and exit.
Use `--no-help` to start with the in-canvas control panel hidden while keeping
the title and footer visible.
Use `--no-hint` with `--no-help` to hide the compact in-canvas control hint too.
Use `--clean` as a shorthand for framed captures with title/footer but no
in-canvas help or hint.
Use `--no-footer-help` when the footer should show signal/settings only.
Use `--cell-aspect` or live `e`/`E` controls to tune roundness for your current
font, terminal size, and capture crop.

## what is this?

Asciiscope is a dark colorful console instrument for signal art.

It turns DSP motion into terminal-native oscilloscope visuals: glowing ASCII
trails, waveform tunnels, chaotic attractor blooms, particle fields, envelope
flashes, and spectral-looking text graphics that can be recorded straight from
Windows Terminal.

This is the Soundemote attention layer:

```markdown
soemdsp          -> math, signal, dsp objects
asciiscope       -> terminal-native visual instrument
asciiscope-clap  -> future plugin window / synth input path
```

## visual identity

```rust
black screen
cyan voltage
violet decay
white-hot transients
blue low-energy ghosts
monospace motion
signal trails that feel like hardware waking up in the dark
```

The console is not a placeholder. The console is the look.

## renderer goal

Asciiscope is CPU-first on purpose.

The core visual path should be able to draw from signal samples directly, with
as much temporal detail as the signal source can provide. That keeps the audio
and visual behavior tightly coherent: a trace can be sampled, interpolated,
aged, and faded by the same timing model that produced the signal instead of
being forced into a separate video-frame rhythm too early.

GPU rendering is not rejected, but it is not the destination by itself. A future
GPU layer should amplify the CPU-built visual state with bloom, glow, feedback,
color grading, phosphor diffusion, capture surfaces, or Syphon/Spout-style
output experiments. The base instrument remains sample-aware and glyph-first.

## files

```markdown
### `include/asciiscope/SignalInput.hpp`
neutral visual input boundary
SignalFrame, SignalSource, SignalSample, SignalKind, ISignalInput

### `include/asciiscope/DemoSignalInput.hpp`
generated signal producer using soemdsp objects
currently feeds the standalone demo

### `include/asciiscope/AnimationScene.hpp`
visual scene modes
consumes SignalFrame data, not sandbox/runtime internals

### `include/asciiscope/ConsoleRenderer.hpp`
terminal framebuffer, glyph decay, ANSI fallback, Windows console buffer output

### `src/main.cpp`
demo loop, live controls, frame timing
```

## signal path

```text
DemoSignalInput
    -> SignalFrame
    -> AnimationScene
    -> ConsoleRenderer
    -> Windows Terminal / cmd
```

Future signal path:

```text
soemdsp-sandbox VisualizationBus
    -> SandboxSignalInput
    -> SignalFrame
    -> AnimationScene
    -> ConsoleRenderer
```

Asciiscope should stay independent. It should not become a sandbox client.
It should become a signal-frame visual instrument that a future adapter can feed.

## current modes

```rust
multi-sprott bloom
// chaotic attractor trace, rotating slowly through terminal space

phasor wave tunnel
// circular waveform tunnel, pulse and LFO modulated

pluck/noise particle field
// envelope flashes, noise drift, orbiting signal dust

spectral ribbon
// vertical signal energy bars with flowing phase motion

circle calibration trace
// one moving point traces a circle and leaves an ASCII trail behind it
```

## soemdsp objects currently used

```cpp
soemdsp::oscillator::Phasor
soemdsp::oscillator::MultiSprottC
soemdsp::oscillator::Thomas
soemdsp::utility::NoiseGenerator
soemdsp::modulator::LinearDASR
soemdsp::filter::LinearSmoother
soemdsp::math
```

## dependency

`soemdsp` lives at:

```text
libs/soemdsp
```

This checkout is configured for the local Soundemote source repo:

```powershell
git submodule update --init --recursive
```

If the submodule needs to be reattached:

```powershell
git submodule set-url libs/soemdsp C:/Users/argit/Desktop/soemdsp
git submodule update --init --recursive
```

## build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## run

```powershell
.\build\Release\asciiscope.exe
```

Options:

```powershell
.\build\Release\asciiscope.exe --help
.\build\Release\asciiscope.exe --list-presets
.\build\Release\asciiscope.exe --color-ramp
.\build\Release\asciiscope.exe --preset ghost-spectral --reel --describe
.\build\Release\asciiscope.exe --tour --seconds 16 --hold 2
.\build\Release\asciiscope.exe --tour --tour-seconds 3 --seconds 12
.\build\Release\asciiscope.exe --no-color
.\build\Release\asciiscope.exe --once
.\build\Release\asciiscope.exe --mode circle
.\build\Release\asciiscope.exe --mode circle --circle-hz 0.025
.\build\Release\asciiscope.exe --mode circle --zoom 3 --center-x -0.4 --center-y 0.2
.\build\Release\asciiscope.exe --mode circle --cell-aspect 1.8 --describe
.\build\Release\asciiscope.exe --preset circle-slow --clean
.\build\Release\asciiscope.exe --preset circle-slow --clean --no-footer-help
.\build\Release\asciiscope.exe --preset circle-slow --clean --trail-age 96
.\build\Release\asciiscope.exe --mode spectral
.\build\Release\asciiscope.exe --mode spectral --frames 240
.\build\Release\asciiscope.exe --mode spectral --frames 240 --fps 30
.\build\Release\asciiscope.exe --mode spectral --seconds 8 --fps 30
.\build\Release\asciiscope.exe --mode spectral --frames 240 --warmup 90 --fps 30
.\build\Release\asciiscope.exe --mode spectral --seed 0xA5C115C0
.\build\Release\asciiscope.exe --mode tunnel --width 96 --height 54
.\build\Release\asciiscope.exe --mode particles --speed 1.4 --density 1.3 --zoom 1.2 --trail 3
.\build\Release\asciiscope.exe --mode tunnel --glyphs dense
.\build\Release\asciiscope.exe --mode particles --palette ember
.\build\Release\asciiscope.exe --preset ghost-spectral --canvas-only --fps 30
.\build\Release\asciiscope.exe --preset particle-storm --reel
.\build\Release\asciiscope.exe --preset neon-tunnel
.\build\Release\asciiscope.exe --preset circle-calibration
.\build\Release\asciiscope.exe --preset circle-calibration --clean
.\build\Release\asciiscope.exe --preset particle-storm --zoom 1.6
.\build\Release\asciiscope.exe --preset ghost-spectral --no-hud --title ASCIISCOPE
```

The footer shows the current mode, speed, density, zoom, center offset, trail
amount, signal RMS/peak/min/max, glyph style, input backend, pause state, and
last adjusted control.

`--trail` controls how much the framebuffer fades each frame. `--trail-age`
controls the maximum trail memory/intensity, which is useful for slow circle
captures that should leave a longer phosphor path.

`--seed` accepts decimal or `0x` hex values for repeatable demo takes.

`--seconds` converts clip duration into a visible frame count using the selected
`--fps`. If both `--seconds` and `--frames` are present, `--frames` wins.

`--warmup` draws hidden frames before the first visible frame, so trails and
envelopes are already alive when capture starts.

`--hold` keeps the final frame visible before the terminal is restored. This is
useful when stopping a recording or checking the last frame of a bounded take.

`--canvas-only` hides the title, border, and footer for clean OBS crops.

`--reel` applies quick social-capture defaults: canvas-only, 30 FPS, 8 visible
seconds, and 90 warmup frames. You can still override it with `--fps`,
`--seconds`, `--frames`, or `--warmup`.

`--tour` cycles bloom, tunnel, particles, spectral, and circle looks in one
canvas-only capture. Use `--tour-seconds` to choose how long each look stays on
screen.

Glyph styles:

```markdown
classic
dense
blocks
wire
```

Color palettes:

```markdown
neon
ember
acid
ice
```

Presets:

```markdown
bloom-reel      classic glyphs, neon palette, attractor bloom
neon-tunnel     dense glyphs, neon palette, wide tunnel
particle-storm  block glyphs, ember palette, high density particles
ghost-spectral  wire glyphs, ice palette, wide spectral ribbon
circle-cal      classic glyphs, neon palette, circle calibration trace
circle-slow     classic glyphs, neon palette, slow circle trace
```

## future

CLAP/JUCE handoff note:

```markdown
docs/CLAP_STARTINGPOINT_HANDOFF.md
```

```rust
AudioSignalInput
// live synth or audio interface input

SandboxSignalInput
// consumes future soemdsp-sandbox visualization/probe frames

PluginRenderer
// asciiscope-clap draws the console look inside a plugin editor

SyphonOutput
// macOS realtime frame sharing for VJ/video tools
// https://syphon.info/

SpoutOutput
// Windows realtime video routing for VJ/video tools
// https://spout.zeal.co/

WaveformScope
// honest oscilloscope mode for real synth signals

SpectralConsole
// dark colorful pseudo-spectrum made from real audio features
```

## philosophy

```markdown
* real executable output
* terminal visuals first
* visible motion over architecture theater
* dark console, bright signal
* generated demos now, live synths next
* keep the visual core independent
* terminal first, video-pipeline ready
```
