# MorphOsc
Use the Sensel Morph to interact with OSC-capable software like Resolume and Plogue Bidule.

Usage: morphosc.exe [-v] -c {configfile}

The configfile is JSON which specifies what OSC messages to send
for the X, Y, and Z (pressure) of the Morph.  See the morphosc*.json files
in this directory for examples.

In Resolume, you can use the Mapping feature (Edit Application OSC Map) to
see what OSC messages you should send as OSC input to manipulate
each control in Resolume.

You specify the OSC messages independently for each of the three dimensions -
"x", "y", and "z" (pressure).  Normally, the values sent are scaled from
0 to 1, but you can specify a "min" and "max" value in the configfile
to adjust the sent values to a particular range.  E.g. if for the "x" dimension
you specify "min" s 0.4 and "max" as 0.6, then when your finger is on the
left edge of the Morph, it will be sending 0.4, and when your finger is on
the right edge of the Morph, it will be sending 0.6.

Any questions, email me@timthompson.com
