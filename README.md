# paqimg – Specialized Lossless Image Compressor

## About

**paqimg** is a specialized, high-ratio lossless image compressor derived from
[paq8px](https://encode.su/threads/342-paq8px) (PAQ compression family). Everything not related
to image compression has been removed: text models, x86/x64 executable models, audio models, the
LSTM neural network model, pre-training, multi-file archives, developer/test command line
options and more are gone. What remains is a lean, focused tool that only accepts images.

The whole program is a **single source file**, `paqimg.cpp` (an amalgamation in the style of
SQLite), plus the bundled `zlib` for PNG/deflate handling. There is nothing else to read,
patch or port.

Supported image formats:

| Format | Block type |
| --- | --- |
| BMP (uncompressed 1/4/8/24/32-bit) | IMAGE1 / IMAGE4 / IMAGE8 / IMAGE8GRAY / IMAGE24 / IMAGE32 |
| PBM, PGM, PPM, PAM (binary) | IMAGE1 / IMAGE8GRAY / IMAGE24 / IMAGE32 |
| JPEG (baseline, progressive, lossless) | JPEG |
| HDR / RGBE | HDR |
| GIF (87a, 89a, animated) | GIF |
| PNG (all bit depths, interlaced; also PNG8, PNG8GRAY, PNG24, PNG32 via internal re-encoding) | PNG8 / PNG8GRAY / PNG24 / PNG32 |
| TIFF (little-endian, uncompressed or LZW) | IMAGE1 / IMAGE4 / IMAGE8 / IMAGE8GRAY / IMAGE24 / LZW |
| TGA (uncompressed and RLE) | IMAGE8 / IMAGE8GRAY / IMAGE24 / IMAGE32 / RLE |
| Raw RGB (.rgb) | IMAGE8 |
| ZLIB / deflate streams inside supported containers | ZLIB |

Any other input is rejected with an error message.

## Usage

There are no compression levels to choose and no flags to remember. **The compression level is
selected automatically**: paqimg checks the installed RAM and picks the strongest of its 12
levels that still fits (level 1 uses ~583 MB, level 12 uses ~24.5 GB). The level used is stored
in the archive and printed on screen.

Compress an image:

```
paqimg image.png
```

Decompress:

```
paqimg -d image.png.paqimg2
```

An optional output name/path can be given as a second argument; otherwise the archive is created
next to the input with the `.paqimg2` extension. Run `paqimg -help` for the built-in help.

> **Note:** archives of earlier paqimg versions (`.paqimg1`) are not compatible and cannot be
> opened with this version.

## Build

### Any platform with CMake

```
mkdir build && cd build
cmake ..
cmake --build . --config Release
```

zlib is bundled and always built (needed for PNG/deflate handling). C++17 and a C++17-compliant
compiler are required.

## Repository layout

```
paqimg.cpp      the entire program (single translation unit)
CMakeLists.txt  build script
zlib/           bundled zlib
```

The single file is generated from the include graph rooted at `main()`; each section carries a
`// ===== path =====` marker.

## What was removed from paq8px

- All text models and dictionaries (text context model, word models, XML, indirect, record,
  sparse, chart, DMC forest, similarity models, English dictionaries)
- x86/x64 executable models and filters (EXE filter, x86 model, pre-training)
- Audio models and filters (WAV, AIFF, S3M, MOD, DEC alpha)
- Non-image container detection (PDF, ZIP, TAR, MRB, DBF, CD sector, BASE64/BASE85, endianness, EOL)
- Multi-file archive mode, file lists (`@FILELIST`), archive listing (`-l`)
- Brute-force DEFLATE detection, forced binary/text modes
- Model training (`-B`, `-E`, `-T` flags)
- The LSTM neural network model and all its options (`-L`, `-lstmonly`, `-lstmlayers`,
  `-loadlstm`, `-savelstm`)
- Developer/test command line options: `-t` (test mode), `-v` (verbose), `-log`, `-param=`,
  `-simd`, the RGB skip option and the whole TransformOptions machinery
- All compression level flags: the level is now chosen automatically from the installed RAM
- Dead code: legacy ContextMap v1, double-precision OLS variants, orphaned BMP/RLE filter
  classes, VLI and Clz helpers, Visual Studio project files
- Extra build scripts (only the CMake build remains)
- The 150+ file source tree itself: everything is merged into `paqimg.cpp`

## Credits

Based on [paq8px](https://encode.su/threads/342-paq8px) by Jan Ondrus, Márcio Pais and
Zoltán Gotthardt, which in turn is based on the PAQ family of compressors by Matt Mahoney
and contributors. PAQ is free software under the GPL, see
<http://www.gnu.org/licenses/gpl.txt>.
