/*
  paqimg – Specialized Lossless Image Compressor (based on PAQ8PX)
  see README.md for information
*/

//////////////////////// Versioning ////////////////////////////////////////

#define PROGNAME     "paqimg"
#define PROGVERSION  "1"  //update version here before publishing your changes
#define PROGYEAR     "2026"


#include "Utils.hpp"

#include <memory>
#include <stdexcept>  //std::exception

#include "Encoder.hpp"
#include "ProgramChecker.hpp"
#include "Shared.hpp"
#include "file/FileName.hpp"
#include "file/fileUtils2.hpp"
#include "filter/Filters.hpp"
#include "Simd.hpp"
#include "PredictorBlock.hpp"
#include "PredictorMain.hpp"


typedef enum { DoNone, DoCompress, DoExtract } WHATTODO;

static void printHelp() {
  printf(
    "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
    "Usage:\n"
    "  to compress       ->   " PROGNAME " -LEVEL[FLAGS] INPUT [OUTPUT]\n"
    "  to decompress     ->   " PROGNAME " -d INPUT." PROGNAME PROGVERSION " [OUTPUT]\n"
    "\n"
    "paqimg is a specialized lossless image compressor: it supports\n"
    "BMP, PBM/PGM/PPM, PGM (RAW), JPEG, HDR/RGBE, GIF, PNG and TIFF images.\n"
    "Other file types are rejected.\n"
    "\n"
    "LEVEL:\n"
    "  -1 -2 -3 -4          | Compress using less memory (583, 597, 626, 685 MB)\n"
    "  -5 -6 -7 -8          | Use more memory (801, 1034, 1501, 2433 MB)\n"
    "  -9 -10 -11 -12       | Use even more memory (4298, 7772, 14719, 24583 MB)\n"
    "  -0                   | Segment and transform only, no compression\n"
    "\n"
    "FLAGS:\n"
    "  S                    | Skip RGB color transform (images)\n"
    "\n"
    "  Example: " PROGNAME " -8 image.png\n"
    "\n"
    "Notes:\n"
    "  INPUT is an image FILE or PATH/FILE\n"
    "  OUTPUT is optional: FILE, PATH, PATH/FILE\n"
    "  The archive is created in the current folder with ." PROGNAME PROGVERSION " extension if OUTPUT omitted\n"
    "  FLAGS are case-insensitive and only needed for compression; they may appear in any order\n"
    "  INPUT must precede OUTPUT\n"
  );
}

static void printHelpVerbose() {
  printf(
    "\n"
    "Detailed help:\n"
    "\n"
    "Usage:\n"
    "  paqimg -LEVEL[FLAGS] INPUT [OUTPUT]   : compress INPUT to archive\n"
    "  paqimg -d INPUT.paqimg1 [OUTPUT]      : decompress archive to OUTPUT\n"
    "\n"
    "Supported image formats:\n"
    "  BMP, PBM/PGM/PPM (ASCII and binary), PGM (RAW), JPEG (progressive/baseline, lossless),\n"
    "  HDR/RGBE, GIF (87a/89a), PNG (all bit depths, interlaced), TIFF (uncompressed/packbits/LZW/deflate)\n"
    "  Internal transforms are applied to these images before the actual compression.\n"
    "  Any other input file type is rejected with an error message.\n"
    "\n"
    "Compression levels:\n"
    "  -0   : transform only, no compression\n"
    "  -1..-4  : fastest, lower memory usage\n"
    "  -5..-8  : good ratio\n"
    "  -9..-12 : best ratio, high memory usage\n"
    "\n"
    "Compression flags (case-insensitive):\n"
    "  S : skip the RGB color transform, just reorder the RGB channels\n"
    "\n"
    "Examples:\n"
    "  paqimg -5 photo.bmp\n"
    "  paqimg -8 picture.png out.paqimg1\n"
    "  paqimg -d out.paqimg1 restored.png\n"
    "\n"
    "=============================================================================\n"
  );
}

