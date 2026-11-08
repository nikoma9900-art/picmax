/*
  paqimg – Specialized Lossless Image Compressor (based on PAQ8PX)
  see README.md for information
*/

//////////////////////// Versioning ////////////////////////////////////////

#define PROGNAME     "paqimg"
#define PROGVERSION  "1"  //update version here before publishing your changes
#define PROGYEAR     "2026"


#include "Utils.hpp"

#include <stdexcept>  //std::exception
#include <string> //std::stof, std::to_string

#include "Encoder.hpp"
#include "ProgramChecker.hpp"
#include "Shared.hpp"
#include "String.hpp"
#include "file/FileName.hpp"
#include "file/fileUtils2.hpp"
#include "filter/Filters.hpp"
#include "Models.hpp"
#include "Simd.hpp"
#include "PredictorBlock.hpp"
#include "PredictorMain.hpp"
#include "PredictorMainLstmOnly.hpp"


typedef enum { DoNone, DoCompress, DoExtract, DoCompare } WHATTODO;

static void printHelp() {
  printf(
    "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
    "Usage:\n"
    "  to compress       ->   " PROGNAME " -LEVEL[FLAGS] [OPTIONS] INPUT [OUTPUT]\n"
    "  to decompress     ->   " PROGNAME " -d INPUT." PROGNAME PROGVERSION " [OUTPUT]\n"
    "  to test           ->   " PROGNAME " -t INPUT." PROGNAME PROGVERSION " [OUTPUT]\n"
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
    "  -0L                  | Segment and transform then LSTM-only compression (alternative: -lstmonly)\n"
    "\n"
    "FLAGS:\n"
    "  L                    | Enable LSTM model (+24 MB per block type)\n"
    "  S                    | Skip RGB color transform (images)\n"
    "\n"
    "  Example: " PROGNAME " -8L image.png   <- Level 8 + LSTM\n"
    "\n"
    "LSTM-specific options:\n"
    "  -lstmlayers=N        | Set the number of LSTM layers to N (1..5, default is 2)\n"
    "  -savelstm FILE       | Save learned LSTM model weights after compression\n"
    "  -loadlstm FILE       | Load LSTM model weights before compression/decompression\n"
    "\n"
    "Misc options:\n"
    "  -v                   | Verbose output\n"
    "  -log FILE            | Append compression results with runtime to log file\n"
    "  -simd MODE           | Override SIMD detection - expert only (NONE|SSE2|SSE3|SSE41|AVX2|AVX512|NEON)\n"
    "\n"
    "Notes:\n"
    "  INPUT is an image FILE or PATH/FILE\n"
    "  OUTPUT is optional: FILE, PATH, PATH/FILE\n"
    "  The archive is created in the current folder with ." PROGNAME PROGVERSION " extension if OUTPUT omitted\n"
    "  FLAGS are case-insensitive and only needed for compression; they may appear in any order\n"
    "  INPUT must precede OUTPUT; all other OPTIONS may appear anywhere\n"
  );
}

static void printHelpVerbose() {
  printf(
    "\n"
    "Detailed help:\n"
    "\n"
    "Usage:\n"
    "  paqimg -LEVEL[FLAGS] [OPTIONS] INPUT [OUTPUT]   : compress INPUT to archive\n"
    "  paqimg -d INPUT.paqimg1 [OUTPUT]                 : decompress archive to OUTPUT\n"
    "  paqimg -t INPUT.paqimg1 [OUTPUT]                 : compare archive with OUTPUT\n"
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
    "  L : enable the LSTM (Long Short-Term Memory) neural network model\n"
    "  S : skip the RGB color transform, just reorder the RGB channels\n"
    "\n"
    "Examples:\n"
    "  paqimg -5 photo.bmp\n"
    "  paqimg -8L picture.png out.paqimg1\n"
    "  paqimg -d out.paqimg1 restored.png\n"
    "  paqimg -t out.paqimg1 original.png\n"
    "\n"
    "=============================================================================\n"
  );
}

