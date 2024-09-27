# BMtoBMP
A header-only library for converting BM image files to BMP format using PAL files.

## Installing

Navigate to the releases page and download the latest zip file.

Once downloaded, extract the zip file wherever you'd like and you're done!

See [usage](#Usage) below for more details on running the application.

### Building from Source (Linux/macOS/Unix)

Install dependencies:

```
gcc make cmake mingw32-gcc clang-tools-extra
```

Clone the project:
```bash
$ git clone https://github.com/IcePanorama/BMtoBMP.git
# ... or, alternatively ...
$ git clone git@github.com:IcePanorama/BMtoBMP.git
```

Install:
```bash
$ cd BMtoBMP && mkdir build && cd build && cmake .. && make release && cd ..
```

## Usage
```
BMtoBMP_x86_64.exe path\to\file.BM path\to\file.PAL # 64-bit Windows Systems
BMtoBMP_i686.exe path\to\file.BM path\to\file.PAL # 32-bit Windows Systems
./BMtoBMP path/to/file.BM path/to/file.PAL # Linux/macOS/Unix Systems
```

## Usage as a library

Debug error output messages can be enabled by compiling with the `-DBMtoBMP_DEBUG_OUTPUT` flag.

### Overview:
This library provides functionality to convert BM image files to standard 24-bit BMP images, using an accompanying PAL file to map pixel values to RGB colors.

This library only exposes one public function for use: `BMtoBMP_convert_image()`.

### Function:

#### `BMtoBMP_convert_image(FILE *bm_file, FILE *pal_file, const char *output_filename)`

The primary function for converting a BM image file to BMP format.

**Input:**
* `bm_file`: A pointer to an open BM file (`FILE *`), which contains the image's pixel data, where each byte is an index into the palette, (`pal_file`).
* `pal_file`: A pointer to an open PAL file (`FILE *`), used to map the indices in the BM file to RGB color values.
* `output_filename`: A null-terminated C string representing the desired output filename (without extension). The filename must not exceed 250 characters. This function automatically appends the ".bmp" extension to the provided name.

**Returns:**
* A zero on success, non-zero on failure.

 **Responsibilities:**
* The caller is responsible for opening and closing both input files (`bm_file` and `pal_file`).
* It is also the caller's responsibility to ensure that the files provided are valid BM/PAL files, as the library does not perform any validation.