int processCommandLine(int argc, char **argv) {
  ProgramChecker *programChecker = ProgramChecker::getInstance();
  Shared shared;
  try {

    if( !shared.toScreen ) { //we need a minimal feedback when redirected
      fprintf(stderr, PROGNAME " data compressor v" PROGVERSION " (c) " PROGYEAR ", Matt Mahoney et al.\n");
    }
    printf(PROGNAME " v" PROGVERSION " (c) " PROGYEAR ", Matt Mahoney et al.\n");

    // Print help message
    if( argc < 2 ) {
      printHelp();
      printf(
        "\n"
        "For detailed help with examples and more explanation, use:\n"
        "  " PROGNAME " -help\n"
      );
      quit();
    }

    if ( strcasecmp(argv[1], "-help") == 0 ) {
      printHelp();
      printHelpVerbose();
      quit();
    }


    // Parse command line arguments
    WHATTODO whattodo = DoNone;
    int level = 0;

    FileName input;
    FileName output;
    FileName inputPath;
    FileName outputPath;
    FileName archiveName;

    for( int i = 1; i < argc; i++ ) {
      int argLen = static_cast<int>(strlen(argv[i]));
      if( argv[i][0] == '-' ) {
        if( argLen == 1 ) {
          quit("Empty command.");
        }
        if( argv[i][1] >= '0' && argv[i][1] <= '9' ) { // first  digit of level
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          level = argv[i][1] - '0';
          int j = 2;
          if (argLen >= 3 && argv[i][2] >= '0' && argv[i][2] <= '9') { // second digit of level
            level = level*10 + argv[i][2] - '0';
            j++;
          }
          if (level > 12) {
            quit("Compression level must be between 0 and 12.");
          }
          whattodo = DoCompress;
          //process optional compression switches
          for( ; j < argLen; j++ ) {
            switch( argv[i][j] & 0xDF ) {
              case 'S':
                shared.SetOptionSkipRGB();
                break;
              default: {
                printf("Invalid compression switch: %c", argv[1][j]);
                quit();
              }
            }
          }
        } else if( strcasecmp(argv[i], "-d") == 0 ) {
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          whattodo = DoExtract;
        } else {
          printf("Invalid command: %s", argv[i]);
          quit();
        }
      } else { //this parameter does not begin with a dash ("-") -> it must be a folder/filename
        if( input.strsize() == 0 ) {
          input += argv[i];
          input.replaceSlashes();
        } else if( output.strsize() == 0 ) {
          output += argv[i];
          output.replaceSlashes();
        } else {
          quit("More than two file names specified. Only an input and an output is needed.");
        }
      }
    }

    // Determine CPU's (and OS) support for SIMD vectorization instruction set
    int simdIset = simdDetect();

    // Set highest available vectorization mode
    if (simdIset == 11) {
      shared.chosenSimd = SIMDType::SIMD_NEON;
    } else if (simdIset >= 10) {
      shared.chosenSimd = SIMDType::SIMD_AVX512;
    } else if (simdIset >= 9) {
      shared.chosenSimd = SIMDType::SIMD_AVX2;
    } else if (simdIset >= 6) {
      shared.chosenSimd = SIMDType::SIMD_SSE41;
    } else if (simdIset >= 4) {
      shared.chosenSimd = SIMDType::SIMD_SSE3;
    } else if( simdIset >= 3 ) {
      shared.chosenSimd = SIMDType::SIMD_SSE2;
    } else {
      shared.chosenSimd = SIMDType::SIMD_NONE;
    }

    if (!IS_ARM_NEON_AVAILABLE && shared.chosenSimd == SIMDType::SIMD_NEON) {
      quit("The ARM Neon instruction set is not available on this platform.");
    }
    if (!IS_X64_SIMD_AVAILABLE && (
      shared.chosenSimd == SIMDType::SIMD_SSE2 ||
      shared.chosenSimd == SIMDType::SIMD_SSE3 ||
      shared.chosenSimd == SIMDType::SIMD_SSE41 ||
      shared.chosenSimd == SIMDType::SIMD_AVX2 ||
      shared.chosenSimd == SIMDType::SIMD_AVX512
      )) {
      quit("The x64 SIMD instruction set is not available on this platform.");
    }

    // Successfully parsed command line arguments
    // Let's check their validity

    if( whattodo == DoNone ) {
      quit("A command switch is required: -0..-12 to compress, -d to decompress.");
    }
    if( input.strsize() == 0 ) {
      printf("\nAn %s is required %s.\n", whattodo == DoCompress ? "input image file" : "archive filename",
             whattodo == DoCompress ? "for compressing" : "for decompressing");
      quit();
    }


    int pathType = 0;

    // Separate paths from input filename/directory name
    pathType = examinePath(input.c_str());
    if( pathType == 2 || pathType == 4 ) {
      printf("\nSpecified input is a directory but should be a file: %s", input.c_str());
      quit();
    }
    if( pathType == 3 ) {
      printf("\nSpecified input file does not exist: %s", input.c_str());
      quit();
    }
    if( pathType == 0 ) {
      printf("\nThere is a problem with the specified input file: %s", input.c_str());
      quit();
    }
    if( input.lastSlashPos() >= 0 ) {
      inputPath += input.c_str();
      inputPath.keepPath();
      input.keepFilename();
    }

    // Separate paths from output filename/directory name
    if( output.strsize() > 0 ) {
      pathType = examinePath(output.c_str());
      if( pathType == 1 || pathType == 3 ) { //is an existing file, or looks like a file
        if( output.lastSlashPos() >= 0 ) {
          outputPath += output.c_str();
          outputPath.keepPath();
          output.keepFilename();
        }
      } else if( pathType == 2 || pathType == 4 ) {//is an existing directory, or looks like a directory
        outputPath += output.c_str();
        if( !outputPath.endsWith("/") && !outputPath.endsWith("\\")) {
          outputPath += GOODSLASH;
        }
        //output file is not specified
        output.resize(0);
        output.pushBack(0);
      } else {
        printf("\nThere is a problem with the specified output: %s", output.c_str());
        quit();
      }
    }

    //determine archive name
    if( whattodo == DoCompress ) {
      archiveName += outputPath.c_str();
      if( output.strsize() == 0 ) { // If no archive name is provided, construct it from input (append PROGNAME extension to input filename)
        archiveName += input.c_str();
        archiveName += "." PROGNAME PROGVERSION;
      } else {
        archiveName += output.c_str();
      }
    } else { // extract: archivename is simply the input
      archiveName += inputPath.c_str();
      archiveName += input.c_str();
    }

    Mode mode = whattodo == DoCompress ? COMPRESS : DECOMPRESS;

    FileName fn(inputPath.c_str());
    fn += input.c_str();
    getFileSize(fn.c_str()); // Does file exist? Is it readable? (we don't actually need the file size now)

    FileDisk archive;  // compressed file

    if( mode == DECOMPRESS ) {
      archive.open(archiveName.c_str(), true);
      // Verify archive header, get level and options
      int len = static_cast<int>(strlen(PROGNAME));
      for( int i = 0; i < len; i++ ) {
        if( archive.getchar() != PROGNAME[i] ) {
          printf("%s: not a valid %s file.", archiveName.c_str(), PROGNAME);
          quit();
        }
      }
      
      c = archive.getchar(); // level
      level = c;
      c = archive.getchar(); // options
      shared.options = static_cast<uint8_t>(c);

      if (level > 12) {
        quit("Unexpected compression level setting in archive");
      }
      if (c == EOF) {
        quit("Unexpected end of archive file.");
      }
    }

    printf("\n");

    // Write archive header to archive file
    if( mode == COMPRESS ) {
      printf("Creating archive %s...\n", archiveName.c_str());
      archive.create(archiveName.c_str());
      archive.append(PROGNAME);
      archive.putChar(level);
      archive.putChar(shared.options);
    }

    // In single file mode with no output filename specified we must construct it from the supplied archive filename
    if( whattodo == DoExtract && output.strsize() == 0 ) {
        output += input.c_str();
        const char *fileExtension = "." PROGNAME PROGVERSION;
        if( output.endsWith(fileExtension)) {
          output.stripEnd(static_cast<int>(strlen(fileExtension)));
        } else {
          printf("Can't construct output filename from archive filename.\nArchive file extension must be: '%s'", fileExtension);
          quit();
        }
    }

    shared.init(level);

    bool doEncoding = level != 0;

    Shared sharedForBlockPredictor;
    std::unique_ptr<Predictor> predictorBlock = nullptr;
    std::unique_ptr<Predictor> predictorMain = nullptr;

    if (doEncoding) {
      sharedForBlockPredictor.init(level, 16);
      sharedForBlockPredictor.chosenSimd = shared.chosenSimd;
      predictorBlock = std::make_unique<PredictorBlock>(&sharedForBlockPredictor);
      predictorMain = std::make_unique<PredictorMain>(&shared);
    }

    Encoder en(predictorBlock.get(), predictorMain.get(), doEncoding, mode, &archive);
    uint64_t contentSize = 0;

    // Compress or decompress the file
    if( mode == COMPRESS ) {
      if( !shared.toScreen ) { //we need a minimal feedback when redirected
        fprintf(stderr, "Output is redirected - only minimal feedback is on screen\n");
      }
      FileName fn;
      fn += inputPath.c_str();
      fn += input.c_str();
      const char *fName = fn.c_str();
      uint64_t fSize = getFileSize(fName);
      if( !shared.toScreen ) { //we need a minimal feedback when redirected
        fprintf(stderr, "\nFilename: %s (%" PRIu64 " bytes)\n", fName, fSize);
      }
      printf("\nFilename: %s (%" PRIu64 " bytes)\n", fName, fSize);
      compressfile(&shared, fName, fSize, en, false);
      contentSize += fSize;

      en.flush();
      printf("-----------------------\n");
      printf("Total input size     : %" PRIu64 "\n", contentSize);
      printf("Total archive size   : %" PRIu64 "\n", en.size());
      printf("\n");
    } else { //decompress
      FileName fn;
      fn += outputPath.c_str();
      fn += output.c_str();
      const char *fName = fn.c_str();
      decompressFile(&shared, fName, FMode::FDECOMPRESS, en);
    }

    archive.close();

    programChecker->print();
  }
    // we catch only the intentional exceptions from quit() to exit gracefully
    // any other exception should result in a crash and must be investigated
  catch( IntentionalException const & ) {
  }

  return 0;
}

