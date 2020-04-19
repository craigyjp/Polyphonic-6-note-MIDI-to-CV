# Polyhonic-6-note-MIDI-to-CV
I needed a 6 note poly controller for a new hardware synth I was building and this is it.

It works, but as I'm using a teensy 3.5 and the orginal was based on a 5v Teensy 2.0 some work is required to set the scaling to 1v/Octave. Also the Pitchbend, CC, gates and triggers will all need some level conversion in hardware which I'm not going into in the thread, but they are currently 3.3v max so will need converting to +5 or +10v depending on preferences.

6 note polyphonic
6 velocity outputs
6 gate outputs
6 trigger outputs
Pitchbend and CC outputs
MIDI Channel selection or Omni
Poly/Unison/Mono modes

Still to do:-

Transpose function would be useful