static void printModules() {
  printf("\n");
  printf("Build: ");
#ifndef DISABLE_ZLIB
  printf("ZLIB: ENABLED, ");
#else
  printf("ZLIB: DISABLED, ");
#endif

  printf("\n");
}

static void printSimdInfo(int simdIset, int detectedSimdIset) {
  printf("\nHighest SIMD vectorization support on this system: ");
  if( detectedSimdIset < 0 || detectedSimdIset > 11 ) {
    quit("Oops, sorry. Unexpected result.");
  }
  static const char *vectorizationString[12] = {"none", "MMX", "SSE", "SSE2", "SSE3", "SSSE3", "SSE4.1", "SSE4.2", "AVX", "AVX2", "AVX512", "ARM Neon"};
  printf("%s.\n", vectorizationString[detectedSimdIset]);

  printf("Using ");
  if (simdIset == 11) {
    printf("NEON");
  } else if( simdIset >= 10 ) {
    printf("AVX512");
  } else if( simdIset >= 9 ) {
    printf("AVX2");
  } else if( simdIset >= 6 ) {
    printf("SSE3");
  } else if( simdIset >= 4 ) {
    printf("SSE3");
  } else if( simdIset >= 3 ) {
    printf("SSE2");
  } else {
    printf("non-vectorized");
  }
  printf(" neural network functions.\n");
}

static void printCommand(const WHATTODO &whattodo) {
  printf(" To do          = ");
  if( whattodo == DoNone ) {
    printf("-");
  }
  if( whattodo == DoCompress ) {
    printf("Compress");
  }
  if( whattodo == DoExtract ) {
    printf("Extract");
  }
  if( whattodo == DoCompare ) {
    printf("Compare");
  }
  printf("\n");
}

