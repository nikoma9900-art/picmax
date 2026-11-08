ï»¿#pragma once

#include "../Array.hpp"
#include "../BlockType.hpp"
#include "../Encoder.hpp"
#include "../TransformOptions.hpp"
#include "../file/File.hpp"
#include "../file/FileDisk.hpp"
#include "../file/FileTmp.hpp"
#include "../Utils.hpp"
#include "Filter.hpp"
#include "TextParserStateInfo.hpp"
#include "gif.hpp"
#include "lzw.hpp"
#include "png.hpp"
#include "zlib.hpp"
#include <cctype>
#include <cstdint>
#include <cstring>

/////////////////////////// Filters /////////////////////////////////
//@todo: Update this documentation
//
// Before compression, data is encoded in blocks with the following format:
//
//   <type> <size> <encoded-data>
//
// type is 1 byte (type BlockType): DEFAULT=0, JPEG, HDR, IMAGE1..IMAGE32, ZLIB, GIF, PNG*, RLE, LZW
// size is 4 bytes in big-endian format.
// Encoded-data decodes to <size> bytes.  The encoded size might be
// different.  Encoded data is designed to be more compressible.
//
//   void encode(File *in, File *out, int n);
//
// Reads n bytes of in (open in "rb" mode) and encodes one or
// more blocks to temporary file out (open in "wb+" mode).
// The file pointer of in is advanced n bytes.  The file pointer of
// out is positioned after the last byte written.
//
//   en.setFile(File *out);
//   int decode(Encoder& en);
//
// Decodes and returns one byte.  Input is from en.decompressByte(), which
// reads from out if in COMPRESS mode.  During compression, n calls
// to decode() must exactly match n bytes of in, or else it is compressed
// as type 0 without encoding.
//
//   BlockType detect(File *in, int n, BlockType type);
//
// Reads n bytes of in, and detects when the type changes to
// something else.  If it does, then the file pointer is repositioned
// to the start of the change and the new type is returned.  If the type
// does not change, then it repositions the file pointer n bytes ahead
// and returns the old type.
//
// For each type X there are the following 2 functions:
//
//   void encode_X(File *in, File *out, int n, ...);
//
// encodes n bytes from in to out.
//
//   int decode_X(Encoder& en);
//
// decodes one byte from en and returns it.  decode() and decode_X()
// maintain state information using static variables.

#endif //DISABLE_ZLIB

static bool isGrayscalePalette(File *in, int n = 256, int isRGBA = 0) {
  uint64_t offset = in->curPos();
  int stride = 3 + isRGBA;
  int res = (n > 0) << 8;
  int order = 1;
  for( int i = 0; (i < n * stride) && ((res >> 8) != 0); i++ ) {
    int b = in->getchar();
    if( b == EOF) {
      res = 0;
      break;
    }
    if (i == 0) {
      res = 0x100 | b;
      order = 1 - 2 * static_cast<int>(b > int(ilog2(n) / 4));
      continue;
    }
    //"j" is the index of the current byte in this color entry
    int j = i % stride;
    if (j == 0) {
      // load first component of this entry
      int k = (b - (res & 0xFF)) * order;
      res = res & (static_cast<int>(k >= 0 && k <= 8) << 8);
      res |= (res) != 0 ? b : 0;
    }
    else if (j == 3) {
    res &= (static_cast<int>((b == 0) || (b == 0xFF)) * 0x1FF); // alpha/attribute component must be zero or 0xFF
    }
    else {
    res &= (static_cast<int>(b == (res & 0xFF)) * 0x1FF);
    }
  }
  in->setpos(offset);
  return (res >> 8) > 0;
}

// Checks whether a TIFF ColorMap (tag 320) represents a grayscale palette.
// Assumes little-endian TIFF ("II")
// TIFF ColorMap layout: all R entries (uint16 LE), then all G, then all B.
// nEntries is the number of entries per channel (e.g. 256 for an 8-bit image).
// fileOffset is the absolute file offset to the start of the ColorMap data.
// Returns true if R==G==B for all entries (i.e. the palette is grayscale).
static bool isTiffGrayscaleColorMap(File* in, uint64_t fileOffset, int nEntries) {
  in->setpos(fileOffset);
  Array<uint8_t> rHi(nEntries);
  for (int e = 0; e < nEntries; ++e) {
    int lo = in->getchar(); int hi = in->getchar();
    if (lo == EOF || hi == EOF) return false;
    rHi[e] = static_cast<uint8_t>(hi);
  }
  for (int e = 0; e < nEntries; ++e) {
    int lo = in->getchar(); int hi = in->getchar();
    if (lo == EOF || hi == EOF) return false;
    if (static_cast<uint8_t>(hi) != rHi[e]) return false;
  }
  for (int e = 0; e < nEntries; ++e) {
    int lo = in->getchar(); int hi = in->getchar();
    if (lo == EOF || hi == EOF) return false;
    if (static_cast<uint8_t>(hi) != rHi[e]) return false;
  }
  return true;
}


struct DetectionInfo {
  uint64_t HeaderStart{};
  uint64_t HeaderLength{};
  uint64_t DataStart{};
  uint64_t DataLength{};
  BlockType Type{};
  int DataInfo{};

  void IMG_DET(uint64_t blockStart, BlockType type, uint64_t start_pos, uint32_t header_len, uint32_t width, uint32_t height) {
    Type = type;
    HeaderStart = blockStart + start_pos;
    HeaderLength = header_len;
    DataStart = HeaderStart + HeaderLength;
    DataLength = static_cast<uint64_t>(width) * height;
    DataInfo = width;
  }

  bool SizeVerificationPassed(uint64_t nextBlockStart) {
    bool passed = DataStart + DataLength <= nextBlockStart;
    if (!passed)
      memset(this, 0, sizeof(DetectionInfo));
    return passed;
  }
};

