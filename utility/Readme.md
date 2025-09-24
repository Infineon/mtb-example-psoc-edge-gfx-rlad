# RLAD encoder tool

This tool helps to generate Run-length Adaptive Dithering (RLAD) (Graphics IP component) understandable format buffer from any given PNG file. PSOC&trade; Edge E84 MCU Graphics IP will be able decode this buffer and transfer original image to display.

The program takes a image(.png), compresses it in given format (RL / RLA / RLAD / RLAD_UNIFORM) and generates a header file(.h) corresponding to compressed image.

The excecutable takes three inputs:
1. input file name(png only):file to be compressed. eg, duck.png, selfie.png.
2. output file name(.h): name of a header file to be created. eg, duck.h, selfie.h
3. Mode of compression: RL / RLA / RLAD / RLAD_UNIFORM .

Example:
```
./rlad-encoder duck.png duck.h RLAD
```