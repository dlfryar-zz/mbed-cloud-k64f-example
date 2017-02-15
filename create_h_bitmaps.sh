#!/bin/bash

IMAGE_DIR="IMAGES"
IMAGE_HEADER="images.h"

if [ ! -d "$DIRECTORY" ]; then
  mkdir $IMAGE_DIR
fi

if [ ! -z "$IMAGE_HEADER" ]; then
  rm -f $IMAGE_HEADER
  touch $IMAGE_HEADER
fi

inverse=false

for url in http://www.nxp.com/files-static/graphic/logo_external/MBED_ENABLED_LOGO_100X100.jpg \
           http://img.technews.co/wp-content/uploads/2015/05/ARM-logo.jpg \
           https://developer.mbed.org/media/uploads/chris/qsg.png \
           black_square
    do
        # we need a 96x96 black image to clear the display
        if [  "$url" = "black_square" ]; then
        filename_orig="$url"".bmp"
        convert -size 96x96 xc:#000000 "$IMAGE_DIR""/""$filename_orig"
        # otherwise we can just process every other image normally
        else
        wget --directory-prefix=$IMAGE_DIR $url
        filename_orig=$(echo $url | sed 's/.*\///')
        fi

        # echo $filename_orig
        filename_only=$(echo $filename_orig | sed 's/\.[^.]*$//')
        # echo $filename_only
        filename_one="$filename_only""-96X96.bmp"
        # echo $filename_one
        filename_two=$(echo $filename_one | sed 's/\.[^.]*$//')"-mono.bmp"
        # echo $filename_two
        # filename_three=$(echo $filename_two | sed 's/\.[^.]*$//')".img"
        # echo $filename_three
        filename_header=$(echo $filename_only)".h"
        # echo $filename_header
        
        convert "$IMAGE_DIR""/""$filename_orig" -resize 96x96 -background white -gravity center -extent 96x96 -flip "$IMAGE_DIR""/""$filename_one"
        convert "$IMAGE_DIR""/""$filename_one" -monochrome -colors 2 "$IMAGE_DIR""/""$filename_two"

        # Make the ARM letters white on a black background
        if [ "$filename_only" = "ARM-logo" ] || [ "$filename_only" = "MBED_ENABLED_LOGO_100X100" ]; then
          if [ "$inverse" = true ]; then
              cp $IMAGE_DIR"/"$filename_two $IMAGE_DIR"/"$filename_only"_dexter.bmp"
              mogrify +negate "$IMAGE_DIR""/""$filename_two"
          fi
        fi

        # We can skip the filename_three step and dd by using the -s option on xxd
        # cat "$IMAGE_DIR""/""$filename_two" | dd skip=146 bs=1 of="$IMAGE_DIR""/""$filename_three"
        # xxd -i "$IMAGE_DIR""/""$filename_three" $filename_header
        cd $IMAGE_DIR
        xxd -s 146 -i $filename_two >> "../"$IMAGE_HEADER && cd ..
    done

# Clean up all the images we created
rm -rf $IMAGE_DIR