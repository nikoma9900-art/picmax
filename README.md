# paqimg – Specialized Lossless Image Compressor

## About

**paqimg** is a specialized, high-ratio lossless image compressor derived from
[paq8px](https://encode.su/threads/342-paq8px) (PAQ compression family). Everything not related
to image compression has been removed: text models, x86/x64 executable models, audio models, the
LSTM neural network model, pre-training, multi-file archives, developer/test command line
options and more are gone. What remains is a lean, focused tool that only accepts images.

Supported image formats:

| Format | Block type |
| --- | --- |
| BMP (uncompressed 1/4/8/24/32-bit) | IMAGE1 / IMAGE4 / IMAGE8 / IMAGE8GRAY / IMAGE24 / IMAGE32 |
| PBM, PGM, PPM (ASCII) | IMAGE1 / IMAGE8GRAY / IMAGE24 |
| PGM RAW, PPM RAW | IMAGE8GRAY / IMAGE24 |
| JPEG (baseline, progressive, lossless) | JPEG |
| HDR / RGBE | HDR |
| GIF (87a, 89a, animated) | GIF |
| PNG (all bit depths, interlaced; also PNG8, PNG8GRAY, PNG24, PNG32 via internal re-encoding) | PNG8 / PNG8GRAY / PNG24 / PNG32 |
| TIFF (uncompressed, PackBits, LZW, Deflate) | IMAGE24 / IMAGE32 / ZLIB |

Any other input is rejected with an error message.

## Quick start

`paqimg` requires no installation.

Open a terminal and run `paqimg` with the desired options to compress an image:

```
paqimg -8 image.png
```

Decompress:

```
paqimg -d image.png.paqimg1
```

## Usage

```
Usage:
  to compress       ->   paqimg -LEVEL[FLAGS] INPUT [OUTPUT]
  to decompress     ->   paqimg -d INPUT.paqimg1 [OUTPUT]

LEVEL:
  -1 -2 -3 -4          | Compress using less memory
  -5 -6 -7 -8          | Use more memory
  -9 -10 -11 -12       | Use even more memory
  -0                   | Segment and transform only, no compression

FLAGS:
  S                    | Skip RGB color transform (images)
```

The archive gets the `.paqimg1` extension (`1` is the version). Run `paqimg -help` for detailed help.

## Compile

### Linux (cmake)

```
sudo apt install gcc build-essential cmake
cd paqimg/build
./build-linux-with-cmake.sh
```

zlib is bundled and always built (needed for PNG/deflate handling). C++17 and a C++17-compliant
compiler are required.

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
  `-simd`
- Dead code: legacy ContextMap v1, double-precision OLS variants, orphaned BMP/RLE filter
  classes, VLI and Clz helpers, Visual Studio project files
- Extra build scripts (only the CMake build remains)

## Credits

Based on [paq8px](https://encode.su/threads/342-paq8px) by Jan Ondrus, Márcio Pais and
Zoltán Gotthardt, which in turn is based on the PAQ family of compressors by Matt Mahoney
and contributors. PAQ is free software under the GPL, see
<http://www.gnu.org/licenses/gpl.txt>.
