#!/bin/bash

python3 multi_slot_sign.py. \
\
flasher.bin \
ethos-u-alif_kws.bin \
ethos-u-alif_obj_detection.bin \
ethos-u-alif_kws.bin \
ethos-u-alif_obj_detection.bin \
\
--versions \
1.5.1 \
1.5.2 \
1.5.3 \
2.2.1 \
2.2.2 \
\
--comments \
"Bootloader application" \
"Application 1 example" \
"Application 2 example" \
"alif_kws application example" \
"obj_detection application example" \
\
--merged_output image_merged.bin