// Detect blocks
static DetectionInfo detect(File *in, uint64_t blockSize, const TransformOptions* const transformOptions) {

  DetectionInfo detectionInfo;

  int textParserState = 0;
  uint64_t blockHash = 0;

  // TODO: Large file support
  const uint64_t n = blockSize;

  // last 16 bytes
  uint32_t buf3 = 0;
  uint32_t buf2 = 0;
  uint32_t buf1 = 0;
  uint32_t buf0 = 0;
  
  uint64_t start = 0;

  start = in->curPos(); // start of the current block
  
  // For JPEG detection
  uint64_t soi = 0; // Start Of Image 
  uint64_t sof = 0; // Start Of Frame
  uint64_t sos = 0; // Start Of Scan
  uint64_t app = 0; // Application-specific marker
  uint64_t firstSoi = 0;

  // For BMP detection
  uint64_t bmpi = 0;
  uint64_t dibi = 0;
  int imgbpp = 0;
  int bmpx = 0;
  int bmpy = 0;
  int bmpof = 0;
  int bmps = 0;
  int nColors = 0;
  
  // For RGB detection
  uint64_t rgbi = 0;
  int rgbx = 0;
  int rgby = 0; 
  
  // For TGA detection
  uint64_t tga = 0;
  int tgax = 0;
  int tgay = 0;
  int tgaz = 0;
  int tgat = 0;
  int tgaid = 0;
  int tgamap = 0; 
  
  // For PBM (Portable BitMap), PGM (Portable GrayMap), PPM (Portable PixMap), PAM (Portable Arbitrary Map) detection
  uint64_t pgm = 0;
  int pgmComment = 0; // flag for presence of a comment line
  int pgmw = 0; // width
  int pgmh = 0; // height
  int pgmc = 0;  // color depth
  int pgmn = 0; // format: 4: PBM, 5: PGM 6: PPM, 7: PAM
  int pamatr = 0; // currently processed PAM attribute
  int pamd = 0; // PAM color depth (number of color channes)
  uint64_t pgmdata = 0; // image data start
  uint64_t pgmDataSize = 0; // image data size in bytes
  int pgmPtr = 0; // index in pgmBuf
  char pgmBuf[32];
  
  // For ZLIB stream detection
  uint8_t zBuf[256 + 32] = {0};
  uint8_t zin[1 << 16] = {0};
  uint8_t zout[1 << 16] = {0};

  int zBufPos = 0;
  // For GIF detection
  uint64_t gifi = 0;
  uint64_t gifa = 0;
  int gif = 0;
  int gifw = 0;
  int gifc = 0;
  int gifb = 0;
  int gifplt = 0;
  static bool gifGray = false;
  
  // For PNG detection
  uint64_t png = 0;
  int pngw = 0;
  int pngh = 0;
  int pngbps = 0;
  int pngType = 0;
  int pnggray = 0;
  int lastChunk = 0;
  uint64_t nextChunk = 0;

  // Static state for multi-strip TIFF LZW detection.
  // Persists across detect() calls to emit one strip per call, similar to GIF's dett mechanism.
  static const int MAX_TIFF_STRIPS = 256;
  static struct
  {
    uint64_t offsets[MAX_TIFF_STRIPS];
    int      sizes[MAX_TIFF_STRIPS];
    int      info;
    int      count;
    int      next;
  } tiffStrips = {};

  // Continue emitting pending TIFF LZW strips from a previous detect() call
  if (tiffStrips.next > 0 && tiffStrips.next < tiffStrips.count) {
    const int s = tiffStrips.next++;
    detectionInfo.Type = BlockType::LZW;
    detectionInfo.DataInfo = tiffStrips.info;
    detectionInfo.DataStart = tiffStrips.offsets[s];
    detectionInfo.DataLength = tiffStrips.sizes[s];
    if (tiffStrips.next >= tiffStrips.count)
      tiffStrips.next = 0; // all strips emitted, reset
    return detectionInfo;
  }

  for (uint64_t i = 0; i < n; ++i) {
    int c = in->getchar();
    if (c == EOF) {
      quit("detect(): Unexpected end of file");
    }

    blockHash = hash(blockHash, c);

    buf3 = buf3 << 8 | buf2 >> 24;
    buf2 = buf2 << 8 | buf1 >> 24;
    buf1 = buf1 << 8 | buf0 >> 24;
    buf0 = buf0 << 8 | c;

    // helper for .pbm .pgm .ppm .pam detection
    uint32_t t = TextParserStateInfo::utf8StateTable[c];
    textParserState = TextParserStateInfo::utf8StateTable[256 + textParserState + t];

    // detect PNG images
    if ((png == 0) && buf3 == 0x89504E47 /*%PNG*/ && buf2 == 0x0D0A1A0A && buf1 == 0x0000000D && buf0 == 0x49484452) {
      png = i, pngType = -1, lastChunk = buf3;
    }
    if (png != 0) {
      const uint64_t p = i - png;
      if (p == 12) {
        pngw = buf2;
        pngh = buf1;
        pngbps = buf0 >> 24;
        pngType = static_cast<uint8_t>(buf0 >> 16); //Color type codes represent sums of the following values: 1 (palette used), 2 (color used), and 4 (alpha channel used). Valid values are 0, 2, 3, 4, and 6.
        pnggray = 0;
        png *= static_cast<int>((buf0 & 0xFFFF) == 0 && (pngw != 0) && (pngh != 0) && pngbps == 8 &&
          ((pngType == 0) || pngType == 2 || pngType == 3 || pngType == 4 || pngType == 6));
      }
      else if (p > 12 && pngType < 0) {
        png = 0;
      }
      else if (p == 17) {
        png *= static_cast<int>((buf1 & 0xFF) == 0);
        nextChunk = (png) != 0 ? i + 8 : 0;
      }
      else if (p > 17 && i == nextChunk) {
        nextChunk += buf1 + 4 /*CRC*/ + 8 /*Chunk length+id*/;
        lastChunk = buf0;
        png *= static_cast<int>(lastChunk != 0x49454E44 /*IEND*/);
        if (lastChunk == 0x504C5445 /*PLTE*/) {
          png *= static_cast<int>(buf1 % 3 == 0);
          pnggray = static_cast<int>((png != 0) && isGrayscalePalette(in, buf1 / 3));
        }
      }
    }

    // ZLIB stream detection
    zBuf[zBufPos] = c;
    if (zBufPos < 32)
      zBuf[zBufPos + 256] = c;
    zBufPos = (zBufPos + 1) & 0xFF;

    int zh = parseZlibHeader(((int)zBuf[(zBufPos - 32) & 0xFF]) * 256 + (int)zBuf[(zBufPos - 32 + 1) & 0xFF]);
    bool valid = (i >= 31 && zh != -1);
    if (valid) {
      int streamLength = 0, ret = 0;

      // Quick check possible stream by decompressing first 32 bytes
      z_stream strm;
      strm.zalloc = Z_NULL;
      strm.zfree = Z_NULL;
      strm.opaque = Z_NULL;
      strm.next_in = Z_NULL;
      strm.avail_in = 0;
      if (zlibInflateInit(&strm, zh) == Z_OK) {
        strm.next_in = &zBuf[(zBufPos - 32) & 0xFF];
        strm.avail_in = 32;
        strm.next_out = zout;
        strm.avail_out = 1 << 16;
        ret = inflate(&strm, Z_FINISH);
        ret = (inflateEnd(&strm) == Z_OK && (ret == Z_STREAM_END || ret == Z_BUF_ERROR) && strm.total_in >= 16);
      }
      if (ret) {
        // Verify valid stream and determine stream length
        const uint64_t savedpos = in->curPos();
        strm.zalloc = Z_NULL;
        strm.zfree = Z_NULL;
        strm.opaque = Z_NULL;
        strm.next_in = Z_NULL;
        strm.avail_in = 0;
        strm.total_in = strm.total_out = 0;
        if (zlibInflateInit(&strm, zh) == Z_OK) {
          uint64_t blstart = static_cast<uint64_t>(std::max<int64_t>(i - 31, 0));
          for (uint64_t j = blstart; j < n; j += 1 << 16) {
            uint32_t blsize = static_cast<uint32_t>(min(n - j, UINT64_C(1) << 16));
            in->setpos(start + j);
            if (in->blockRead(zin, blsize) != blsize)
              break;
            strm.next_in = zin;
            strm.avail_in = blsize;
            do {
              strm.next_out = zout;
              strm.avail_out = 1 << 16;
              ret = inflate(&strm, Z_FINISH);
            } while (strm.avail_out == 0 && ret == Z_BUF_ERROR);
            if (ret == Z_STREAM_END)
              streamLength = strm.total_in;
            if (ret != Z_BUF_ERROR)
              break;
          }
          if (inflateEnd(&strm) != Z_OK)
            streamLength = 0;
        }
        in->setpos(savedpos);
      }
      if (streamLength > 0) {
        int info = 0;
        if (png && pngw < 0x1000000 && lastChunk == 0x49444154 /*IDAT*/) {
          if (pngbps == 8 && pngType == 2 /*color, no alpha*/ && (int)strm.total_out == (pngw * 3 + 1) * pngh)
            info = (BlockType::PNG24 << 24) | (pngw * 3), png = 0;
          else if (pngbps == 8 && pngType == 6 /*color, with alpha*/ && (int)strm.total_out == (pngw * 4 + 1) * pngh)
            info = (BlockType::PNG32 << 24) | (pngw * 4), png = 0;
          else if (pngbps == 8 && (pngType == 0 /*no color channels, no palette, i.e. grayscale*/ || pngType == 3 /*palette, color*/) && (int)strm.total_out == (pngw + 1) * pngh)
            info = (((pngType == 0 /*grayscale png*/ || pnggray) ? BlockType::PNG8GRAY : BlockType::PNG8) << 24) | (pngw), png = 0;
        }
        detectionInfo.Type = BlockType::ZLIB;
        detectionInfo.DataInfo = info;
        detectionInfo.DataLength = streamLength;
        detectionInfo.DataStart = start + i - 31;
        return detectionInfo;
      }
    }


    // Detect JPEG by code SOI APPx (FF D8 FF Ex) followed by
    // SOF0 (FF C0 xx xx 08) and SOS (FF DA) within a reasonable distance.
    // Detect end by any code other than RST0-RST7 (FF D9-D7) or
    // a byte stuff (FF 00).

    if ((soi == 0) && i >= 3 && (buf0 & 0xffffff00) == 0xffd8ff00 && ((buf0 & 0xFE) == 0xC0 || static_cast<uint8_t>(buf0) == 0xC4 ||
        (static_cast<uint8_t>(buf0) >= 0xDB && static_cast<uint8_t>(buf0) <= 0xFE))) {
      soi = i, app = i + 2, sos = sof = 0;
    }
    if (soi != 0) {
      if (app == i && (buf0 >> 24) == 0xff && ((buf0 >> 16) & 0xff) > 0xc1 && ((buf0 >> 16) & 0xff) < 0xff) {
        app = i + (buf0 & 0xffff) + 2;
      }
      if (app < i && (buf1 & 0xff) == 0xff && (buf0 & 0xfe0000ff) == 0xc0000008) {
        sof = i;
      }
      if ((sof != 0) && sof > soi && i - sof < 0x1000 && (buf0 & 0xffff) == 0xffda) {
        sos = i;
        if (firstSoi == 0) {
          firstSoi = soi;
        }
      }
      if (i - soi > 0x40000 && (sos == 0)) { // fail
        soi = 0;
      }
    }
    if (firstSoi != 0 && (sos != 0) && i > sos && (buf0 & 0xff00) == 0xff00 && (buf0 & 0xff) != 0 && (buf0 & 0xf8) != 0xd0) {
      detectionInfo.Type = BlockType::JPEG;
      detectionInfo.DataStart = start + (firstSoi - 3);
      detectionInfo.DataLength = i + 1 - (firstSoi - 3);
      return detectionInfo;
    }





    // Detect .bmp image
    if ((bmpi == 0) && (dibi == 0)) {
      if ((buf0 & 0xffff) == 16973) { // 'BM'
        bmpi = i; // header start: bmpi-1
        dibi = i - 1 + 18; // we expect a DIB header to come
      }
      else if (buf0 == 0x28000000) { // headerless (DIB-only)
        dibi = i + 1;
      }
    }
    else {
      const uint64_t p = i - dibi + 1 + 18;
      if (p == 10 + 4) {
        bmpof = bswap(buf0), bmpi = (bmpof < 54 || start + blockSize < bmpi - 1 + bmpof) ? (dibi = 0)
          : bmpi; //offset of pixel data (this field is still located in the BMP Header)
      }
      else if (p == 14 + 4 && buf0 != 0x28000000) {
        bmpi = dibi = 0; //BITMAPINFOHEADER (0x28)
      }
      else if (p == 18 + 4) {
        bmpx = bswap(buf0), bmpi = (((bmpx & 0xff000000) != 0 || bmpx == 0) ? (dibi = 0) : bmpi); //width
      }
      else if (p == 22 + 4) {
        bmpy = abs(int(bswap(buf0))), bmpi = (((bmpy & 0xff000000) != 0 || bmpy == 0) ? (dibi = 0) : bmpi); //height
      }
      else if (p == 26 + 2) {
        bmpi = ((bswap(buf0 << 16)) != 1) ? (dibi = 0) : bmpi; //number of color planes (must be 1)
      }
      else if (p == 28 + 2) {
        imgbpp = bswap(buf0 << 16), bmpi = ((imgbpp != 1 && imgbpp != 4 && imgbpp != 8 && imgbpp != 24 && imgbpp != 32) ? (dibi = 0)
          : bmpi); //color depth
      }
      else if (p == 30 + 4) {
        bmpi = ((buf0 != 0) ? (dibi = 0) : bmpi); //compression method must be: BI_RGB (uncompressed)
      }
      else if (p == 34 + 4) {
        bmps = bswap(buf0); //image size or 0
        //else if (p==38+4) ; // the horizontal resolution of the image (ignored)
        //else if (p==42+4) ; // the vertical resolution of the image (ignored)
      }
      else if (p == 46 + 4) {
        nColors = bswap(buf0); // the number of colors in the color palette, or 0 to default to (1<<imgbpp)
        if (nColors == 0 && imgbpp <= 8) {
          nColors = 1 << imgbpp;
        }
        if (nColors > (1 << imgbpp) || (imgbpp > 8 && nColors > 0)) {
          bmpi = dibi = 0;
        }
      }
      else if (p == 50 + 4) { //the number of important colors used
        if (bswap(buf0) <= static_cast<uint32_t>(nColors) || bswap(buf0) == 0x10000000) {
          if (bmpi == 0 /*headerless*/ && (bmpx * 2 == bmpy) && imgbpp > 1 && // possible icon/cursor?
            ((bmps > 0 && bmps == ((bmpx * bmpy * (imgbpp + 1)) >> 4)) || (((bmps == 0) || bmps < ((bmpx * bmpy * imgbpp) >> 3)) &&
              ((bmpx == 8)  || (bmpx == 10) || (bmpx == 14) || (bmpx == 16) ||
               (bmpx == 20) || (bmpx == 22) || (bmpx == 24) ||
               (bmpx == 32) || (bmpx == 40) || (bmpx == 48) ||
               (bmpx == 60) || (bmpx == 64) || (bmpx == 72) ||
               (bmpx == 80) || (bmpx == 96) || (bmpx == 128) ||
               (bmpx == 256))))) {
            bmpy = bmpx;
          }

          BlockType blockType = BlockType::DEFAULT;
          int widthInBytes = 0;
          if (imgbpp == 1) {
            blockType = BlockType::IMAGE1;
            widthInBytes = (((bmpx - 1) >> 5) + 1) * 4;
          }
          else if (imgbpp == 4) {
            blockType = BlockType::IMAGE4;
            widthInBytes = ((bmpx * 4 + 31) >> 5) * 4;
          }
          else if (imgbpp == 8) {
            blockType = BlockType::IMAGE8;
            widthInBytes = (bmpx + 3) & -4;
          }
          else if (imgbpp == 24) {
            blockType = BlockType::IMAGE24;
            widthInBytes = ((bmpx * 3) + 3) & -4;
          }
          else if (imgbpp == 32) {
            blockType = BlockType::IMAGE32;
            widthInBytes = bmpx * 4;
          }

          if (imgbpp == 8) {
            const uint64_t colorPalettePos = dibi - 18 + 54;
            const uint64_t savedPos = in->curPos();
            in->setpos(colorPalettePos);
            if (isGrayscalePalette(in, nColors, 1)) {
              blockType = BlockType::IMAGE8GRAY;
            }
            in->setpos(savedPos);
          }

          const uint64_t headerPos = bmpi > 0 ? bmpi - 1 : dibi - 4;
          const uint64_t minHeaderSize = (bmpi > 0 ? 54 : 54 - 14) + nColors * 4;
          const uint64_t headerSize = bmpi > 0 ? bmpof : minHeaderSize;

          // some final sanity checks
          if (bmps != 0 &&
            bmps < widthInBytes * bmpy) { /*printf("\nBMP guard: image is larger than reported in header\n",bmps,widthInBytes*bmpy);*/
          }
          else if (start + blockSize < headerPos + headerSize + widthInBytes * bmpy) { /*printf("\nBMP guard: cropped data\n");*/
          }
          else if (headerSize == (bmpi > 0 ? 54 : 54 - 14) && nColors > 0) { /*printf("\nBMP guard: missing palette\n");*/
          }
          else if (bmpi > 0 && bmpof < minHeaderSize) { /*printf("\nBMP guard: overlapping color palette\n");*/
          }
          else if (bmpi > 0 && uint64_t(bmpi) - 1 + bmpof + widthInBytes * bmpy >
            start + blockSize) { /*printf("\nBMP guard: reported pixel data offset is incorrect\n");*/
          }
          else if (widthInBytes * bmpy <= 64) { /*printf("\nBMP guard: too small\n");*/
          } // too small - not worthy to use the image models
          else {
            detectionInfo.IMG_DET(start, blockType, headerPos, headerSize, widthInBytes, bmpy);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
        }
        bmpi = dibi = 0;
      }
    }

    // Detect binary .pbm .pgm .ppm .pam images
    if ((buf0 & 0xfff0ff) == 0x50300a) { //"Px" + line feed, where "x" shall be a number
      pgmn = (buf0 & 0xf00) >> 8; // extract "x"
      if ((pgmn >= 4 && pgmn <= 6) || pgmn == 7) {
        pgm = i, pgmdata = pgmDataSize = 0; // "P4" (pbm), "P5" (pgm), "P6" (ppm), "P7" (pam)
        pgmw = pgmh = pgmc = pamd = pgmComment = 0;
        pgmPtr = 0;
        pamatr = 0;
      }
    }
    if (pgm != 0) {
      if (pgmdata == 0) { // parse header
        if (i - pgm == 1 && c == '#') {
          pgmComment = 1; // # (pgm comment)
        }
        if ((pgmComment == 0) && (pgmPtr != 0)) {
          uint64_t s = 0;
          if (pgmn == 7) {
            if ((buf1 & 0xdfdf) == 0x5749 && (buf0 & 0xdfdfdfff) == 0x44544820) {
              pgmPtr = 0, pamatr = 1; // WIDTH
            }
            if ((buf1 & 0xdfdfdf) == 0x484549 && (buf0 & 0xdfdfdfff) == 0x47485420) {
              pgmPtr = 0, pamatr = 2; // HEIGHT
            }
            if ((buf1 & 0xdfdfdf) == 0x4d4158 && (buf0 & 0xdfdfdfff) == 0x56414c20) {
              pgmPtr = 0, pamatr = 3; // MAXVAL
            }
            if ((buf1 & 0xdfdf) == 0x4445 && (buf0 & 0xdfdfdfff) == 0x50544820) {
              pgmPtr = 0, pamatr = 4; // DEPTH
            }
            if ((buf2 & 0xdf) == 0x54 && (buf1 & 0xdfdfdfdf) == 0x55504c54 && (buf0 & 0xdfdfdfff) == 0x59504520) {
              pgmPtr = 0, pamatr = 5; // TUPLTYPE
            }
            if ((buf1 & 0xdfdfdf) == 0x454e44 && (buf0 & 0xdfdfdfff) == 0x4844520a) {
              pgmPtr = 0, pamatr = 6; // ENDHDR
            }
            if (c == 0x0a) {
              if (pamatr == 0) {
                pgm = 0;
              }
              else if (pamatr < 5) {
                s = pamatr;
              }
              if (pamatr != 6) {
                pamatr = 0;
              }
            }
          }
          else if (c == 0x20 && (pgmw == 0)) {
            s = 1;
          }
          else if (c == 0x0a && (pgmh == 0)) {
            s = 2;
          }
          else if (c == 0x0a && (pgmc == 0) && pgmn != 4) {
            s = 3;
          }
          if (s != 0) {
            pgmBuf[pgmPtr++] = 0;
            int v = atoi(pgmBuf); // parse width/height/depth/maxval value
            if (s == 1) {
              pgmw = v;
            }
            else if (s == 2) {
              pgmh = v;
            }
            else if (s == 3) {
              pgmc = v;
            }
            else if (s == 4) {
              pamd = v;
            }
            if (v == 0 || (s == 3 && v > 255)) {
              pgm = 0;
            }
            else {
              pgmPtr = 0;
            }
          }
        }
        if (pgmComment == 0) {
          pgmBuf[pgmPtr++] = c;
        }
        if (pgmPtr >= 32) {
          pgm = 0;
        }
        if ((pgmComment != 0) && c == 0x0a) {
          pgmComment = 0;
        }
        if ((pgmw != 0) && (pgmh != 0) && (pgmc == 0) && pgmn == 4) {
          pgmdata = i;
          pgmDataSize = (pgmw + 7) / 8 * pgmh;
          textParserState = 0; //start monitoring pixel data
        }
        if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 5 || (pgmn == 7 && pamd == 1 && pamatr == 6))) {
          pgmdata = i;
          pgmDataSize = pgmw * pgmh;
          textParserState = 0; //start monitoring pixel data
        }
        if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 6 || (pgmn == 7 && pamd == 3 && pamatr == 6))) {
          pgmdata = i;
          pgmDataSize = pgmw * 3 * pgmh;
          textParserState = 0; //start monitoring pixel data
        }
        if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 7 && pamd == 4 && pamatr == 6)) {
          pgmdata = i;
          pgmDataSize = pgmw * 4 * pgmh;
          textParserState = 0; //start monitoring pixel data
        }
      }
      else { // pixel data
        if (textParserState == TextParserStateInfo::utf8Reject || // for any sign of non-text data in pixel area
          (pgm - 2 == 0 && n - pgmDataSize == i)) // or the image is the whole file/block -> FINISH (success)
        {
          if ((pgmw != 0) && (pgmh != 0) && (pgmc == 0) && pgmn == 4) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE1, pgm - 2, pgmdata - pgm + 3, (pgmw + 7) / 8, pgmh);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 5 || (pgmn == 7 && pamd == 1 && pamatr == 6))) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE8GRAY, pgm - 2, pgmdata - pgm + 3, pgmw, pgmh);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 6 || (pgmn == 7 && pamd == 3 && pamatr == 6))) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE24, pgm - 2, pgmdata - pgm + 3, pgmw * 3, pgmh);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if ((pgmw != 0) && (pgmh != 0) && (pgmc != 0) && (pgmn == 7 && pamd == 4 && pamatr == 6)) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE32, pgm - 2, pgmdata - pgm + 3, pgmw * 4, pgmh);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          pgm = 0; //fail
        }
        else if ((--pgmDataSize) == 0) {
          pgm = 0; // all data was probably text in pixel area: fail
        }
      }
    }

    // Detect .rgb image
    if ((buf0 & 0xffff) == 0x01da) {
      rgbi = i, rgbx = rgby = 0;
    }
    if (rgbi != 0) {
      const uint64_t p = i - rgbi;
      if (p == 1 && c != 0) {
        rgbi = 0;
      }
      else if (p == 2 && c != 1) {
        rgbi = 0;
      }
      else if (p == 4 && (buf0 & 0xffff) != 1 && (buf0 & 0xffff) != 2 && (buf0 & 0xffff) != 3) {
        rgbi = 0;
      }
      else if (p == 6) {
        rgbx = buf0 & 0xffff, rgbi = (rgbx == 0 ? 0 : rgbi);
      }
      else if (p == 8) {
        rgby = buf0 & 0xffff, rgbi = (rgby == 0 ? 0 : rgbi);
      }
      else if (p == 10) {
        int z = buf0 & 0xffff;
        if ((rgbx != 0) && (rgby != 0) && (z == 1 || z == 3 || z == 4)) {
          detectionInfo.IMG_DET(start, BlockType::IMAGE8, rgbi - 1, 512, rgbx, rgby * z);
          if (detectionInfo.SizeVerificationPassed(start + n))
            return detectionInfo;
        }
        rgbi = 0;
      }
    }

    // Detect .tiff file header (little-endian (II), uncompressed or LZW-compressed,
    // 1/4/8/24-bit color, single or multi-strip).
    // Note: big-endian (Motorola) is not supported
    if (buf1 == 0x49492a00 && n > i + static_cast<int>(bswap(buf0))) { // "II*\0"
      const uint64_t savedPos = in->curPos();
      // Seek to the Image File Directory (IFD); buf0 holds the IFD offset (4 bytes, LE).
      // We subtract 7 because 'i' points to the last byte of the 8-byte TIFF header,
      in->setpos(start + i + static_cast<uint64_t>(bswap(buf0)) - 7);

      // IFD layout: 2-byte entry count (read as two separate bytes here),
      // followed by 12-byte entries: tag(2) + type(2) + count(4) + value/offset(4)
      int dirSize = in->getchar(); // low byte of entry count
      int tifX = 0; // tag 256: image width in pixels
      int tifY = 0; // tag 257: image height in pixels
      int tifZ = 0; // tag 277: samples per pixel (1 = grayscale/palette, 3 = RGB)
      int tifZb = 0; // tag 258: bits per sample (1, 4, 8)
      int tifC = 0; // tag 259: compression (1 = none, 5 = LZW)
      int tifofval = 0; // 1 if strip offsets are already resolved (inline or read from array)

      // Tag 320 (ColorMap): only present for palette-color images (PhotometricInterpretation==3).
      // Stores 3 * 2^BitsPerSample uint16 values: all R entries, then all G, then all B.
      int tifColorMapOfs = 0; // absolute file offset to the ColorMap data, 0 = absent
      int tifColorMapLen = 0; // number of palette entries per channel (e.g. 256 for 8-bit)

      // Tag 273 (StripOffsets) and tag 279 (StripByteCounts): a TIFF image may be split
      // into multiple horizontal strips, each independently compressed.
      // We support up to 256 strips.
      uint64_t tifStripOffsets[MAX_TIFF_STRIPS] = {}; // absolute file offset of each strip
      int      tifStripSizes[MAX_TIFF_STRIPS] = {}; // compressed byte count of each strip
      int      tifStripCount = 0;                     // number of strips

      int b[12];
      if (in->getchar() == 0) { // high byte of entry count must be 0 (max 255 entries)
        for (int i = 0; i < dirSize; i++) {
          for (int j = 0; j < 12; j++) {
            b[j] = in->getchar();
          }
          if (b[11] == EOF)
            break;
          int tag = b[0] + (b[1] << 8);           // IFD tag identifier
          int tagFmt = b[2] + (b[3] << 8);        // data type: 3=SHORT(uint16), 4=LONG(uint32)
          int tagLen = b[4] + (b[5] << 8) + (b[6] << 16) + (b[7] << 24); // number of values
          int tagVal = b[8] + (b[9] << 8) + (b[10] << 16) + (b[11] << 24); // value or file offset to value array
          if (tagFmt == 3 || tagFmt == 4) {
            if (tag == 256) {
              tifX = tagVal; // ImageWidth
            }
            else if (tag == 257) {
              tifY = tagVal; // ImageLength (height)
            }
            else if (tag == 258) {
              // BitsPerSample: bits per channel.
              // For multi-channel images tagLen > 1 (one value per channel),
              // but all channels have the same depth so we just use 8 as default.
              tifZb = tagLen == 1 ? tagVal : 8; // bits per component
            }
            else if (tag == 259) {
              tifC = tagVal; // Compression: 1 = uncompressed, 5 = LZW
            }
            else if (tag == 273 && (tagFmt == 3 || tagFmt == 4)) {
              // StripOffsets: file offsets to each strip's data.
              // If tagLen == 1 the single value fits inline in the IFD entry.
              // If tagLen > 1 the value is a file offset to an array of offsets.
              const int bytesPerVal = (tagFmt == 3) ? 2 : 4; // SHORT or LONG
              if (tagLen == 1) {
                tifStripOffsets[0] = tagVal; // single strip, value is inline
                tifStripCount = 1;
                tifofval = 1; // already resolved, no indirection needed
              }
              else if (tagLen <= MAX_TIFF_STRIPS) {
                // tagVal is the file offset to the array of strip offsets
                const uint64_t savedPos2 = in->curPos();
                in->setpos(tagVal);
                tifStripCount = tagLen;
                for (int s = 0; s < tagLen; s++) {
                  uint64_t ofs = in->getchar();
                  ofs += (uint64_t)in->getchar() << 8;
                  if (bytesPerVal == 4) {
                    ofs += (uint64_t)in->getchar() << 16;
                    ofs += (uint64_t)in->getchar() << 24;
                  }
                  tifStripOffsets[s] = ofs;
                }
                in->setpos(savedPos2);
                tifofval = 1; // offsets already resolved from array
              }
            }
            else if (tag == 277) {
              tifZ = tagVal; // SamplesPerPixel: components per pixel
            }
            else if (tag == 279) {
              // StripByteCounts: compressed (or uncompressed) byte count for each strip.
              // If tagLen == 1 the single value fits inline in the IFD entry.
              // If tagLen > 1 the value is a file offset to an array of counts.
              const int bytesPerVal = (tagFmt == 3) ? 2 : 4; // SHORT or LONG
              if (tagLen == 1) {
                tifStripSizes[0] = tagVal; // single strip, value is inline
              }
              else if (tagLen <= MAX_TIFF_STRIPS) {
                // tagVal is the file offset to the array of strip byte counts
                const uint64_t savedPos2 = in->curPos();
                in->setpos(tagVal);
                for (int s = 0; s < tagLen; s++) {
                  int v = in->getchar();
                  v += in->getchar() << 8;
                  if (bytesPerVal == 4) {
                    v += in->getchar() << 16;
                    v += in->getchar() << 24;
                  }
                  tifStripSizes[s] = v;
                }
                in->setpos(savedPos2);
              }
            }
            // ColorMap tag (320): only present for palette-color images
            // (PhotometricInterpretation == 3).
            // tagFmt == 3 (SHORT), tagLen == 3 * 2^BitsPerSample entries total
            // (e.g. 768 for 8-bit: 256 R + 256 G + 256 B uint16 values).
            // Since tagLen > 2 always for a valid palette, tagVal is a file offset
            // to the palette data (doesn't fit inline in the 4-byte IFD value field).
            else if (tag == 320 && tagFmt == 3 && tagLen > 2) {
              tifColorMapOfs = tagVal;  // absolute file offset to the ColorMap data
              tifColorMapLen = tagLen / 3; // entries per channel (e.g. 256 for 8-bit)
            }
          }
        }
      }

      // Compute total compressed size across all strips
      int tifTotalSize = 0;
      for (int s = 0; s < tifStripCount; s++)
        tifTotalSize += tifStripSizes[s];

      const uint64_t tifofs = tifStripCount > 0 ? tifStripOffsets[0] : 0;

      if ((tifX != 0) && (tifY != 0) && (tifZb != 0) && (tifZ == 1 || tifZ == 3) &&
        ((tifC == 1) || (tifC == 5 && tifTotalSize > 0)) &&
        (tifofs != 0) && tifofs < (1 << 18) && tifofs + i < n)
      {
        if (tifC == 1) {
          // Uncompressed (BI_RGB): pixel data starts at tifofs
          if (tifZ == 1 && tifZb == 1) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE1, i - 7, tifofs, ((tifX - 1) >> 3) + 1, tifY);
            return detectionInfo;
          }
          if (tifZ == 1 && tifZb == 8) {
            // 8-bit single channel: grayscale unless a ColorMap says otherwise
            BlockType tifType = BlockType::IMAGE8GRAY; // default: no ColorMap = grayscale
            if (tifColorMapOfs != 0 && tifColorMapLen > 0) {
              // ColorMap is present: seek to it and inspect all entries.
              // TIFF ColorMap layout is planar (all R, then all G, then all B),
              // unlike interleaved RGB â so we use isTiffGrayscaleColorMap()
              // rather than the generic isGrayscalePalette().
              const uint64_t cmapEnd = static_cast<uint64_t>(tifColorMapOfs) + tifColorMapLen * 6u;
              if (cmapEnd <= start + n) {
                tifType = isTiffGrayscaleColorMap(in, tifColorMapOfs, tifColorMapLen)
                  ? BlockType::IMAGE8GRAY
                  : BlockType::IMAGE8;
              }
              else {
                // ColorMap offset is past end of file: corrupted?
              }
            }
            detectionInfo.IMG_DET(start, tifType, i - 7, tifofs, tifX, tifY);
            return detectionInfo;
          }
          if (tifZ == 3 && tifZb == 8) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE24, i - 7, tifofs, tifX * 3, tifY);
            return detectionInfo;
          }
        }
        else if (tifC == 5 && tifTotalSize > 0) {
          // LZW compressed: determine the image type and uncompressed row stride.
          // Each strip is an independent LZW stream (with its own RESET and EOI codes),
          // so multi-strip images are emitted one strip at a time via tiffStrips state.
          int widthInBytes;
          BlockType tifType;
          if (tifZb == 1) {
            tifType = BlockType::IMAGE1;
            widthInBytes = ((tifX - 1) / 8 + 1) * tifZ; // ceil(width/8) bytes per row
          }
          else if (tifZb == 4) {
            tifType = BlockType::IMAGE4;
            widthInBytes = (tifX + 1) / 2 * tifZ; // ceil(width/2) bytes per row, 2 pixels per byte
          }
          else { // tifZb == 8
            tifType = (tifZ == 1) ? BlockType::IMAGE8 : BlockType::IMAGE24;
            widthInBytes = tifX * tifZ; // one byte per channel per pixel
          }
          // Pack image type into high byte and row stride into low 24 bits,
          // matching the info encoding used by other image block types
          const int info = (static_cast<int>(tifType) << 24) | widthInBytes;

          if (tifStripCount == 1) {
            // Single strip: emit directly, no static state needed
            detectionInfo.Type = BlockType::LZW;
            detectionInfo.DataInfo = info;
            detectionInfo.DataStart = tifStripOffsets[0];
            detectionInfo.DataLength = tifStripSizes[0];
            return detectionInfo;
          }
          else {
            // Multi-strip: TIFF does not guarantee strips are contiguous in the file.
            // We only handle the contiguous case (strips laid out sequentially with
            // no gaps), since the LZW decoder expects a single linear byte range.
            // Non-contiguous strips fall through to DEFAULT.
            bool contiguous = true;
            for (int s = 1; s < tifStripCount; s++) {
              if (tifStripOffsets[s] != tifStripOffsets[s - 1] + tifStripSizes[s - 1]) {
                contiguous = false;
                break;
              }
            }
            if (contiguous) {
              // Store all strips in static state and emit the first strip now.
              // Subsequent detect() calls will emit strips 1..count-1 via the
              // early-return block at the top of detect().
              tiffStrips.count = tifStripCount;
              tiffStrips.info = info;
              tiffStrips.next = 1; // next call will emit strip 1
              for (int s = 0; s < tifStripCount; s++) {
                tiffStrips.offsets[s] = tifStripOffsets[s];
                tiffStrips.sizes[s] = tifStripSizes[s];
              }
              detectionInfo.Type = BlockType::LZW;
              detectionInfo.DataInfo = info;
              detectionInfo.DataStart = tiffStrips.offsets[0];
              detectionInfo.DataLength = tiffStrips.sizes[0];
              return detectionInfo;
            }
            // Non-contiguous strips: fall through to savedPos restore
          }
        }
      }
      in->setpos(savedPos);
    }

    // Detect .tga image (8-bit 256 colors or 24-bit uncompressed)
    if ((buf1 & 0xFFF7FF) == 0x00010100 && (buf0 & 0xFFFFFFC7) == 0x00000100 && (c == 16 || c == 24 || c == 32)) {
      tga = i, tgax = tgay, tgaz = 8, tgat = (buf1 >> 8) & 0xF, tgaid = buf1 >> 24, tgamap = c / 8;
    }
    else if ((buf1 & 0xFFFFFF) == 0x00000200 && buf0 == 0x00000000) {
      tga = i, tgax = tgay, tgaz = 24, tgat = 2;
    }
    else if ((buf1 & 0xFFF7FF) == 0x00000300 && buf0 == 0x00000000) {
      tga = i, tgax = tgay, tgaz = 8, tgat = (buf1 >> 8) & 0xF;
    }
    if (tga != 0) {
      if (i - tga == 8) {
        tga = (buf1 == 0 ? tga : 0), tgax = (bswap(buf0) & 0xffff), tgay = (bswap(buf0) >> 16);
      }
      else if (i - tga == 10) {
        if ((buf0 & 0xFFF7) == 32 << 8) {
          tgaz = 32;
        }
        if ((tgaz << 8) == static_cast<int>(buf0 & 0xFFD7) && (tgax != 0) && (tgay != 0) && uint32_t(tgax * tgay) < 0xFFFFFFF) {
          if (tgat == 1) {
            in->setpos(start + tga + 11 + tgaid);
            bool isGray = isGrayscalePalette(in);
            detectionInfo.IMG_DET(start, isGray ? BlockType::IMAGE8GRAY : BlockType::IMAGE8, tga - 7, 18 + tgaid + 256 * tgamap, tgax, tgay);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if (tgat == 2) {
            detectionInfo.IMG_DET(start, (tgaz == 24) ? BlockType::IMAGE24 : BlockType::IMAGE32, tga - 7, 18 + tgaid, tgax * (tgaz >> 3), tgay);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if (tgat == 3) {
            detectionInfo.IMG_DET(start, BlockType::IMAGE8GRAY, tga - 7, 18 + tgaid, tgax, tgay);
            if (detectionInfo.SizeVerificationPassed(start + n))
              return detectionInfo;
          }
          if (tgat == 9 || tgat == 11) {
            int info;
            const uint64_t savedPos = in->curPos();
            in->setpos(start + tga + 11 + tgaid);
            if (tgat == 9) {
              bool isGray = isGrayscalePalette(in);
              info = (isGray ? BlockType::IMAGE8GRAY : BlockType::IMAGE8) << 24;
              in->setpos(start + tga + 11 + tgaid + 256 * tgamap);
            }
            else {
              info = BlockType::IMAGE8GRAY << 24;
            }
            info |= tgax;
            // now detect compressed image data size
            uint32_t detd = 0;
            int c = in->getchar();
            int b = 0;
            int total = tgax * tgay;
            int line = 0;
            while (total > 0 && c >= 0 && (++detd, b = in->getchar()) >= 0) {
              if (c == 0x80) {
                c = b;
                continue;
              }
              if (c > 0x7F) {
                total -= (c = (c & 0x7F) + 1);
                line += c;
                c = in->getchar();
                detd++;
              }
              else {
                in->setpos(in->curPos() + c);
                detd += ++c;
                total -= c;
                line += c;
                c = in->getchar();
              }
              if (line > tgax) {
                break;
              }
              if (line == tgax) {
                line = 0;
              }
            }
            if (total == 0) {
              detectionInfo.Type = BlockType::RLE;
              detectionInfo.DataInfo = info;
              detectionInfo.DataStart = start + tga + 11 + tgaid + 256 * tgamap;
              detectionInfo.DataLength = detd;
              if (detectionInfo.SizeVerificationPassed(start + n))
                return detectionInfo;
            }
            in->setpos(savedPos);
          }
        }
        tga = 0;
      }
    }

    static BlockType dett = BlockType::DEFAULT;

    // Detect .gif
    if (detectionInfo.Type == BlockType::DEFAULT && dett == BlockType::GIF && i == 0) {
      dett = BlockType::DEFAULT;
      if (c == 0x2c || c == 0x21) { //image section or extension block
        gif = 2; // flag: image section or extension block
        gifi = 2; //jump 2 bytes
      }
      else {
        gifGray = 0;
      }
    }
    if ((gif == 0) && (buf1 & 0xffff) == 0x4749 /*GI*/ && (buf0 == 0x46383961 /*F89a*/ || buf0 == 0x46383761 /*F87a*/)) {
      gif = 1; // flag: found header
      gifi = i + 5; // position after the header
    }
    if (gif != 0) { //we are in a GIF file
      if (gif == 1 && i == gifi) {
        gif = 2;
        gifplt = (c & 128) != 0 ? (3 * (2 << (c & 7))) : 0;
        gifi = i + 5 + gifplt;
      }
      if (gif == 2 && (gifplt != 0) && i == gifi - gifplt - 3) {
        gifGray = isGrayscalePalette(in, gifplt / 3);
        gifplt = 0;
      }
      if (gif == 2 && i == gifi) {
        if ((buf0 & 0xff0000) == 0x210000) {
          gif = 5;
          gifi = i;
        }
        else if ((buf0 & 0xff0000) == 0x2c0000) {
          gif = 3;
          gifi = i;
        }
        else {
          gif = 0; //failed
        }
      }
      if (gif == 3 && i == gifi + 6) {
        gifw = (bswap(buf0) & 0xffff);
      }
      if (gif == 3 && i == gifi + 7) {
        gif = 4;
        gifc = gifb = 0;
        gifplt = (c & 128) != 0 ? (3 * (2 << (c & 7))) : 0;
        gifa = gifi = i + 2 + gifplt;
      }
      if (gif == 4 && (gifplt != 0)) {
        gifGray = isGrayscalePalette(in, gifplt / 3);
        gifplt = 0;
      }
      if (gif == 4 && i == gifi) {
        if (c > 0 && gifb != 0 && gifc != gifb) {
          gifw = 0;
        }
        if (c > 0) {
          gifb = gifc;
          gifc = c;
          gifi += c + 1;
        }
        else if (gifw == 0) {
          gif = 2;
          gifi = i + 3;
        }
        else {
          dett = BlockType::GIF;
          detectionInfo.Type = BlockType::GIF;
          detectionInfo.DataStart = start + gifa - 1;
          detectionInfo.DataLength = i - gifa + 2;
          detectionInfo.DataInfo = ((gifGray ? BlockType::IMAGE8GRAY : BlockType::IMAGE8) << 24) | gifw;
          return detectionInfo;
        }
      }
      if (gif == 5 && i == gifi) {
        if (c > 0) {
          gifi += c + 1;
        }
        else {
          gif = 2;
          gifi = i + 3;
        }
      }
    }


  }
  
  if (detectionInfo.Type != BlockType::DEFAULT)
    quit("detect(): detection didn't finish properly.");

  //special detections (using hash signature)
  //we could detect these *types* of content properly, but since
  //they are rare, it's just much simpler to detect only those files 
  //we are targeting

  //printf("BlockHash: %" PRIu64 "\n", blockHash);

  if (blockHash == UINT64_C(16175250862432333790) && blockSize == 513216) { //calgary/pic; canterbury/ptt5
    detectionInfo.IMG_DET(start, BlockType::IMAGE1, 0, 0, 1728 / 8, 2376);
    return detectionInfo;
  }

  //nothing detected
  detectionInfo.Type = BlockType::DEFAULT;
  detectionInfo.DataStart = start + n;
  detectionInfo.DataLength = 0;

  return detectionInfo;
}

