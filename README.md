# OpenExrComposer
A simple and fast tool to compose sequences of EXR files.

## Usage:

To simply add two exr files and store the result in a new output.exr, do:
> OpenExrComposer.exe "outputfile.exr = inputfileA.exr + inputfileB.exr"

The filenames can also contain fully qualified paths.

The righthand side of the = assignment may contain any math expression consisting of existing exr files, constants, parentheses and any number of + - * / operands. For example:
> OpenExrComposer.exe "beauty_pass.exr = (diffuse.exr * (lighting_raw.exr + gi_raw.exr)) + (reflection_raw.exr * reflection_filter.exr) + (refraction_raw.exr * refraction_filter.exr) + specular.exr + sss.exr + self_illum.exr + caustics.exr + background.exr + atmospheric_effects.exr"

You can also compose sequences by using # as wildcard character. Example:
> OpenExrComposer.exe "beauty_without_reflection_#.exr = beauty_#.exr - reflection#.exr - specular#.exr"

This will search the folders of the input files for all files matching the pattern (replacing # with any other string) and process all files.

When compositing sequences, it's also possible to have some input files be sequence files and others to remain the same. Example:
> OpenExrComposer.exe "watermarked_output_#.exr = inputSequence#.exr + watermarkFile.exr"

You can also use constants instead of input files. Examples:
> OpenExrComposer.exe "signed_normals.exr = (unsigned_normals.exr - 0.5) * 2.0"

> OpenExrComposer.exe "inverted_depth_#.exr = 1.0 - depth#.exr"

By default, the ouptut will be compressed using 16-scanline zlib compression. If you'd like to use another compression, you can specify it using the -c or --compression flag. Example (pay attention not to have the flag included in the expression surrounded by ""):
> OpenExrComposer.exe "output.exr = input.exr" --compression DWAB

## List of supported compressions:
- NO          : uncompressed output
- RLE         : run length encoding
- ZIP_SINGLE  : zlib compression, one scan line at a time
- **ZIP         : zlib compression, in blocks of 16 scan lines (the default)**
- PIZ         : piz-based wavelet compression
- PXR24       : lossy 24-bit float compression
- B44         : lossy 4-by-4 pixel block compression, fixed compression rate
- B44A        : lossy 4-by-4 pixel block compression, flat fields are compressed more
- DWAA        : lossy DCT based compression, in blocks of 32 scanlines. More efficient for partial buffer access
- **DWAB        : lossy DCT based compression, in blocks of 256 scanlines. More efficient space wise and faster to decode full frames than DWAA. (recommended for minimal file size)**

## Current limitations:
- Currently only works with RGB images.
- Does not yet work with deep exr.
- When using wildcard #, all matching input files must exist.
- Outputfile will be 32bit float.

# How To Build:
- Have Visual Studio 2017 or newer installed
- Have MSYS2 installed
- Run build.bat
- The output will be bazel-bin\openExrComposer.exe
