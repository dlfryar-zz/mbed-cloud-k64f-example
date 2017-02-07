# mbed-cloud-k64f-example

### requirements

- Python
- Python Virtualenv
- Python-PIP
- Python mbed-cli
- Brew on MacOS
- GCC-none compilers for ARM CortexM
- Imagemagick
- flash.py (optional to make flashing and resetting easier)

mbed target board FRDM-K64F with a .96 OLED Grove Display and mbed cloud client

<!---
mbed new mbed-cloud-k64f-example && cd mbed-cloud-k64f-example
-->

mbed add mbed-os

virtualenv .venv && source .venv/bin/activate

pip install -r mbed-os/requirements.txt

// Add the library for the OLED display

mbed add https://developer.mbed.org/users/danielashercohen/code/SeeedGrayOLED/

// Optionally get the firmware flashing tool

wget https://gist.githubusercontent.com/mbartling/359fe8df6fd785e8960d566fb3c3b479/raw/2d7fd3c131c3e33bfefe1fa12e1024987039b312/flash.py

// Add the library for the motion sensor part on the FRDM-K64F board

mbed add http://developer.mbed.org/teams/Freescale/code/FXOS8700Q

### Header File Generation

All the graphics below can be fetched with the create_h_bitmaps.sh script including header file generation

#### Download graphics for display
// Grab a logo from NXP that says mbed enabled - note it's not really 100x100 rather 530x630

wget http://www.nxp.com/files-static/graphic/logo_external/MBED_ENABLED_LOGO_100X100.jpg

// Grab the ARM logo from a public source

wget http://img.technews.co/wp-content/uploads/2015/05/ARM-logo.jpg

// Grab the IoT ARM image

https://developer.mbed.org/media/uploads/chris/qsg.png


#### BLANK CLEAR SCREEN GRAPHIC

// We need to generate a black bitmap with Imagemagick

convert -size 96x96 xc:#000000 blank.bmp

// Make the black bitmap monochrome with Imagemagick

convert blank.bmp -monochrome -colors 2 blank_96X96.bmp

// We need to strip the bitmap header off the file and get pure binary image data

cat blank_96X96.bmp | dd skip=146 bs=1 of=blank_96X96.img

// Dump a C header file with an array of hexadecimal image payload

xxd -i blank_96X96.img blank.h

#### ARM MBED ENABLED LOGO

// We need to convert the original image we downloaded with wget to a 96x96 to fit on the display
notice it's also filling the edges and flipping it so we fill the display it the orientation is correct

convert MBED_ENABLED_LOGO_100X100.jpg -resize 96x96 -background white -gravity center -extent 96x96 -flip MBED_ENABLED_LOGO_96X96.bmp

// Make the mbed enabled image monochrome with Imagemagick

convert MBED_ENABLED_LOGO_96X96.bmp -monochrome -colors 2 MBED_ENABLED_LOGO_96X96_LOGO.bmp

// We need to strip the bitmap header off the file and get pure binary image data

cat MBED_ENABLED_LOGO_96X96_LOGO.bmp | dd skip=146 bs=1 of=MBED_ENABLED_LOGO_96X96_LOGO.img

// Dump a C header file with an array of hexadecimal image payload

xxd -i MBED_ENABLED_LOGO_96X96_LOGO.img logo.h

#### ARM STARTUP LOGO

convert ARM-logo.jpg -resize 96x96 -background white -gravity center -extent 96x96 -flip ARM-logo_96X96.bmp

convert ARM-logo_96X96.bmp -monochrome -colors 2 ARM-logo_96X96-mono.bmp

cat ARM-logo_96X96-mono.bmp | dd skip=146 bs=1 of=ARM-logo_96X96-mono.img

xxd -i ARM-logo_96X96-mono.img armlogo.h

### BUILD and RUN

// Generate the header files from images

./create_h_bitmaps.sh

// Build the firmware with the GCC compilers

mbed compile -m K64F -t GCC_ARM

// Optional flash tool which copies the firmware binary to the board and sends a serial break
command to reset and start executing the new code.  Otherwise just drag over the compiled
binary to the DAPLINK connected USB storage device and reset the board

./flash.py