#include "bmp.hpp"
#include "rle.hpp"

//////////////////// Compress, Decompress ////////////////////////////

static void directEncodeBlock(BlockType type, File *in, uint64_t len, Encoder &en, int info) {
  // TODO: Large file support
  Block::EncodeBlockHeader(&en, type, len, info);
  fprintf(stderr, "Compressing... ");
  for( uint64_t j = 0; j < len; ++j ) {
    if((j & 0xfff) == 0 ) {
      en.printStatus(j, len);
    }
    en.compressByte(en.predictorMain, in->getchar());
  }
  fprintf(stderr, "\b\b\b\b\b\b\b\b\b\b\b\b\b\b\b");
}

static void compressRecursive(File *in, uint64_t blockSize, Encoder &en, String &blstr, float p1, float p2, const TransformOptions* const transformOptions, const bool isTopLevel = true);

static uint64_t decodeFunc(BlockType type, Encoder &en, File *tmp, uint64_t len, int info, File *out, FMode mode, uint64_t &diffFound, const TransformOptions* const transformOptions) {
  if( type == BlockType::IMAGE24 ) {
    auto f = BmpFilter();
    f.setWidth(info);
    f.setSkipRgb(transformOptions->skipRgb);
    f.setEncoder(en);
    return f.decode(tmp, out, mode, len, diffFound);
  }
  if( type == BlockType::IMAGE32 ) {
    auto f = BmpFilter();
    f.setWidth(info);
    f.setSkipRgb(transformOptions->skipRgb);
    f.setHasApha();
    f.setEncoder(en);
    return f.decode(tmp, out, mode, len, diffFound);
  }
  if (type == BlockType::PNG8GRAY || type == BlockType::PNG8 || type == BlockType::PNG24|| type == BlockType::PNG32) {
    auto f = PngFilter();
    f.setWidth(info);
    auto stride = type == BlockType::PNG24 ? 3 : type == BlockType::PNG32 ? 4 : 1;
    f.setStride(stride);
    f.setEncoder(en);
    return f.decode(tmp, out, mode, len, diffFound);
  }
  if( type == BlockType::ZLIB ) {
    return decodeZlib(tmp, len, out, mode, diffFound);
  }
  else if( type == BlockType::GIF ) {
    return decodeGif(tmp, len, out, mode, diffFound);
  } else if( type == BlockType::RLE ) {
    auto f = RleFilter();
    //f.setScanLineSize(info & 0xFFFFFF); //now it self-encodes this info, but eventually we need to pass it
    return f.decode(tmp, out, mode, len, diffFound);
  } else if( type == BlockType::LZW ) {
    return decodeLzw(tmp, out, mode, diffFound);
  } else {
    assert(false);
  }
  return 0;
}

