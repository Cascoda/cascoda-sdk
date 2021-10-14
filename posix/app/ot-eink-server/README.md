# EInk Server Documentation #

`ot-eink-server` is very similar to `ot-server-standalone`. In order to use it,
you must have a Chili flashed with mac-dongle plugged into your POSIX
computer.  You can then run `bin/ot-eink-server`.

The application acts a lot like a simple file server: clients can send a GET
request to `ca/img` with the `id` query option set to the name of the requested
file. For instance: `GET ca/img?id=001.gz` would send the file titled `001.gz`
in the response.

If a child attempts to open a nonexistant file, the server ignores the GET
request and prints an error. Same behaviour if the client requests a file that is too
large to transmit in a single CoAP message (larger than 1024 bytes).

For the E-Ink application in particular, the client expects the files to be
GZipped 1-bit raw pixel data. Thankfully, this is easy enough to accomplish
using ImageMagick (looks like `convert` on most Unix systems, and ImageMagick
is significantly easier to Google than `convert`). You can install it with
`apt install imagemagick`, on Ubuntu and Debian. There are several
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
`format.sh` script from above, and therefore it must exist in the same folder.
The text is scaled to fit onto a single line, so long lines may prove difficult
to read.

The script expects one argument containing the text of the image, and generates
two files: "$TEXT.bmp" is a preview of what will be shown on the display, and
"$TEXT.gz" is the file that should be uploaded by the server. If the text you
want to display contains spaces, you must surround it in quotes: 
``./label.sh "Text containing spaces"``.


```bash
#!/bin/sh

convert -size 296x128 -gravity south -depth 1 label:"$1" "$1.bmp"
./format.sh "$1"
```

## Application Demonstration ##
The application works with two commands which enable communication between the client and the server. On running the application, the client sends a discover request, to which the server responds back with a discovery response confirming the discovery request and the number of connected devices.

The client will send a GET request to the server for an image. It is up to the user to 'assign' an image to a connected device; until then, the server will respond back with a message stating that there is 'No Image available'. The client will wait a few seconds before it attempts to send another GET request. This will continue to take place until the user assigns an image to the device.

Assigning an image is straightforward; you will need the device's ID and a file with the extension `.gz`. The file must also be of a compatible size to fit the Eink display; ideally, the size should be `296x128`.

### List Command ###
A device is assigned an ID once the discovery has been handled by the server. In order to obtain a device's ID, you will have to use the `list` command. A user is expected to enter `list devices` to display the total number of connected devices, including their IDs, their associated Eui64, their IP address and finally, the image that has been assigned to them.

```
> list devices

Device 1: ID 0  Eui64: 1122334455667788, IP: [0123:4567:8901:2345:6789:0123:4567], Assigned Image: None.
```

### Assign Command ###
With the ID now accessible, the user may now use the `assign` command to assign a file with a `.gz` extension to a device with a specific ID. The file must exist in the same directory as the ot-eink-server executable. 

The command works as follows:
` assign [Device ID] [file.gz]`


Should the user enter an incorrect ID or file name, the server does not send anything and prompts the user to enter the command once again correctly.  

On a successful attempt, the server prints the following:

```
assign 0 001.gz
File Name: 001.gz, assigned to device with ID: 0.
> 2021-09-07 15:00:31 - server received GET Request from [0123:4567:8901:2345:6789:0123:4567]
2021-09-07 15:00:31 - Sent image titled "001.gz"

```
The next time the device sends a GET request, the server will respond back with the file assigned by the user and the Eink display will update with the new image.
