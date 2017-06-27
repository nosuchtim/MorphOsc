# MorphOsc
Use the Sensel Morph to interact with OSC-capable software like Resolume and Plogue Bidule.

Usage: morphosc.exe [-l] [-v] {configfile}

The -l option will list the serial numbers of all connected Morphs.

The -v option will print messages showing the raw data from the Morph.

The {configfile} is a text file containing JSON that specifies what
OSC messages to send for the X, Y, and Z (force) values of the Morph.
See the morphosc*.json files in this directory for examples.
You can specify different controls for different Morphs using
their serial numbers (as in morphosc.json), or you can specify a
default set of controls for all Morphs by using "*" for the serial number.

In Resolume, you can use the Mapping feature (Edit Application OSC Map) to
see what OSC messages you should send to manipulate each control in Resolume.
These message are specified as the "osc" value in the configuration file.

The OSC messages are specified independently for each of the three dimensions -
"x", "y", and "force" .  Normally, the values sent are scaled from
0 to 1, but you can specify a "min" and "max" value in the configfile
to adjust the sent values to a particular range.  E.g. if for the "x" dimension
you specify "min" s 0.4 and "max" as 0.6, then when your finger is on the
left edge of the Morph, it will be sending 0.4, and when your finger is on
the right edge of the Morph, it will be sending 0.6.

You can specify a "maxforce" value in the configuration - any
force values greater than that value will be mapped to the "max" value
for that control.

The "host" value in the configuration specifies the port and hostname to which
the OSC messages will be sent.  For Resolume, the default port is 7000, and
you have to explicitly enable OSC Input in the OSC section of the
General Preferences in Resolume.

I haven't yet tested this utility with Plogue Bidule (which can also be
controlled with OSC), but it should probably work there as well.

One detail worth mentioning is that when your finger leaves the pad surface,
the "x" and "y" values will be whatever they were when you lifted your finger,
but the "force" value will always return to 0.0 when your finger leaves
the pad surface.

Any questions or suggestions for improvement, email to me@timthompson.com