static uint64_t encodeFunc(BlockType type, File *in, File *tmp, uint64_t len, int info, int &hdrsize, const TransformOptions* const transformOptions) {
  if( type == BlockType::IMAGE24 ) {
    auto f= BmpFilter();
    f.setSkipRgb(transformOptions->skipRgb);
    f.encode(in, tmp, len, info, hdrsize);
  } else if( type == BlockType::IMAGE32 ) {
    auto f = BmpFilter();
    f.setSkipRgb(transformOptions->skipRgb);
    f.setHasApha();
    f.encode(in, tmp, len, info, hdrsize);
  } else if (type == BlockType::PNG8GRAY || type == BlockType::PNG8 || type == BlockType::PNG24 || type == BlockType::PNG32) {
    auto f = PngFilter();
    auto stride = type == BlockType::PNG24 ? 3 : type == BlockType::PNG32 ? 4 : 1;
    f.setStride(stride);
    f.encode(in, tmp, len, info, hdrsize);
  } else if( type == BlockType::ZLIB ) {
    return encodeZlib(in, tmp, len, hdrsize) ? 0 : 1;
  } else if( type == BlockType::GIF ) {
    return encodeGif(in, tmp, len, hdrsize) != 0 ? 0 : 1;
  } else if( type == BlockType::RLE ) {
    auto f = RleFilter();
    f.setScanLineSize(info & 0xFFFFFF);
    f.encode(in, tmp, len, info, hdrsize);
  } else if( type == BlockType::LZW ) {
    return encodeLzw(in, tmp, len, hdrsize) != 0 ? 0 : 1;
  } else {
    assert(false);
  }
  return 0;
}

