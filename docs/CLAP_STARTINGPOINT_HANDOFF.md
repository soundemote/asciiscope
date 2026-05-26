# Asciiscope CLAP Startingpoint Handoff

Asciiscope is ready for the next repo step when the goal is live synth input or
a plugin window. The terminal app should stay independent; the plugin project
should reuse the signal-frame idea rather than pulling console assumptions into
the audio/plugin layer.

## Current Boundary

```text
ISignalInput
    -> SignalFrame
    -> AnimationScene
    -> ConsoleRenderer
```

The reusable idea is the middle of that chain:

```text
audio/plugin input
    -> SignalFrame
    -> visual scene / renderer
```

The CLAP project should produce frames from plugin audio/control data. Scenes
should keep asking for named signal sources, latest samples, stats, and timing.
They should not know about CLAP, JUCE, plugin processors, ports, buses, or host
callbacks.

## Renderer Direction

Keep the base renderer CPU-first and sample-aware.

Asciiscope's advantage is that trace generation, interpolation, trail aging, and
glyph placement can stay close to the signal timeline. That gives the visual
system room to draw from audio/control samples at much higher temporal density
than a normal video frame loop, limited mainly by the source sample rate and the
chosen visual budget.

GPU work can still happen later, but it should sit after the CPU visual state:

```text
signal samples
    -> CPU visual state / glyph field / trace memory
    -> optional GPU glow, bloom, feedback, capture, Syphon/Spout output
```

Do not make OpenGL the implied migration target for the core renderer. Use it
only when it adds effects or output plumbing that the CPU/glyph path already
defined.

## First Plugin Shape

```text
asciiscope-clap
    PluginProcessor
        receives audio/control blocks
        updates a lock-safe visualization buffer

    PluginEditor
        reads latest visualization frame
        draws the console look

    ClapSignalInput
        maps plugin audio/control data to SignalFrame
```

The first version can be minimal:

```text
stereo audio in
gain/scale control
freeze control
mode control
palette control
one console-style editor
```

## What To Reuse From Asciiscope

```text
SignalKind
SignalSample
SignalStats
SignalSource
SignalFrame
scene math and visual modes, after separating them from ConsoleRenderer
preset names and visual identity
```

## What Not To Reuse Directly

```text
terminal input polling
Windows console buffer output
command-line capture flags
thread sleeps as timing
stdout rendering
```

## Likely Refactor Before Or During Plugin Work

`AnimationScene` currently draws directly into `ConsoleRenderer`. For the CLAP
repo, the useful extraction is a renderer-neutral drawing sink:

```cpp
class IVisualSurface {
  public:
    virtual void beginFrame() = 0;
    virtual void fade(int amount) = 0;
    virtual void plot(double x, double y, double intensity) = 0;
    virtual int width() const = 0;
    virtual int height() const = 0;
};
```

Then:

```text
ConsoleRenderer implements IVisualSurface
PluginRenderer implements IVisualSurface
AnimationScene draws to IVisualSurface
```

That lets the terminal app keep its look while the CLAP editor gets a native
renderer later.

## First Startingpoint Pass

1. Inspect `baconpaul/sidequest-startingpoint`.
2. Keep its JUCE/CLAP/CMake structure intact.
3. Rename only what the template expects to be renamed.
4. Add MIT license metadata.
5. Add a tiny signal visual prototype in the editor.
6. Avoid pulling `soemdsp-sandbox` internals.
7. Decide whether Asciiscope code becomes a submodule, copied small shared
   headers, or a later shared library after the template is understood.

## Ready Criteria

Asciiscope is ready to hand off when:

```text
terminal demo captures are easy to run
SignalFrame boundary exists
scene code is isolated enough to extract
plugin plan does not require sandbox internals
startingpoint can be inspected without editing this repo
```

That is the current state.
