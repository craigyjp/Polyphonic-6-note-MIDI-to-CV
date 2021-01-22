# Polyhonic-6-note-MIDI-to-CV
I needed a 6 note poly controller for a new hardware synth I was building and this is it.

It works, but as I'm using a teensy 3.6 and the orginal was based on a 5v Teensy 2.0 so the DAC output is 3.3v max. Also the Pitchbend, CC, gates and triggers will all need some level conversion in hardware which I've covered in the schematic PDF. I've used matching 10k resistors on the DAC level converters to give 2x conversion and this gives 1v/octave, the triggers and gates are currently +5v.

6 note polyphonic

6 velocity outputs

6 gate outputs

6 trigger outputs

Pitchbend and CC outputs

MIDI Channel selection or Omni

Poly/Unison/Mono modes

Transpose Mode +/- one Octave

Octave shift 0 to -3 Octaves to give 0V on bottom C

Scaling on individual notes to improve tuning

Up/Down/Select buttons for menu

USB MIDI Host support as well as 5 pin DIN