static void
transformEncodeBlock(BlockType type, File *in, uint64_t len, Encoder &en, int info, String &blstr, float p1, float p2, uint64_t begin, const TransformOptions* const transformOptions) {
  if( hasTransform(type, info)) {
    FileTmp tmp;
    int headerSize = 0;
    uint64_t diffFound = encodeFunc(type, in, &tmp, len, info, headerSize, transformOptions);
    const uint64_t tmpSize = tmp.curPos();
    tmp.setpos(tmpSize); //switch to read mode
    if( diffFound == 0 ) {
      tmp.setpos(0);
      en.setFile(&tmp);
      in->setpos(begin);
      decodeFunc(type, en, &tmp, tmpSize, info, in, FMode::FCOMPARE, diffFound, transformOptions);
    }
    // Test fails, compress without transform
    if( diffFound > 0 || tmp.getchar() != EOF) {
      printf("Transform fails at %" PRIu64 ", skipping...\n", diffFound - 1);
      in->setpos(begin);
      directEncodeBlock(BlockType::DEFAULT, in, len, en, -1);
    } else {
      tmp.setpos(0);
      if( hasRecursion(type)) {
        Block::EncodeBlockHeader(&en, type, tmpSize, info&0xffffff);
        BlockType type2 = static_cast<BlockType>((info >> 24) & 0xFF);
        if (isPNG(type)) {
            type2 =
                type == BlockType::PNG8GRAY ? BlockType::IMAGE8GRAY :
                type == BlockType::PNG8 ? BlockType::IMAGE8 :
                type == BlockType::PNG24 ? BlockType::IMAGE24 :
                type == BlockType::PNG32 ? BlockType::IMAGE32 : BlockType::DEFAULT;
        }
        if( type2 != BlockType::DEFAULT ) {
          String blstrSub0;
          blstrSub0 += blstr.c_str();
          blstrSub0 += "->";
          String blstrSub1;
          blstrSub1 += blstr.c_str();
          blstrSub1 += "-->";
          String blstrSub2;
          blstrSub2 += blstr.c_str();
          blstrSub2 += "-->";
          const char* exploded = 
                           "exploded    ";
          const char* addedheader =
            isPNG(type)  ? "filter data " : // PNG ->Image
                           "added header";
          const char* dataname =
            isPNG(type2) ? "png data    " : // ZLIB -> PNG
            isPNG(type)  ? "pixel data  " : // PNG  -> Image
                           "data        ";
          if(!isPNG(type))
            printf(" %-11s | ->  %s |%10d bytes [%d - %d]\n", blstrSub0.c_str(), exploded, int(tmpSize), 0, int(tmpSize - 1));
          if (headerSize != 0) {
            printf(" %-11s | --> %s |%10d bytes [%d - %d]\n", blstrSub1.c_str(), addedheader, headerSize, 0, headerSize - 1);
            directEncodeBlock(BlockType::HDR, &tmp, headerSize, en, -1);
            printf(" %-11s | --> %s |%10d bytes [%d - %d]\n", blstrSub2.c_str(), dataname, int(tmpSize - headerSize), headerSize, int(tmpSize - 1));
          }
          transformEncodeBlock(type2, &tmp, tmpSize - headerSize, en, info & 0xffffff, blstr, p1, p2, headerSize, transformOptions);
        } else {
          compressRecursive(&tmp, tmpSize, en, blstr, p1, p2, transformOptions, /*isTopLevel: */ false);
        }
      } else {
        directEncodeBlock(type, &tmp, tmpSize, en, info);
      }
    }
    tmp.close();
  } else {
    directEncodeBlock(type, in, len, en, info);
  }
}

