## Color Reduction

Reduce color information (quantization) of a 24-bit RGB bitmap down to 8-bit palette-based bitmap.

### Compile

```
make
```

or 

```
makedos.bat		# MS-DOS target
```

No external dependencies required. It was tested on **macOS Monterey** (clang) and **MS-DOS** (DJGPP).

### Usage

```
./unipal input.bmp [dither]
```

Whereas:

* `input.bmp`: image to be quantized, must be a 24-bit Windows bitmap.
* `dither`: enable dithering using 4x4 ordered matrix

Output image will be stored as a 8-bit Windows bitmap under the name `output.bmp`.

### Preview

**Left**: Original; **Middle**: 8-bit undithered; **Right** 8-bit dithered.

![https://github.com/dzutrinh/Uniform-Palette/blob/main/compares.png?raw=true](https://github.com/dzutrinh/Uniform-Palette/blob/main/compares.png?raw=true)