#ifdef WINDOWS
#include "shellapi.h"
#pragma comment(lib,"shell32.lib")
#endif

int main(int argc, char **argv) {
#ifdef WINDOWS
  // On Windows, argv is encoded in the effective codepage, therefore unsuitable for acquiring command line arguments (file names
  // in our case) not representable in that particular codepage.
  // -> We will recreate argv as UTF8 (like in Linux)
  uint32_t oldcp = GetConsoleOutputCP();
  SetConsoleOutputCP(CP_UTF8);
  wchar_t **szArglist;
  int argc_utf8;
  char** argv_utf8;
  if( (szArglist = CommandLineToArgvW(GetCommandLineW(), &argc_utf8)) == NULL) {
    printf("CommandLineToArgvW failed\n");
    return 0;
  } else {
    if(argc!=argc_utf8)quit("Error preparing command line arguments.");
    argv_utf8=new char*[argc_utf8+1];
    for(int i=0; i<argc_utf8; i++) {
      wchar_t *s=sArglist[i];
      int buffersize = WideCharToMultiByte(CP_UTF8,0,s,-1,NULL,0,NULL,NULL);
      argv_utf8[i] = new char[buffersize];
      WideCharToMultiByte(CP_UTF8,0,s,-1,argv_utf8[i],buffersize,NULL,NULL);
      //printf("%d: %s\n", i, argv_utf8[i]); //debug: see if conversion is successful
    }
    argv_utf8[argc_utf8]=nullptr;
    int retval=processCommandLine(argc_utf8, argv_utf8);
    for(int i=0; i<argc_utf8; i++)
      delete[] argv_utf8[i];
    delete[] argv_utf8;
    SetConsoleOutputCP(oldcp);
    return retval;
  }
#else
  return processCommandLine(argc, argv);
#endif
}
