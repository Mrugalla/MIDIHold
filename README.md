# MIDIHold
OMG MIDIHold!! GET IT NOW!!! OUT NOW!!! SOON OUT OF STOCK!!!!!
![2016_06_25_midihold](https://github.com/Mrugalla/MIDIHold/assets/54960398/fae16fe1-0d68-4779-aba3-7cb62f746962)

It's a plugin that takes your (mostly monophonic) input MIDI and removes the pauses.
This results in the sensation that exactly one note is held forever (or until you stop playback).
This is useful for vocoders, that stop playing when there are pauses in the input MIDI.
I personally always find that a little annoying, so here's my solution! :)

The plugin doesn't require you to play perfectly. It can deal with up to 12 overlapping notes.
And if you enable MPE and send MIDI data of different channels into the plugin, it gives
each MIDI channel its own MIDIHold, which results in polyphonic MIDI.

You can also choose between the note priorities 'lowest' and 'highest' which matters for
MIDI inputs, in which multiple notes start at the same time (chords).

This is the stream in which I started working on it:
https://www.youtube.com/live/CLF6jKWC6xQ?feature=share

And this is the release video, in which I demonstrate its functionality and code:
https://youtu.be/aIVAfVaLEz8
