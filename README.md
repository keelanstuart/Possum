# Playing Possum

*An extensible event player that currently happens to be a digital audio workstation.*

Possum is a... well, it could be a lot of things.

At its heart, Possum is an event player: an editor for placing commands on a timeline and letting plug-ins do something when the time comes. The machinery beneath it is called **Nucleus**, a general-purpose scheduling and plug-in framework.

Today, right now, **Possum is a digital audio workstation**. You can write music, synthesize sounds, arrange samples, shape effects, and render the results. Tomorrow? Maybe synchronized lasers. Maybe lighting. Maybe something nobody has thought of yet.

It's a work in progress.

## What can it do today?

- **Compose on a multitrack timeline.** Arrange and edit notes, samples, and commands in time.
- **Make sounds from scratch.** Use sine, square, triangle, and sawtooth oscillators, or several colors of noise.
- **Play samples.** Combine synthesized sounds and recorded audio in the same composition.
- **Shape the sound.** Edit spline-based envelopes for pitch, volume, panning, filtering, delay, reverb, and more, at either the track or global level.
- **Edit more than one thing at once.** Select, move, resize, duplicate, proportionally scale, and batch-edit the properties of multiple commands. Undo and redo are supported.
- **Listen while you work.** Audition notes, play or loop your arrangement, and export the result to audio (currently only .wav).

Possum is designed for experimentation.

## Possum and Nucleus

**Nucleus** provides the underlying model: tracks, timed commands, playback, devices, and a plug-in architecture. It doesn't need to know what a musical note is.

**Possum** provides the Windows editor: the timeline, property inspector, command list, automation tools, and the other conveniences that make those events pleasant to work with.

The separation is deliberate. The same scheduler that drives an audio synthesizer could, in principle, drive entirely different devices.

## Building

Possum is a native Windows C++ application built with **Visual Studio and MFC**. Nucleus and its plug-ins are part of the project.

Open the solution in Visual Studio and build the desired configuration. Some optional plug-ins have additional dependencies; for example, the Speech plug-in uses the Microsoft Cognitive Services Speech SDK through NuGet, so package restoration may be required before building it.

## Status

Possum is actively evolving. It's a personal project that has grown from a general-purpose event system into a surprisingly capable little music-making environment. Expect rough edges and occasional changes of direction.

The audio workstation is real. The broader possibilities are the dream.

*Not sleeping. Just Playing Possum.*