static void composeSubBlockStringToPrint(String& blstr, String& blstrSub, int blNum) {
  //Compose block enumeration string
  blstrSub += blstr.c_str();
  if (blstrSub.strsize() != 0) {
    blstrSub += "-";
  }
  blstrSub += uint64_t(blNum);
}


static void printBlock(const uint64_t begin, const uint64_t len, const BlockType type, const int blockInfo, String& blstrSub) {
  static const char* typeNames[(int)BlockType::Count] = { "default", "jpeg", "hdr", "1b-image", "4b-image", "8b-image", "8b-img-grayscale",
                                                         "24b-image", "32b-image", "zlib", "gif", "png-8b", "png-8b-grayscale",
                                                         "png-24b", "png-32b", "rle", "lzw" };

  const char* typeName =
    type == BlockType::ZLIB && isPNG(BlockType(blockInfo >> 24)) ? typeNames[blockInfo >> 24] :
    typeNames[(int)type];
  printf(" %-11s | %-16s |%10" PRIu64 " bytes [%" PRIu64 " - %" PRIu64 "]", blstrSub.c_str(), typeName, len, begin, (begin + len) - 1);
  if (
    type == BlockType::IMAGE1 ||
    type == BlockType::IMAGE4 ||
    type == BlockType::IMAGE8 ||
    type == BlockType::IMAGE8GRAY ||
    type == BlockType::IMAGE24 ||
    type == BlockType::IMAGE32 ||
    (type == BlockType::ZLIB && isPNG(BlockType(blockInfo >> 24)))) {
    printf(" (width: %d)", (type == BlockType::ZLIB) ? (blockInfo & 0xFFFFFF) : blockInfo);
  }
  else if (hasRecursion(type) && (blockInfo >> 24) != (int)BlockType::DEFAULT) {
      printf(" (%s)", typeNames[blockInfo >> 24]);
  }
  printf("\n");
}

