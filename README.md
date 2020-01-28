# OpenExrComposer
A simple and fast tool to compose sequences of EXR files.

## Usage:

To simply add two exr files and store it in a new output.exr, do:
> OpenExrComposer.exe "outputfile.exr = inputfileA.exr + inputfileB.exr"

The righthand side of the = may contain any math expression consisting of exr file paths, constants, parentheses and any number of + - * / operands. For example:
> OpenExrComposer.exe "beauty_pass.exr = (diffuse.exr * (lighting_raw.exr + gi_raw.exr)) + (reflection_raw.exr * reflection_filter.exr) + (refraction_raw.exr * refraction_filter.exr) + specular.exr + sss.exr + self_illum.exr + caustics.exr + background.exr + atmospheric_effects.exr"

You can also compose sequences by using # as whitecard character. Example:
> OpenExrComposer.exe "beauty_without_reflection_#.exr = beauty_#.exr - reflection#.exr - specular#.exr"
This will search the folders of the input files for all files matching the pattern (replacing # with any other string) and process all files.

You can also use constants instead of input files. Example:
> OpenExrComposer.exe "signed_normals.exr = (unsigned_normals.exr - 0.5) * 2.0"