static void printOptions(Shared *shared, int level) {
  printf(" Level          = %d\n", level);
  printf(" Skip RGB   (s) = %s\n", shared->GetOptionSkipRGB() ? "On  (Skip the color transform, just reorder the RGB channels)" : "Off");
  printf(" Use LSTM   (l) = %s\n", shared->GetOptionUseLSTM() ? "On  (Use LSTM (Long Short-Term Memory) model)" : "Off");
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
    bool verbose = false;
    int simdIset = -1; //simd instruction set to use
    bool lstmOnly = false;
    int level = 0;
    
    FileName input;
    FileName output;
    FileName inputPath;
    FileName outputPath;
    FileName archiveName;
    FileName logfile;
    FileName lstmLoadFilename;
    FileName lstmSaveFilename;

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
              case 'L':
                shared.SetOptionUseLSTM();
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
        } else if( strcasecmp(argv[i], "-t") == 0 ) {
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          whattodo = DoCompare;
        } else if( strcasecmp(argv[i], "-v") == 0 ) {
          verbose = true;
        } else if( strcasecmp(argv[i], "-log") == 0 ) {
          if( logfile.strsize() != 0 ) {
            quit("Only one logfile may be specified.");
          }
          if( ++i == argc ) {
            quit("The -log switch requires a filename.");
          }
          logfile += argv[i];
        } else if (strcasecmp(argv[i], "-loadlstm") == 0) {
          if (lstmLoadFilename.strsize() != 0) {
            quit("Only one LSTM load file may be specified.");
          }
          if (++i == argc) {
            quit("The -loadlstm switch requires a filename.");
          }
          lstmLoadFilename += argv[i];
        } else if (strcasecmp(argv[i], "-lstmonly") == 0) {
          lstmOnly = true;
          shared.SetOptionUseLSTM();
        } else if (strcasecmp(argv[i], "-savelstm") == 0) {
          if (lstmSaveFilename.strsize() != 0) {
            quit("Only one LSTM save file may be specified.");
          }
          if (++i == argc) {
            quit("The -savelstm switch requires a filename.");
          }
          lstmSaveFilename += argv[i];
        }
        else if ( strncmp(argv[i], "-param=", 7) == 0 )
        {
          try
          {
            shared.tuning_param = std::stof(argv[i] + 7);
          }
          catch (...)
          {
            quit("Could not parse tuning param.");
          }
        }
        else if (strncmp(argv[i], "-lstmlayers=", 12) == 0)
        {
          try
          {
            shared.LstmSettings.num_layers = std::stoi(argv[i] + 12);
          }
          catch (...)
          {
            quit("Could not parse lstmlayers parameter.");
          }
          if (shared.LstmSettings.num_layers < 1 || shared.LstmSettings.num_layers > 5)
            quit("Illegal number of lstmlayers. Should be between 1 and 5.");
        }
        else if( strcasecmp(argv[i], "-simd") == 0 ) {
          if( ++i == argc ) {
            quit("The -simd switch requires an instruction set name (NONE, SSE2, AVX2, AVX512, NEON).");
          }
          if( strcasecmp(argv[i], "NONE") == 0 ) {
            simdIset = 0;
          } else if( strcasecmp(argv[i], "SSE2") == 0 ) {
            simdIset = 3;
          } else if( strcasecmp(argv[i], "SSE3") == 0 ) {
            simdIset = 4;
          } else if( strcasecmp(argv[i], "SSE41") == 0 ) {
            simdIset = 6;
          } else if( strcasecmp(argv[i], "AVX2") == 0 ) {
            simdIset = 9;
          } else if( strcasecmp(argv[i], "AVX512") == 0 ) {
            simdIset = 10;
          } else if (strcasecmp(argv[i], "NEON") == 0) {
            simdIset = 11;
          } else {
            quit("Invalid -simd option. Use -simd NONE, -simd SSE2, -simd SSE3, -simd SSE41, -simd AVX2, -simd AVX512 or -simd NEON.");
          }
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

    if( verbose ) {
      printModules();
    }

    // Determine CPU's (and OS) support for SIMD vectorization instruction set
    int detectedSimdIset = simdDetect();
    if( simdIset == -1 ) {
      simdIset = detectedSimdIset;
    }

    // Print anything only if the user wants/needs to know
    if( verbose || simdIset != detectedSimdIset ) {
      printSimdInfo(simdIset, detectedSimdIset);
    }

    // Set highest or user selected vectorization mode
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

    if (simdIset > detectedSimdIset) {
      printf("\nOverriding system highest vectorization support. Expect a crash.");
    }

    // Successfully parsed command line arguments
    // Let's check their validity

    if (lstmOnly && whattodo == DoNone) {
      whattodo = DoCompress;
    }

    if( whattodo == DoNone ) {
      quit("A command switch is required: -0..-12 to compress, -d to decompress, -t to test.");
    }
    if( input.strsize() == 0 ) {
      printf("\nAn %s is required %s.\n", whattodo == DoCompress ? "input image file" : "archive filename",
             whattodo == DoCompress ? "for compressing" : whattodo == DoExtract ? "for decompressing" : whattodo == DoCompare
                                                                                                        ? "for testing"
                                                                                                          : "");
      quit();
    }


    int pathType = 0;

    //Logfile supplied?
    if( logfile.strsize() != 0 ) {
      if( whattodo != DoCompress ) {
        quit("A log file may only be specified for compression.");
      }
      pathType = examinePath(logfile.c_str());
      if( pathType == 2 || pathType == 4 ) {
        quit("Specified log file should be a file, not a directory.");
      }
      if( pathType == 0 ) {
        printf("\nThere is a problem with the log file: %s", logfile.c_str());
        quit();
      }
    }

    // Validate LSTM parameters
    if (lstmLoadFilename.strsize() != 0 && !shared.GetOptionUseLSTM()) {
      quit("The -loadlstm switch requires the -L option to be set.");
    }
    if (lstmSaveFilename.strsize() != 0 && !shared.GetOptionUseLSTM()) {
      quit("The -savelstm switch requires the -L option to be set.");
    }
    if (shared.LstmSettings.num_layers > 0 && !shared.GetOptionUseLSTM()) {
      quit("Number of LSTM layers specified without enabling the LSTM model. Enable the LSTM model.");
    }
    if (shared.GetOptionUseLSTM() && shared.LstmSettings.num_layers == 0) {
      shared.LstmSettings.num_layers = 2; // default
    }

    if (verbose && shared.GetOptionUseLSTM()) {
      printf("Number of trainable parameters in LSTM model: %d\n", LstmModelContainer::GetNumberOfTrainableParameters(&shared));
    }

    // Print options in verbose mode
    if (verbose) {
      printf("\n");
      printf(" Command line   =");
      for (int i = 0; i < argc; i++) {
        printf(" %s", argv[i]);
      }
      printf("\n");
    }


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
    } else { // extract/compare/list: archivename is simply the input
      archiveName += inputPath.c_str();
      archiveName += input.c_str();
    }
    if( verbose ) {
      printf(" Archive        = %s\n", archiveName.c_str());
      printf(" Input folder   = %s\n", inputPath.strsize() == 0 ? "." : inputPath.c_str());
      printf(" Output folder  = %s\n", outputPath.strsize() == 0 ? "." : outputPath.c_str());
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
      if (shared.GetOptionUseLSTM()) {
        c = archive.getchar(); // lstmlayers
        shared.LstmSettings.num_layers = c;
        if (c < 1 || c > 5) {
          quit("Unexpected lstm layer settings in archive");
        }
      }
      if (c == EOF) {
        quit("Unexpected end of archive file.");
      }
    }

    // Load LSTM model if requested
    if (lstmLoadFilename.strsize() != 0) {
      printf("Loading LSTM model from %s", lstmLoadFilename.c_str());
      FILE* lstmFile = fopen(lstmLoadFilename.c_str(), "rb");
      if (!lstmFile) {
        printf("\nError: Cannot open LSTM model file: %s\n", lstmLoadFilename.c_str());
        quit();
      }
      Models models = Models(&shared, nullptr);
      LstmModelContainer& lstmModel = models.lstmModelGeneric();
      lstmModel.LoadModelParameters(lstmFile);
      fclose(lstmFile);
      printf(" ... Done.\n");
    }

    if( verbose ) {
      printCommand(whattodo);
      printOptions(&shared, level);
    }
    printf("\n");


    // Write archive header to archive file
    if( mode == COMPRESS ) {
      printf("Creating archive %s...\n", archiveName.c_str());
      archive.create(archiveName.c_str());
      archive.append(PROGNAME);
      archive.putChar(level);
      archive.putChar(shared.options);
      if(shared.GetOptionUseLSTM())
        archive.putChar(shared.LstmSettings.num_layers);
    }

    // In single file mode with no output filename specified we must construct it from the supplied archive filename
      if((whattodo == DoExtract || whattodo == DoCompare) && output.strsize() == 0 ) {
        output += input.c_str();
        const char *fileExtension = "." PROGNAME PROGVERSION;
        if( output.endsWith(fileExtension)) {
          output.stripEnd(static_cast<int>(strlen(fileExtension)));
        } else {
          printf("Can't construct output filename from archive filename.\nArchive file extension must be: '%s'", fileExtension);
          quit();
        }
      }

    if (lstmOnly && level > 0) {
      quit("The -lstmonly option is not compatible with specifying a compression level.");
    }

    shared.init(level);

    bool useCm = level != 0;
    bool useLstm = shared.GetOptionUseLSTM();
    bool doEncoding = useCm || useLstm;

    Shared sharedForBlockPredictor;
    std::unique_ptr<Predictor> predictorBlock = nullptr;
    std::unique_ptr<Predictor> predictorMain = nullptr;

    if (doEncoding) {
      sharedForBlockPredictor.init(level, 16);
      sharedForBlockPredictor.chosenSimd = shared.chosenSimd;
      predictorBlock = std::make_unique<PredictorBlock>(&sharedForBlockPredictor);
      if (level == 0 && useLstm) {
        predictorMain = std::make_unique<PredictorMainLstmOnly>(&shared);
      }
      else {
        predictorMain = std::make_unique<PredictorMain>(&shared);
      }
    }

    Encoder en(predictorBlock.get(), predictorMain.get(), doEncoding, mode, &archive);
    uint64_t contentSize = 0;
    uint64_t totalSize = 0;

    // Compress list of files
    if( mode == COMPRESS ) {
      uint64_t start = en.size(); //header size (=8)
      if( verbose ) {
        printf("Writing header : %" PRIu64 " bytes\n", start);
      }
      totalSize += start;


    }

    // Compress or decompress files
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
      compressfile(&shared, fName, fSize, en, verbose);
      totalSize += fSize + 4; //4: file size information
      contentSize += fSize;

    uint64_t preFlush = en.size();
    en.flush();
      totalSize += en.size() - preFlush; //we consider padding bytes as auxiliary bytes
      printf("-----------------------\n");
      printf("Total input size     : %" PRIu64 "\n", contentSize);
      if( verbose ) {
        printf("Total metadata bytes : %" PRIu64 "\n", totalSize - contentSize);
      }
      printf("Total archive size   : %" PRIu64 "\n", en.size());
      printf("\n");
      // Log compression results
      if( logfile.strsize() != 0 ) {
        bool showParam = shared.tuning_param != 0.0f;
        String results;
        pathType = examinePath(logfile.c_str());
        //Write header if needed
        if( pathType == 3 /*does not exist*/ ||
            (pathType == 1 && getFileSize(logfile.c_str()) == 0)/*exists but does not contain a header*/) {
          results += "PROG_NAME\tPROG_VERSION\tCOMMAND_LINE\tLEVEL\t";
            if (showParam)
              results += "PARAM\t";
          results +=  "INPUT_FILENAME\tORIGINAL_SIZE_BYTES\tCOMPRESSED_SIZE_BYTES\tRUNTIME_MS\n";
        }
        //Write results to logfile
        results += PROGNAME "\t" PROGVERSION "\t";
        for( int i = 1; i < argc; i++ ) {
          if( i != 0 ) {
            results += ' ';
          }
          results += argv[i];
        }
        results += "\t";
        results += uint64_t(shared.level);
        results += "\t";
        if (showParam) {
          results += std::to_string(shared.tuning_param).c_str();
          results += "\t";
        }
        results += input.c_str();
        results += "\t";
        results += contentSize;
        results += "\t";
        results += en.size();
        results += "\t";
        results += uint64_t(programChecker->getRuntime() * 1000.0);
        results += "\t";
        results += "\n";
        appendToFile(logfile.c_str(), results.c_str());
        printf("Results logged to file '%s'\n", logfile.c_str());
        printf("\n");
      }
    } else { //decompress
      if( whattodo == DoExtract || whattodo == DoCompare ) {
        FMode fMode = whattodo == DoExtract ? FMode::FDECOMPRESS : FMode::FCOMPARE;
        FileName fn;
        fn += outputPath.c_str();
        fn += output.c_str();
        const char *fName = fn.c_str();
        decompressFile(&shared, fName, fMode, en);
      }
    }

    archive.close();

    // Save LSTM model if requested
    if (lstmSaveFilename.strsize() != 0) {
      printf("\nSaving LSTM model parameters to %s", lstmSaveFilename.c_str());
      FILE* lstmFile = fopen(lstmSaveFilename.c_str(), "wb");
      if (!lstmFile) {
        printf("\nError: Cannot create LSTM model file: %s\n", lstmSaveFilename.c_str());
        quit();
      }
      Models models = Models(&shared, nullptr);
      LstmModelContainer& lstmModel = models.lstmModelGeneric();
      lstmModel.SaveModelParameters(lstmFile);
      fclose(lstmFile);
      printf(" ... Done.\n");
    }

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
      wchar_t *s=szArglist[i];
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