static void compressBlock(File* in, const uint64_t begin, const uint64_t len, int &blNum, BlockType type, int blockInfo, Encoder& en, String& blstr, float &p1, float &p2, const float pscale, const TransformOptions* const transformOptions) {
  p2 = p1 + pscale * len;
  en.setStatusRange(p1, p2);

  String blstrSub;
  composeSubBlockStringToPrint(blstr, blstrSub, blNum);
  printBlock(begin, len, type, blockInfo, blstrSub);
  transformEncodeBlock(type, in, len, en, blockInfo, blstrSub, p1, p2, begin, transformOptions);
  blNum++;

  p1 = p2;
}

static void compressRecursive(File *in, uint64_t bytesToProcess, Encoder &en, String &blstr, float p1, float p2, const TransformOptions* const transformOptions, const bool isTopLevel) {

  uint64_t begin = in->curPos();

  float pscale = bytesToProcess != 0 ? (p2 - p1) / bytesToProcess : 0;

  int blNum = 0;
  while(bytesToProcess > 0 ) {

    //detect a block 
    DetectionInfo detectionInfo = detect(in, bytesToProcess, transformOptions); // Special blocktypes
    in->setpos(begin);

    //image-only gate: reject the input when the very first detection pass finds no image
    //content at all, i.e. the whole file would be compressed as one generic (default) block
    if (isTopLevel && blNum == 0 &&
      detectionInfo.Type == BlockType::DEFAULT && detectionInfo.HeaderLength == 0 &&
      detectionInfo.DataLength == 0 && detectionInfo.DataStart == begin + bytesToProcess) {
      quit("Not a supported image file. Supported formats: JPEG, BMP, PNG, GIF, TIFF, TGA, PBM/PGM/PPM/PAM, raw RGB, HDR, FAX.");
    }

    uint64_t blockStart = detectionInfo.HeaderLength != 0 ? detectionInfo.HeaderStart : detectionInfo.DataStart;
    while(blockStart != begin) {
      const uint64_t gapLength = blockStart - begin;
      compressBlock(in, begin, gapLength, /*ref: */ blNum, BlockType::DEFAULT, 0, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale, transformOptions);
      begin += gapLength;
      bytesToProcess -= gapLength;
    }

    if (detectionInfo.HeaderLength != 0) {
      compressBlock(in, detectionInfo.HeaderStart, detectionInfo.HeaderLength, /*ref: */ blNum, BlockType::HDR, 0, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale, transformOptions);
      begin += detectionInfo.HeaderLength;
      bytesToProcess -= detectionInfo.HeaderLength;
    }

    if (begin != detectionInfo.DataStart)
      quit("Internal error in compressRecursive");
    
    if (detectionInfo.DataLength != 0) {
      compressBlock(in, detectionInfo.DataStart, detectionInfo.DataLength, /*ref: */ blNum, detectionInfo.Type, detectionInfo.DataInfo, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale, transformOptions);
      begin += detectionInfo.DataLength;
      bytesToProcess -= detectionInfo.DataLength;
    }
  }
}


