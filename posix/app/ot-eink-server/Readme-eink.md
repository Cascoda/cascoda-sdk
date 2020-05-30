# EInk Server Documentation #

`ot-eink-server` is very similar to `ot-server-standalone`. In order to use it,
you must have a Chili flashed with mac-dongle plugged into your POSIX
computer.  You can then run `bin/ot-eink-server`.

The application acts a lot like a simple file server: clients can send a GET
request to `ca/img` with the `id` query option set to the name of the requested
file. For instance: `GET ca/img?id=001.gz` would send the file titled `001.gz`
in the response.

If a child attempts to open a nonexistant file, the server ignores the GET
request and prints an error. However, if the client requests a file that is too
large to transmit in a single CoAP message (larger than 1024 bytes), the server
crashes since this is an unrecoverable error.

For the E-Ink application in particular, the client expects the files to be
GZipped 1-bit raw pixel data. Thankfully, this is easy enough to accomplish
using ImageMagick (looks like `convert` on most Unix systems, and ImageMagick
is significantly easier to Google than `convert`). You can install it with
`apt install imagemagick`, on Ubuntu and Debian. I have written several
helper scripts that accomplish common tasks.

## format.sh ##

This script can be used to convert a BMP image, such as the Cascoda logo, or
example supermarket labels, into the format that the children can decompress
and send to the displays. The script takes one argument, which is the base name
of your image (e.g. "cascoda" if your image is named "cascoda.bmp") and outputs
the desired file as "$BASENAME.gz" (e.g. outputs cascoda.gz).


```bash
#!/bin/sh

convert -rotate "-90" -depth 1 "$1.bmp" "$1.gray"
gzip -n --best "$1.gray"
mv "$1.gray.gz" "$1.gz"
```

## label.sh ##

This script can be used to generate an image containing arbitrary text, in the
format expected by the EInk client device. It accomplishes this by using the
`format.sh` script from above, and therefore it must live in the same folder.
The text is scaled to fit onto a single line, so long lines may prove difficult
to read.

The script expects one argument containing the text of the image, and generates
two files: "$TEXT.bmp" is a preview of what will be shown on the display, and
"$TEXT.gz" is the file that should be uploaded by the server. If the text you
want to display contains spaces, you must surround it in quotes: `./label.sh
"Text containing spaces"`.


```bash
#!/bin/sh

convert -size 296x128 -gravity south -depth 1 label:"$1" "$1.bmp"
./format.sh "$1"
```

## Demonstration Scripts ##

The demonstration works by changing the files provided by the server to each
Chili. There are essentially three "frames" in the "animation": the first frame
is completely blank, used at the start of the demo while the devices are
idling.  This frame is contained within the `clear.sh` script provided below.
It only uses one image: a completely blank image named `blank.gz`.

```bash
#!/bin/sh

cp -f blank.gz 001.gz
cp -f blank.gz 002.gz
cp -f blank.gz 003.gz
cp -f blank.gz 004.gz
cp -f blank.gz 005.gz
cp -f blank.gz 006.gz
cp -f blank.gz 007.gz
cp -f blank.gz 008.gz
```

The second frame consists of a sentence filled with IoT buzzwords generated
using the `label.sh` script. This frame can be found in the `casc.sh` script
below. This frame uses eight different images: `logo.gz` is the Cascoda logo,
and the rest of the images are words generated with `label.sh`.

```bash
#!/bin/sh

cp -f Cascoda.gz 001.gz
cp -f Thread.gz 002.gz
cp -f Scalable.gz 003.gz
cp -f E-Paper.gz 004.gz
cp -f IoT.gz 005.gz
cp -f Display.gz 006.gz
cp -f Demo.gz 007.gz
cp -f logo.gz 008.gz
```

The third frame consists of the Cascoda logo, a label saying "Supermarket
Display" and some example barcodes. Its script is titled `barcode.sh`.

```bash
#!/bin/sh

cp -f logo.gz 001.gz
cp -f "Supermarket Display.gz" 002.gz
cp -f Lanpo_a1.gz 003.gz
cp -f Lanpo_a2.gz 004.gz
cp -f Lanpo_a4.gz 005.gz
cp -f Lanpo_a1.gz 006.gz
cp -f Lanpo_a2.gz 007.gz
cp -f Lanpo_a4.gz 008.gz
```
