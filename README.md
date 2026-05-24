# asciiscope

Soundemote terminal visuals driven by real `soemdsp` signal objects.

Asciiscope is the visual/attention layer for quick social-ready console clips:
oscilloscope traces, waveform tunnels, oscillator motion, chaotic attractor
fields, pluck-envelope flashes, and spectral-looking ASCII output that can be
recorded directly from Windows Terminal.

![example.gif](https://github.com/soundemote/asciiscope/blob/main/example.gif)

## Dependency

`soemdsp` is included as a git submodule at `libs/soemdsp`.

This checkout is configured to use the local source repo:

```powershell
git submodule update --init --recursive
```

If the submodule needs to be reattached manually:

```powershell
git submodule set-url libs/soemdsp C:/Users/argit/Desktop/soemdsp
git submodule update --init --recursive
```

## Build

```powershell
cmake -S . -B build
cmake --build build --config Release
```

## Run

```powershell
.\build\Release\asciiscope.exe
```

Options:

```powershell
.\build\Release\asciiscope.exe --no-color
.\build\Release\asciiscope.exe --once
```

Live controls:

- `1`, `2`, `3`: lock to bloom, tunnel, or particle mode
- `0`: return to automatic mode rotation
- `Space`: pause or resume
- `+` / `-` or up/down arrows: speed up or slow down
- `[` / `]` or left/right arrows: reduce or increase signal density
- Mouse wheel: zoom in or out without clearing the current trails
- `z` / `Z`: keyboard zoom out or in
- `<` / `>`: shorter or longer trails
- `c`: toggle color
- `r` or `x`: clear trails
- `h` or `?`: show control help
- `q` or `Esc`: quit

The default demo runs hands-free and rotates through:

- `multi-sprott bloom`
- `phasor wave tunnel`
- `pluck/noise particle field`

The demo signal input currently uses `soemdsp::oscillator::Phasor`,
`soemdsp::oscillator::MultiSprottC`, `soemdsp::oscillator::Thomas`,
`soemdsp::utility::NoiseGenerator`, `soemdsp::modulator::LinearDASR`,
`soemdsp::filter::LinearSmoother`, and `soemdsp::math` wavetable helpers.

Internally, scenes consume neutral `SignalFrame` / `SignalSource` data from an
`ISignalInput`. The current producer is `DemoSignalInput`; a future
`SandboxSignalInput` can feed the same scenes from a soemdsp-sandbox
visualization bus without making the terminal app depend on sandbox internals.