// Compress a file. Split fileSize bytes into blocks by type.
// For each block, output
// <type> <size> and call encode_X to convert to type X.
// Test transform and compress.
static void compressfile(const Shared* const shared, const char *filename, uint64_t fileSize, Encoder &en, bool verbose) {
  assert(en.getMode() == COMPRESS);
  assert(filename && filename[0]);

  Block::EncodeBlockSize(&en, fileSize);

  FileDisk in;
  in.open(filename, true);
  printf("Block segmentation:\n");
  String blstr;
  TransformOptions transformOptions(shared);
  // detect block types + compress
  compressRecursive(&in, fileSize, en, blstr, 0.0F, 1.0F, &transformOptions);
  in.close();
}

static uint64_t decompressRecursive(File *out, uint64_t blockSize, Encoder &en, FMode mode, TransformOptions *transformOptions) {
  uint64_t i = 0;
  uint64_t diffFound = 0;
  while( i < blockSize ) {

    uint64_t len = Block::DecodeBlockHeader(&en);
    BlockType type = en.predictorMain->shared->State.blockType;
    int info = en.predictorMain->shared->State.blockInfo;
    if( hasRecursion(type)) {
      FileTmp tmp;
      decompressRecursive(&tmp, len, en, FMode::FDECOMPRESS, transformOptions);
      if( mode != FMode::FDISCARD ) {
        tmp.setpos(0);
        if( hasTransform(type, info)) {
          len = decodeFunc(type, en, &tmp, len, info, out, mode, diffFound, transformOptions);
        }
      }
      tmp.close();
    } else if( hasTransform(type, info)) {
      len = decodeFunc(type, en, nullptr, len, info, out, mode, diffFound, transformOptions);
    } else {
      for( uint64_t j = 0; j < len; ++j ) {
        if((j & 0xfff) == 0 ) {
          en.printStatus();
        }
        if( mode == FMode::FDECOMPRESS ) {
          out->putChar(en.decompressByte(en.predictorMain));
        } else if( mode == FMode::FCOMPARE ) {
          if( en.decompressByte(en.predictorMain) != out->getchar() && (diffFound == 0)) {
            mode = FMode::FDISCARD;
            diffFound = i + j + 1;
          }
        } else {
          en.decompressByte(en.predictorMain);
        }
      }
    }
    i += len;
  }
  return diffFound;
}

// Decompress or compare a file
static void decompressFile(const Shared* const shared, const char* filename, FMode fMode, Encoder& en) {
  assert(en.getMode() == DECOMPRESS);
  assert(filename && filename[0]);

  uint64_t fileSize = Block::DecodeBlockSize(&en);

  FileDisk f;
  if( fMode == FMode::FCOMPARE ) {
    f.open(filename, true);
    printf("Comparing");
  } else { //mode==FDECOMPRESS;
    f.create(filename);
    printf("Extracting");
  }
  printf(" %s %" PRIu64 " bytes -> ", filename, fileSize);

  // Decompress/Compare
  TransformOptions transformOptions(shared);
  uint64_t r = decompressRecursive(&f, fileSize, en, fMode, &transformOptions);
  if( fMode == FMode::FCOMPARE && (r == 0) && f.getchar() != EOF) {
    printf("file is longer\n");
  } else if( fMode == FMode::FCOMPARE && (r != 0)) {
    printf("differ at %" PRIu64 "\n", r - 1);
  } else if( fMode == FMode::FCOMPARE ) {
    printf("identical\n");
  } else {
    printf("done   \n");
  }
  f.close();
}