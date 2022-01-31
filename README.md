## Uniform Palette

Reduce color information (quantization) of a 24-bit RGB bitmap down to 8-bit palette-based bitmap.

### Compile

```
make
```

or 

```
makedos.bat	
```

for MS-DOS target.

No external dependencies required. It was tested on **macOS Monterey** (clang) and **MS-DOS** (DJGPP).

### Usage

```
./unipal input.bmp [dither]
```

Whereas:

* input.bmp: image to be quantized, must be 24-bit Windows bitmap.
* dither: enable dithering using ordered matrix

Output image will be stored as a 8-bit Windows bitmap.

### Preview
![https://github.com/dzutrinh/Uniform-Palette/blob/main/compares.png?raw=true](https://github.com/dzutrinh/Uniform-Palette/blob/main/compares.png?raw=true)
