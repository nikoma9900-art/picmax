// paqimg - image-only compressor, single-file amalgamation
// Generated from the paqimg sources (paq8px lineage). Do not edit sections directly.

// ===== src/SystemDefines.hpp =====

static_assert(sizeof(short) == 2, "sizeof(short)");
static_assert(sizeof(int) == 4, "sizeof(int)");

//////////////////////// Target OS/Compiler ////////////////////////////////

#if defined(_WIN32) || defined(_MSC_VER)
#ifndef WINDOWS
#define WINDOWS  //to compile for Windows
#endif
#endif

#if defined(unix) || defined(__unix__) || defined(__unix) || defined(__APPLE__)
#ifndef UNIX
#define UNIX //to compile for Unix, Linux, Solaris, MacOS / Darwin, etc)
#endif
#endif

#if !defined(WINDOWS) && !defined(UNIX)
#error Unknown target system
#endif

#if defined(__ARM_NEON)
#define ARM_NEON_AVAILABLE
#include <arm_neon.h>
constexpr bool IS_ARM_NEON_AVAILABLE = true;
#else
constexpr bool IS_ARM_NEON_AVAILABLE = false;
#endif

#if defined(__x86_64__) || defined(_M_X64)
#define X64_SIMD_AVAILABLE
#include <immintrin.h>
constexpr bool IS_X64_SIMD_AVAILABLE = true;
#else
constexpr bool IS_X64_SIMD_AVAILABLE = false;
#endif


// Floating point operations need IEEE compliance
// Do not use unsafe compiler optimization options
// gcc : -ffast-math (and -Ofast, -funsafe-math-optimizations, -fno-rounding-math) => __FAST_MATH__
// MSVC: /fp:fast => _M_FP_FAST
#if defined(__FAST_MATH__) || defined(_M_FP_FAST)
#error Avoid using aggressive floating-point compiler optimization flags
#endif

// MSVC: Verify we're in a safe floating-point mode
#if defined(_MSC_VER)
#if !defined(_M_FP_PRECISE) && !defined(_M_FP_STRICT)
#warning MSVC floating-point mode unclear. Ensure /fp:precise or /fp:strict is set
#endif
#endif


#if defined(_MSC_VER)
#define ALWAYS_INLINE  __forceinline
#elif (defined(__GNUC__) || defined(__clang__))
#define ALWAYS_INLINE inline __attribute__((always_inline))
#else
#define ALWAYS_INLINE inline
#endif

#include <cassert>

#if defined(__clang__)
#define ASSUME(cond) do { assert(cond); __builtin_assume(cond); } while (0)
#elif defined(_MSC_VER)
#define ASSUME(cond) do { assert(cond); __assume(cond); } while (0)
#elif defined(__GNUC__)
#define ASSUME(cond) do { assert(cond); if (!(cond)) __builtin_unreachable(); } while (0)
#else
#define ASSUME(cond) assert(cond)
#endif

#include <algorithm>
#include <cstdio>
// Determining the proper printf() format specifier for 64 bit unsigned integers:
// - on Windows MSVC and MinGW-w64 use the MSVCRT runtime where it is "%I64u"
// - on Linux it is "%llu"
// The correct value is provided by the PRIu64 macro which is defined here:
#include <cinttypes> //PRIu64

// Platform-specific includes
#ifdef UNIX

#include <cerrno>  //errno
#include <climits> //PATH_MAX (for OSX)
#include <cstring> //strlen(), strcpy(), strcat(), strerror(), memset(), memcpy(), memmove()
#include <unistd.h> //isatty()

#else
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>   //CreateDirectoryW, CommandLineToArgvW, GetConsoleOutputCP, SetConsoleOutputCP
//GetCommandLineW, GetModuleFileNameW, GetStdHandle, GetTempFileName
//MultiByteToWideChar, WideCharToMultiByte,
//FileType, FILE_TYPE_PIPE, FILE_TYPE_DISK,
//uRetVal, DWORD, UINT, TRUE, MAX_PATH, CP_UTF8, etc.
#endif


//////////////////// Cross-platform definitions /////////////////////////////////////

#ifdef _MSC_VER
#define fseeko(a, b, c) _fseeki64(a, b, c)
#define ftello(a) _ftelli64(a)
#else
#ifndef UNIX
#ifndef fseeko
#define fseeko(a, b, c) fseeko64(a, b, c)
#endif
#ifndef ftello
#define ftello(a) ftello64(a)
#endif
#endif
#endif

#ifdef WINDOWS
#define strcasecmp _stricmp
#endif

#if defined(__GNUC__) || defined(__clang__)
#define bswap(x) __builtin_bswap32(x)
#define bswap64(x) __builtin_bswap64(x)
#elif defined(_MSC_VER)
#define bswap(x) _byteswap_ulong(x)
#define bswap64(x) _byteswap_uint64(x)
#else
#define bswap(x) \
  +(((( x ) &0xff000000) >> 24) | +((( x ) &0x00ff0000) >> 8) | +((( x ) &0x0000ff00) << 8) | +((( x ) &0x000000ff) << 24))
#define bswap64(x) \
  +((x) >> 56) |
+(((x) << 40) & 0x00FF000000000000) | \
+ (((x) << 24) & 0x0000FF0000000000) | \
+ (((x) << 8) & 0x000000FF00000000) | \
+ (((x) >> 8) & 0x00000000FF000000) | \
+ (((x) >> 24) & 0x0000000000FF0000) | \
+ (((x) >> 40) & 0x000000000000FF00) | \
+ ((x) << 56))
#endif

// ===== src/Utils.hpp =====

#include <cstdint>
#include <cassert>

#if defined(_MSC_VER)
#include <intrin.h> //__prefetch
#endif

ALWAYS_INLINE
int max(int a, int b) { return std::max<int>(a, b); }

ALWAYS_INLINE
int min(int a, int b) { return std::min<int>(a, b); }

ALWAYS_INLINE
uint64_t min(uint64_t a, uint64_t b) { return std::min<uint64_t>(a, b); }

ALWAYS_INLINE
uint32_t square(uint32_t x) {
  return x * x;
}
/**
 * Returns floor(log2(x)).
 * 0/1->0, 2->1, 3->1, 4->2 ..., 30->4,  31->4, 32->5,  33->5
 * @param x
 * @return floor(log2(x))
 */
ALWAYS_INLINE
uint32_t ilog2(uint32_t x) {
#ifdef _MSC_VER
  DWORD tmp = 0;
  if (x != 0) {
    _BitScanReverse(&tmp, x);
  }
  return tmp;
#elif (defined(__GNUC__) || defined(__clang__))
  if (x != 0) {
    x = 31 - __builtin_clz(x);
  }
  return x;
#else
  //copy the leading "1" bit to its left (0x03000000 -> 0x03ffffff)
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  //how many trailing bits do we have (except the first)?
  return BitCount(x >> 1);
#endif
}


template<typename T>
ALWAYS_INLINE
constexpr bool isPowerOf2(T x) {
  return ((x & (x - 1)) == 0);
}

ALWAYS_INLINE
uint32_t nextPowerOf2(uint32_t x)
{
  x--;
  x |= x >> 1;
  x |= x >> 2;
  x |= x >> 4;
  x |= x >> 8;
  x |= x >> 16;
  x++;
  return x;
}

template <uint8_t e>
struct neg_pow10 {
  static constexpr float value = neg_pow10<e - 1>::value / 10.0f;
};
template <>
struct neg_pow10<0> {
  static constexpr float value = 1.0f;
};

#ifndef NDEBUG
#if defined(UNIX)
#include <execinfo.h>
#define BACKTRACE() \
  { \
    void *callstack[128]; \
    int frames  = backtrace(callstack, 128); \
    char **strs = backtrace_symbols(callstack, frames); \
    for (int i = 0; i < frames; ++i) { \
      printf("%s\n", strs[i]); \
    } \
    free(strs); \
  }
#else
// TODO: How to implement this on Windows?
#define BACKTRACE() \
  { }
#endif
#else
#define BACKTRACE()
#endif

// A basic exception class to let catch() in main() know
// that the exception was thrown intentionally.
class IntentionalException : public std::exception {};

// Error handler: print message if any, and exit
[[noreturn]] static void quit(const char *const message = nullptr) {
  if( message != nullptr ) {
    printf("\n%s", message);
  }
  printf("\n");
  throw IntentionalException();
}

inline uint8_t clip(int const px) {
  if( px > 255 ) {
    return 255;
  }
  if( px < 0 ) {
    return 0;
  }
  return px;
}

inline uint8_t clamp4(const int px, const uint8_t n1, const uint8_t n2, const uint8_t n3, const uint8_t n4) {
  int maximum = n1;
  if( maximum < n2 ) {
    maximum = n2;
  }
  if( maximum < n3 ) {
    maximum = n3;
  }
  if( maximum < n4 ) {
    maximum = n4;
  }
  int minimum = n1;
  if( minimum > n2 ) {
    minimum = n2;
  }
  if( minimum > n3 ) {
    minimum = n3;
  }
  if( minimum > n4 ) {
    minimum = n4;
  }
  if( px < minimum ) {
    return minimum;
  }
  if( px > maximum ) {
    return maximum;
  }
  return px;
}

// Circular/wraparound distance on a 256-value ring
// that is: what is the shortest distance between two byte values in any direction (up or down) when we allow a wrap-around at 255 → 0?
//
// Examples:
//  - Distance between 3 and 10 (or 10 and 3) is 7
//  - Distance between 2 and 200 (or 200 and 2) is 58
// For the latter the true linear distance is 198, but the wraparound distance is only 58
// why: starting from 2 down to 0 then to 255 then further down to 200 takes 58 steps
//
// Where such wrap-around distances are useful:
// For image compression (pixel prediction):
//   images with color-channel transform (b, g, r) -> (g, g-r, g-b): the 2nd and 3rd transformed components experience a wrap-around.
// For multi-byte numeric data prediction:
//   less significant bytes wrap around when the whole multi-byte value is increased/decreased sequentially.
ALWAYS_INLINE static int rabs(int x1, int x2) {
  int d = int8_t(x1 - x2); // -128..127
  return d >= 0 ? d : -d; // abs(d) → 0..128
}


/**
 * Bring the cache line containing the given address into the L1 cache.
 * Purely a hint.
 */
ALWAYS_INLINE
void prefetch(const void* const ptr) {
#if defined(_MSC_VER)
#if defined(X64_SIMD_AVAILABLE)
  _mm_prefetch(static_cast<const char*>(ptr), _MM_HINT_T0);
#elif defined(_M_ARM64) || defined(_M_ARM)
  __prefetch(ptr);
#else
  (void)ptr;
#endif
#elif defined(__GNUC__) || defined(__clang__)
  __builtin_prefetch(ptr, 0 /*read*/, 3 /*high temporal locality*/);
#else
  (void)ptr;
#endif
}

// ===== src/BlockType.hpp =====

#include <type_traits>

enum class BlockType {
  DEFAULT = 0,
  JPEG,
  HDR,
  IMAGE1,
  IMAGE4,
  IMAGE8,
  IMAGE8GRAY,
  IMAGE24,
  IMAGE32,
  ZLIB,
  GIF,
  PNG8,
  PNG8GRAY,
  PNG24,
  PNG32,
  RLE,
  LZW,
  Count
};

inline int operator << (BlockType bt, int shift)
{
  using T = std::underlying_type_t <BlockType>;
  return (static_cast<T>(bt) << shift);
}

bool hasRecursion(BlockType ft);

bool hasInfo(BlockType ft);

bool hasTransform(BlockType ft, int info);

bool isPNG(BlockType ft);

// ===== src/ProgramChecker.hpp =====

#include <cassert>
#include <chrono>
#include <cinttypes>
#include <cstdint>
#include <cstdio>

/**
 * Track time and memory used.
 * @remark: only @ref Array<T> reports its memory usage, we don't know about other types
 */
class ProgramChecker {
private:
  uint64_t memUsed {};  /**< Bytes currently in use (all allocated minus all freed) */
  uint64_t maxMem {};   /**< Most bytes allocated ever */
  std::chrono::time_point<std::chrono::high_resolution_clock> startTime;

  /**
    * Constructor is private so that it cannot be called
    */
  ProgramChecker() {
    startTime = std::chrono::high_resolution_clock::now();
  }

  /**
    * Copy constructor is private so that it cannot be called
    */
  ProgramChecker(ProgramChecker const & /*unused*/) {}

  /**
    * Assignment operator is private so that it cannot be called
    */
  ProgramChecker& operator=(ProgramChecker const & /*unused*/) { return *this; }

  static ProgramChecker *instance;

public:
  static ProgramChecker* getInstance();
  void alloc(uint64_t n);
  void free(uint64_t n);
  double getRuntime() const;

  /**
    * Print elapsed time and used memory
    */
  void print() const;
  ~ProgramChecker();
};


// ===== src/Array.hpp =====

#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cassert>

#ifdef NDEBUG
#define CHECK_INDEX(index, upper_bound)((void) 0)
#else

static void CHECK_INDEX(uint64_t index, uint64_t upperBound) {
  if( index >= upperBound ) {
    fprintf(stderr, "%" PRIu64 " out of upper bound %" PRIu64 "\n", index, upperBound);
    BACKTRACE()
    quit();
  }
}

#endif

/**
 * Array<T, Align> a(n); allocates memory for n elements of T.
 * The base address is aligned if the "alignment" parameter is given.
 * Constructors for T are not called, the allocated memory is initialized to 0s.
 * It's the caller's responsibility to populate the array with elements.
 * Parameters are checked and indexing is bounds checked if assertions are on.
 * Use of copy and assignment constructors are not supported.
 * @tparam T
 * @tparam Align
 */
template<class T, const int Align = 16>
class Array {
private:
  uint64_t usedSize {};
  uint64_t reservedSize {};
  char *ptr {}; /**< Address of allocated memory (may not be aligned) */
  T *data;   /**< Aligned base address of the elements, (ptr <= T) */
  ProgramChecker *programChecker = ProgramChecker::getInstance();
  void create(uint64_t requestedSize);

  inline uint64_t getPadding() const { return Align - 1; }

  inline uint64_t getAllocatedBytes() const {
    return (reservedSize == 0) ? 0 : reservedSize * sizeof(T) + getPadding();
  }

  /**
    * Assignment operator is private so that it cannot be called
    */
  Array& operator=(Array const& /*unused*/) { return *this; }

public:
  explicit Array(uint64_t requestedSize) { create(requestedSize); }

  ~Array();

  T &operator[](uint64_t i) {
    CHECK_INDEX(i, usedSize);
    return data[i];
  }

  const T &operator[](uint64_t i) const {
    CHECK_INDEX(i, usedSize);
    return data[i];
  }

  /**
    * @return the number of T elements currently in the array.
    */
  uint64_t size() const { return usedSize; }

  /**
    * Grows or shrinks the array.
    * @param newSize the new size of the array
    */
  void resize(uint64_t newSize);

  /**
    * Removes the last element by reducing the size by one (but does not free memory).
    */
  void popBack() {
    assert(usedSize > 0);
    --usedSize;
  }

  /**
    * appends x to the end of the array and reserves space for more elements if needed.
    * @param x the element to append
    */
  void pushBack(const T &x);
  /**
    * Prevent copying
    * Remark: GCC complains if this member is private, so it is public
    */
  Array(const Array &) { assert(false); }

};

template<class T, const int Align>
void Array<T, Align>::create(uint64_t requestedSize) {
#ifdef VERBOSE
  printf("Created Array of size %" PRIu64 "\n", requestedSize);
#endif
  assert(isPowerOf2(Align));
  usedSize = reservedSize = requestedSize;
  if( requestedSize == 0 ) {
    data = nullptr;
    ptr = nullptr;
    return;
  }
  const uint64_t bytesToAllocate = getAllocatedBytes();
  ptr = (char *) calloc(bytesToAllocate, 1);
  if( ptr == nullptr ) {
    quit("Out of memory.");
  }
  uint64_t pad = getPadding();
  data = (T *) (((uintptr_t) ptr + pad) & ~(uintptr_t) pad);
  assert(ptr <= (char *) data && (char *) data <= ptr + Align);
  assert(((uintptr_t) data & (Align - 1)) == 0); //aligned as expected?
  programChecker->alloc(bytesToAllocate);
}

template<class T, const int Align>
void Array<T, Align>::resize(uint64_t newSize) {
  if( newSize <= reservedSize ) {
    usedSize = newSize;
    return;
  }
  char *oldPtr = ptr;
  T *oldData = data;
  const uint64_t oldSize = usedSize;
  programChecker->free(getAllocatedBytes());
  create(newSize);
  if( oldSize > 0 ) {
    assert(oldPtr != nullptr && oldData != nullptr);
    memcpy(data, oldData, sizeof(T) * oldSize);
  }
  if( oldPtr != nullptr ) {
    free(oldPtr);
  }
}

template<class T, const int Align>
void Array<T, Align>::pushBack(const T &x) {
  if( usedSize == reservedSize ) {
    const uint64_t oldSize = usedSize;
    const uint64_t newSize = usedSize * 2 + 16;
    resize(newSize);
    usedSize = oldSize;
  }
  data[usedSize++] = x;
}

template<class T, const int Align>
Array<T, Align>::~Array() {
  programChecker->free(getAllocatedBytes());
  free(ptr);
  usedSize = reservedSize = 0;
  data = nullptr;
  ptr = nullptr;
}


// ===== src/RingBuffer.hpp =====

#include <cassert>
#include <cstdint>

template<class T>
class RingBuffer {
private:
  Array<T> b;
  uint32_t offset {0}; /**< Number of input bytes in buffer (not wrapped), will be masked when used for indexing */
  uint32_t mask;

public:
  /**
    * Creates an array of @ref size bytes (must be a power of 2).
    * @param size number of bytes in array
    */
  explicit RingBuffer(const uint32_t size = 0) : b(size), mask(size - 1) {
    assert(isPowerOf2(size));
  }

  void setSize(uint32_t newSize) {
    assert(newSize > 0 && isPowerOf2(newSize));
    b.resize(newSize);
    offset = 0;
    mask = newSize - 1;
  }

  uint32_t getpos() const {
    return offset;
  }

  void fill(const T B) {
    const uint32_t n = (uint32_t) b.size();
    for( uint32_t i = 0; i < n; i++ ) {
      b[i] = B;
    }
  }

  void add(const T B) {
    b[offset & mask] = B;
    offset++;
  }

  /**
    * Returns a reference to the i'th byte with wrap (no out of bounds).
    * @param i
    * @return
    */
  T operator[](const uint32_t i) const {
    return b[i & mask];
  }

  void set(const uint32_t i, const T B) {
    b[i & mask] = B;
  }

  /**
    * Returns i'th byte back from pos (i>0) with wrap (no out of bounds)
    * @param i
    * @return
    */
  T operator()(const uint32_t i) const {
    //assert(i!=0);
    return b[(offset - i) & mask];
  }

  void reset() {
    fill(0);
    offset = 0;
  }

  /**
    * @return the size of the RingBuffer
    */
  uint32_t size() {
    return (uint32_t) b.size();
  }

  // leak internals for high-performance access
  void leakInternals(T* &buffer_ptr, size_t& buffer_mask, size_t& head_pos) {
    buffer_ptr = &b[0];
    buffer_mask = mask;
    head_pos = offset;
  }
};

// ===== src/SIMDType.hpp =====

enum class SIMDType {
  SIMD_NONE, 
  SIMD_SSE2, 
  SIMD_SSE3,
  SIMD_SSE41,
  SIMD_AVX2,
  SIMD_AVX512, 
  SIMD_NEON
};

// ===== src/IPredictor.hpp =====

/**
 * Common interface for all probability predictors (probability map and mixer classes).
 * It declares the update() event handler.
 * A probability predictor predicts the probability that the next bit is 1.

 * After each bit becomes known, all probability predictors participated
 * in the prediction must update their internal state to improve future
 * predictions (adapting). In order to achieve this, a probability
 * predictor must
 *   1) inherit from the IPredictor interface,
 *   2) implement the update() interface method,
 *   3) subscribe for the broadcast in p() or mix(),
 *   4) receive the update event from UpdateBroadcaster and update its internal state in update().
 */
class IPredictor {
public:
  virtual void update() = 0;
  virtual ~IPredictor() = default;
};


// ===== src/UpdateBroadcaster.hpp =====

#include <cassert>

/**
 * The purpose of this class is to inform probability predictors when
 * the next bit is known by calling the update() method of each predictor.
 */
class UpdateBroadcaster {
public:
  void subscribe(IPredictor *subscriber);
  void broadcastUpdate();
  UpdateBroadcaster() {}
private:
  int n {0}; /**< number of subscribed predictors, (number of items in "subscribers" array) */
  IPredictor *subscribers[1024] {};

  /**
    * Copy constructor is private so that it cannot be called
    */
  UpdateBroadcaster(UpdateBroadcaster const & /*unused*/) {}

  /**
    * Assignment operator is private so that it cannot be called
    */
  UpdateBroadcaster& operator=(UpdateBroadcaster const & /*unused*/) { return *this; }
};


// ===== src/Shared.hpp =====

#include <cstdint>

// helper #defines to access shared variables
#define INJECT_SHARED_buf   const RingBuffer<uint8_t> &buf=shared->buf;
#define INJECT_SHARED_pos   const uint32_t pos=shared->buf.getpos();
#define INJECT_SHARED_y     const uint8_t  y=shared->State.y;
#define INJECT_SHARED_c0    const uint8_t  c0=shared->State.c0;
#define INJECT_SHARED_c1    const uint8_t  c1=shared->State.c1;
#define INJECT_SHARED_bpos  const uint8_t  bpos=shared->State.bitPosition;
#define INJECT_SHARED_c4    const uint32_t c4=shared->State.c4;
#define INJECT_SHARED_c8    const uint32_t c8=shared->State.c8;

#define INJECT_SHARED_blockType const BlockType blockType=shared->State.blockType;
#define INJECT_SHARED_blockPos const uint32_t blockPos=shared->State.blockPos;

/**
 * Shared information by all the models and some other classes.
 */
struct Shared
{
private:
  UpdateBroadcaster updateBroadcaster;

public:

  //Shared state and statistics (global)

  RingBuffer<uint8_t> buf; /**< Rotating input queue set by Predictor */
  SIMDType chosenSimd = SIMDType::SIMD_NONE; /**< default value, will be overridden by the CPU dispatcher */
  uint8_t level = 0; /**< level=1..12 compress using less..more RAM */
  uint64_t mem = 0; /**< pre-calculated value of 65536 * 2^level */
  bool toScreen = true;

  struct
  {

    //
    // Global state, used by most models, updated after every bit by update(y)
    // 

    uint8_t y = 0; /**< Last bit, 0 or 1 */
    uint8_t c0 = 1; /**< Last 0-7 bits of the partial byte with a leading 1 bit (1-255) */
    uint8_t c1 = 0; /**< Last whole byte, equals to c4&0xff or buf(1) */
    uint8_t bitPosition = 0; /**< Bits in c0 (0 to 7), in other words the position of the bit to be predicted (0=MSB) */
    uint32_t c4 = 0; /**< Last 4 whole bytes (buf(4)..buf(1)), packed.  Last byte is bits 0-7. */
    uint32_t c8 = 0; /**< Another 4 bytes (buf(8)..buf(5)) */
    uint32_t misses{}; //updated by the Predictor, used by SSE stage
    uint32_t loss{};

    //written by BlockModel, used by many models 
    //see PredictorMain, ContextModel
    BlockType blockType; //current blockType
    int blockInfo;       //current block info (or -1 when none)
    uint32_t blockPos;   //relative position in block

    //written and used by BlockModel
    //see PredictorBlock, ContextModelBlock
    uint32_t blockTypeHistory;
    uint8_t blockStateID;

    //
    // State and statistics per model - set by the individual models
    // Order in appearance: models may use information from models that appears above them
    //

    //MatchModel
    struct
    {
      uint8_t length2;      //used by SSE stage
      uint8_t mode3;        //used by SSE stage 
      uint8_t mode5;        //used by SSE stage 
      uint16_t expectedByte; //used by SSE stage; 0-256 where 256 = not valid
    } Match{};

    //NormalModel
    struct
    {
      uint8_t order; // 0-7
      uint64_t cxt[15]; // context hashes used by NormalModel and MatchModel
    } NormalModel{};

    //image models
    struct
    {
      struct
      {
        uint8_t WW, W, NN, N;
      } pixels; //used by SSE stage
      uint8_t plane; //used by SSE stage
      uint8_t ctx; //used by SSE stage
      uint32_t lossQ; //used by SSE stage
    } Image{};

    //Image1BitModel
    struct
    { // used by the SSE stage
      uint32_t ctx12;    //the 12 nearest pixels
      uint32_t shape;    //how far the ink runs from the current pixel in seven directions
      uint32_t column;   //the 16 pixels above the current one
      uint32_t rowAbove; //15 pixels of the row above, 7 to the left and 7 to the right
      uint8_t run;       //the current run: its length (0-15, in steps) and, in bit 4, its colour
      uint8_t ref;       //the distance to the next colour change on the row above (0-31, in steps) and, in bit 5, the colour of the current run
      uint8_t ink;       //how many of the 40 surrounding pixels are ink (0-40)
      uint8_t match;     //the match models' verdict (0-32): 0 = no match, else 1 + the length of the best match (bits 3-4), the pixel it expects (bit 2), the matches disagree (bit 1), its source was identical (bit 0)
    } Image1{};

    //JpegModel
    struct
    { // used by the SSE stage
      uint16_t state;  // 0: not predicting this bit; otherwise 1 + a direct 12-bit context
    } JPEG;

  } State{};

  Shared();

  void init(uint8_t level, uint32_t bufMem = 0);
  void update(int y, uint32_t p, bool isMissed);
  void reset();
  UpdateBroadcaster* GetUpdateBroadcaster() const;

private:

  /**
    * Copy constructor is private so that it cannot be called
    */
  Shared(Shared const& /*unused*/) {}

  /**
    * Assignment operator is private so that it cannot be called
    */
  Shared& operator=(Shared const& /*unused*/) { return *this; }

  /**
    * Determine if output is redirected
    * @return
    */
  static bool isOutputRedirected();
};


// ===== src/Predictor.hpp =====


class Predictor {
public:
  Shared* shared;
  Predictor(Shared* const sh);
  virtual ~Predictor() = default;
  /**
    * Returns the final probability that the next bit is a 1: P(1).
    * This prediction can be entropy encoded.
    * It is a fixed point number between 0.0 and 1.0 scaled by 31 bits (this comes from the PRECISION requirement of the arithmetic encoder)
    * @return the prediction
    */
  virtual uint32_t p() = 0;
};

// ===== src/String.hpp =====


/**
 * A specialized string class
 */
class String : public Array<char> {
private:
  void appendIntRecursive(uint64_t x);
protected:
#ifdef NDEBUG
  #define chk_consistency() ((void) 0)
#else
  void chk_consistency() const;
#endif
public:
  const char* c_str() const;
  /**
    * Does not include the NUL terminator
    * @return length of the string
    */
  uint64_t strsize() const;
  void operator=(const char *s);
  void operator+=(const char *s);
  void operator+=(char c);
  void operator+=(uint64_t x);
  bool endsWith(const char *ending) const;
  void stripEnd(uint64_t count);
  bool beginsWith(const char *beginning) const;
  void stripStart(uint64_t count);
  int findLast(char c) const;
  explicit String(const char *s = "");
};


// ===== src/file/File.hpp =====

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>

/**
 * This is an abstract class for all the required file operations.
 * The main purpose of these classes is to keep temporary files in RAM as mush as possible. The default behaviour is to simply
 * pass function calls to the operating system - except in case of temporary files.
 */
class File {
public:
  virtual ~File() = default;
  virtual bool open(const char *filename, bool mustSucceed) = 0;
  virtual void create(const char *filename) = 0;
  virtual void close() = 0;
  virtual int getchar() = 0;
  virtual void putChar(uint8_t c) = 0;
  void append(const char *s);
  virtual uint64_t blockRead(uint8_t *ptr, uint64_t count) = 0;
  virtual void blockWrite(uint8_t *ptr, uint64_t count) = 0;
  uint32_t get32();
  void put32(uint32_t x);
  uint64_t getVLI();
  void putVLI(uint64_t i);
  virtual void setpos(uint64_t newPos) = 0;
  virtual void setEnd() = 0;
  virtual uint64_t curPos() = 0;
  virtual bool eof() = 0;
};


// ===== src/file/fileUtils.hpp =====
#ifndef PAQ8PX_FILEUTILS_HPP
#define PAQ8PX_FILEUTILS_HPP

#include <cerrno>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>

//////////////////// IO functions and classes ///////////////////
// Wrappers to utf8 vs. wchar functions
// Linux i/o works with utf8 char* but on windows it's wchar_t*.
// We'll be using utf8 char* in all our functions, so we need to convert to/from
// wchar_t* when on windows.


// The preferred slash for displaying
// We will change the BADSLASH to GOODSLASH before displaying a path string to the user
#ifdef WINDOWS
#define BADSLASH '/'
#define GOODSLASH '\\'
#else
#define BADSLASH '\\'
#define GOODSLASH '/'
#endif

//only for Windows: a class encapsulating a wchar string converted from a utf8 string
//purpose: to properly free the allocated string buffer on destruction
#ifdef WINDOWS
class WcharStr {
  private:
  //convert a utf8 string to a wchar string
  wchar_t *utf8_to_wchar_str(const char *utf8_str) {
    int buffersize     = MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, NULL, 0);
    wchar_t *wchar_str = new wchar_t[buffersize];
    MultiByteToWideChar(CP_UTF8, 0, utf8_str, -1, wchar_str, buffersize);
    return wchar_str;
  }

  public:
  wchar_t *wchar_str;
  WcharStr(const char *utf8_str) {
    wchar_str = utf8_to_wchar_str(utf8_str);
  }
  ~WcharStr() {
    delete[] wchar_str;
  }
};
#endif

//only for Windows: a class encapsulating a utf8 string converted from a wchar string
//purpose: to properly free the allocated string buffer on destruction
#ifdef WINDOWS
class Utf8Str {
  private:
  //convert a wchar string to a utf8 string
  char *wchar_to_utf8_str(const wchar_t *wchar_str) {
    int buffersize = WideCharToMultiByte(CP_UTF8, 0, wchar_str, -1, NULL, 0, NULL, NULL);
    char *utf8_str = new char[buffersize];
    WideCharToMultiByte(CP_UTF8, 0, wchar_str, -1, utf8_str, buffersize, NULL, NULL);
    return utf8_str;
  }

  public:
  char *utf8_str;
  Utf8Str(const wchar_t *wchar_str) {
    utf8_str = wchar_to_utf8_str(wchar_str);
  }
  ~Utf8Str() {
    delete[] utf8_str;
  }
};
#endif

#ifdef WINDOWS
#define STAT __stat64
#else
#define STAT stat
#endif

static constexpr int READ = 0;
static constexpr int WRITE = 1;
static constexpr int APPEND = 2;

/**
 * Wrapper function (Linux vs Windows) to open a file
 * @param filename
 * @param mode
 * @return
 */
static FILE* openFile(const char *filename, const int mode) {
  FILE *file = nullptr;
#ifdef WINDOWS
  file = _wfopen(WcharStr(filename).wchar_str, mode == READ ? L"rb" : mode == WRITE ? L"wb+" : L"a");
#else
  file = fopen(filename, mode == READ ? "rb" : mode == WRITE ? "wb+" : "a");
#endif
  return file;
}

/**
 * Wrapper function (Linux vs Windows) to examine a path
 * @param path
 * @param status
 * @return
 */
static bool statPath(const char *path, struct STAT &status) {
#ifdef WINDOWS
  return _wstat64(WcharStr(path).wchar_str, &status);
#else
  return stat(path, &status) != 0;
#endif
}

/**
 * examines given "path" and returns:
 * 0: on error
 * 1: when exists and is a file
 * 2: when exists and is a directory
 * 3: when does not exist, but looks like a file       /abcd/efgh
 * 4: when does not exist, but looks like a directory  /abcd/efgh/
 * @param path
 * @return
 */
static int examinePath(const char *path) {
  struct STAT status {};
  const bool success = static_cast<int>(statPath(path, status)) == 0;
  if( !success ) {
    if( errno == ENOENT ) { //no such file or directory
      const int len = static_cast<int>(strlen(path));
      if( len == 0 ) {
        return 0; //error: path is an empty string
      }
      const char lastChar = path[len - 1];
      if( lastChar != '/' && lastChar != '\\' ) {
        return 3; //looks like a file
      }

      return 4; //looks like a directory
    }
    return 0; //error
  }
  if((status.st_mode & S_IFREG) != 0 ) {
    return 1; //existing file
  }
  if((status.st_mode & S_IFDIR) != 0 ) {
    return 2; //existing directory
  }
  return 0; //error: "path" may be a socket, symlink, named pipe, etc.
}

/**
 * Creates a directory if it does not exist
 * @param dir
 * @return
 */
static int makeDir(const char *dir) {
  if( examinePath(dir) == 2 ) { //existing directory
    return 2; //2: directory already exists, no need to create
  }
#ifdef WINDOWS
  const bool created = (CreateDirectoryW(WcharStr(dir).wchar_str, nullptr) == TRUE);
#else
#ifdef UNIX
  const bool created = (mkdir(dir, 0777) == 0);
#else
#error Unknown target system
#endif
#endif
  return created ? 1 : 0; //0: failed, 1: created successfully
}

/**
 * Creates directories recursively if they don't exist.
 */
static void makeDirectories(const char *filename) {
  String path(filename);
  uint64_t start = 0;
  if( path[1] == ':' ) {
    start = 2; //skip drive letter (c:)
  }
  if( path[start] == '/' || path[start] == '\\' ) {
    start++; //skip leading slashes (root dir)
  }
  for( uint64_t i = start; path[i] != 0; ++i ) {
    if( path[i] == '/' || path[i] == '\\' ) {
      char saveChar = path[i];
      path[i] = 0;
      const char *dirName = path.c_str();
      const int created = makeDir(dirName);
      if( created == 0 ) {
        printf("Unable to create directory %s", dirName);
        quit();
      }
      if( created == 1 ) {
        printf("Created directory %s\n", dirName);
      }
      path[i] = saveChar;
    }
  }
}

#endif //PAQ8PX_FILEUTILS_HPP


// ===== src/file/FileDisk.hpp =====


/**
 * This class is responsible for files on disk.
 * It simply passes function calls to stdio.
 */
class FileDisk : public File {
private:
  /**
    * Helper function: create a temporary file
    *
    * On Windows when using tmpFile() the temporary file may be created
    * in the root directory causing access denied error when User Account Control (UAC) is on.
    * To avoid this issue with tmpFile() we simply use fopen() instead.
    * We create the temporary file in the directory where the executable is launched from.
    * Luckily the MS c runtime library provides two (MS specific) fopen() flags: "T"emporary and "d"elete.
    * @return
    */
  static FILE* makeTmpFile();
protected:
  FILE *file;

public:
  FileDisk();
  ~FileDisk() override;
  bool open(const char *filename, bool mustSucceed) override;
  void create(const char *filename) override;
  void createTmp();
  void close() override;
  int getchar() override;
  void putChar(uint8_t c) override;
  uint64_t blockRead(uint8_t *ptr, uint64_t count) override;
  void blockWrite(uint8_t *ptr, uint64_t count) override;
  void setpos(uint64_t newPos) override;
  void setEnd() override;
  uint64_t curPos() override;
  bool  eof() override;
};


// ===== src/ArithmeticEncoder.hpp =====


/**
 * Binary arithmetic encoder/decoder with configurable probability precision.
 *
 * Encoding narrows an interval [x1, x2) based on a probability p, then emits
 * the leading bits that have been determined. Pending bits handle the case where
 * the interval straddles the midpoint without resolving a leading bit.
 *
 * Decoding mirrors encoding: it tracks the same interval alongside a 32-bit
 * window `x` into the compressed bitstream, and resolves each bit by comparing
 * x against the interval midpoint.
 */
class ArithmeticEncoder
{
public:
  explicit ArithmeticEncoder(File* archive);

  static constexpr int PRECISION = 31; ///< Probability precision in bits; p is in [1, 2^PRECISION)
  static_assert(PRECISION < 32, "Arithmetic encoder supports max 31 bits of probability precision");

  uint32_t x1;           ///< Low  end of the current coding interval, initially 0
  uint32_t x2;           ///< High end of the current coding interval, initially UINT32_MAX (0xFFFFFFFF)
  uint32_t x;            ///< Only for decoding: current code value, kept in sync with the coding interval [x1, x2)
  uint32_t pending_bits; ///< The number of bits whose value depends on the next resolved bit
  uint8_t  B;            ///< Bit-buffer for packing/unpacking individual bits into bytes
  uint32_t bits_in_B;    ///< Number of valid bits currently held in B (0–8)
  File* archive;         ///< Compressed data file to write during encoding and read during decoding

  void prefetch();                       ///< For decoding: Fill `x` with the first 32 bits of the archive before decoding
  void flush();                          ///< For encoding: Flush any remaining bits to the archive after encoding
  void encodeBit(uint32_t p, int bit);   ///< For encoding: Encode one bit with probability p/2^PRECISION that the bit is 1
  int  decodeBit(uint32_t p);            ///< For decoding: Decode one bit; returns 1 with probability p/2^PRECISION

private:
  int  bit_read();                            ///< Read one bit from the archive
  void bit_write(int bit);                    ///< Write one bit to the archive
  void bit_write_with_pending(int bit);       ///< Write one bit, then flush all pending (opposite) bits
};

// ===== src/Random.hpp =====

#include <cstdint>

/**
 * 32-bit pseudo random number generator
 */
class Random {
  uint64_t _state;

public:
  Random();
  uint32_t operator()(int numberOfBits);
};


// ===== src/StateTable.hpp =====

#include <cstdint>

/**
 * State table:
 *   nex(state, 0) = next state if bit y is 0, 0 <= state < 256
 *   nex(state, 1) = next state if bit y is 1
 *   nex(state, 2) = number of zeros in bit history represented by state
 *   nex(state, 3) = number of ones represented
 *
 * States represent a bit history within some context.
 * State 0 is the starting state (no bits seen).
 * States 1-30 represent all possible sequences of 1-4 bits.
 * States 31-252 represent a pair of counts, (n0,n1), the number
 *   of 0 and 1 bits respectively. If n0+n1 < 16 then there are
 *   two states for each pair, depending on if a 0 or 1 was the last bit seen.
 * If n0 and n1 are too large, then there is no state to represent this
 * pair, so another state with about the same ratio of n0/n1 is substituted.
 * Also, when a bit is observed and the count of the opposite bit is large,
 * then part of this count is discarded to favor newer data over old.
 */
class StateTable {
  static constexpr uint8_t stateTable[256][4] = {
    {  1,  2, 0, 0},{  3,  5, 1, 0},{  4,  6, 0, 1},{  7, 10, 2, 0}, // 0-3
    {  8, 12, 1, 1},{  9, 13, 1, 1},{ 11, 14, 0, 2},{ 15, 19, 3, 0}, // 4-7
    { 16, 23, 2, 1},{ 17, 24, 2, 1},{ 18, 25, 2, 1},{ 20, 27, 1, 2}, // 8-11
    { 21, 28, 1, 2},{ 22, 29, 1, 2},{ 26, 30, 0, 3},{ 31, 33, 4, 0}, // 12-15
    { 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1},{ 32, 35, 3, 1}, // 16-19
    { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2},{ 34, 37, 2, 2}, // 20-23
    { 34, 37, 2, 2},{ 34, 37, 2, 2},{ 36, 39, 1, 3},{ 36, 39, 1, 3}, // 24-27
    { 36, 39, 1, 3},{ 36, 39, 1, 3},{ 38, 40, 0, 4},{ 41, 43, 5, 0}, // 28-31
    { 42, 45, 4, 1},{ 42, 45, 4, 1},{ 44, 47, 3, 2},{ 44, 47, 3, 2}, // 32-35
    { 46, 49, 2, 3},{ 46, 49, 2, 3},{ 48, 51, 1, 4},{ 48, 51, 1, 4}, // 36-39
    { 50, 52, 0, 5},{ 53, 43, 6, 0},{ 54, 57, 5, 1},{ 54, 57, 5, 1}, // 40-43
    { 56, 59, 4, 2},{ 56, 59, 4, 2},{ 58, 61, 3, 3},{ 58, 61, 3, 3}, // 44-47
    { 60, 63, 2, 4},{ 60, 63, 2, 4},{ 62, 65, 1, 5},{ 62, 65, 1, 5}, // 48-51
    { 50, 66, 0, 6},{ 67, 55, 7, 0},{ 68, 57, 6, 1},{ 68, 57, 6, 1}, // 52-55
    { 70, 73, 5, 2},{ 70, 73, 5, 2},{ 72, 75, 4, 3},{ 72, 75, 4, 3}, // 56-59
    { 74, 77, 3, 4},{ 74, 77, 3, 4},{ 76, 79, 2, 5},{ 76, 79, 2, 5}, // 60-63
    { 62, 81, 1, 6},{ 62, 81, 1, 6},{ 64, 82, 0, 7},{ 83, 69, 8, 0}, // 64-67
    { 84, 71, 7, 1},{ 84, 71, 7, 1},{ 86, 73, 6, 2},{ 86, 73, 6, 2}, // 68-71
    { 44, 59, 5, 3},{ 44, 59, 5, 3},{ 58, 61, 4, 4},{ 58, 61, 4, 4}, // 72-75
    { 60, 49, 3, 5},{ 60, 49, 3, 5},{ 76, 89, 2, 6},{ 76, 89, 2, 6}, // 76-79
    { 78, 91, 1, 7},{ 78, 91, 1, 7},{ 80, 92, 0, 8},{ 93, 69, 9, 0}, // 80-83
    { 94, 87, 8, 1},{ 94, 87, 8, 1},{ 96, 45, 7, 2},{ 96, 45, 7, 2}, // 84-87
    { 48, 99, 2, 7},{ 48, 99, 2, 7},{ 88,101, 1, 8},{ 88,101, 1, 8}, // 88-91
    { 80,102, 0, 9},{103, 69,10, 0},{104, 87, 9, 1},{104, 87, 9, 1}, // 92-95
    {106, 57, 8, 2},{106, 57, 8, 2},{ 62,109, 2, 8},{ 62,109, 2, 8}, // 96-99
    { 88,111, 1, 9},{ 88,111, 1, 9},{ 80,112, 0,10},{113, 85,11, 0}, // 100-103
    {114, 87,10, 1},{114, 87,10, 1},{116, 57, 9, 2},{116, 57, 9, 2}, // 104-107
    { 62,119, 2, 9},{ 62,119, 2, 9},{ 88,121, 1,10},{ 88,121, 1,10}, // 108-111
    { 90,122, 0,11},{123, 85,12, 0},{124, 97,11, 1},{124, 97,11, 1}, // 112-115
    {126, 57,10, 2},{126, 57,10, 2},{ 62,129, 2,10},{ 62,129, 2,10}, // 116-119
    { 98,131, 1,11},{ 98,131, 1,11},{ 90,132, 0,12},{133, 85,13, 0}, // 120-123
    {134, 97,12, 1},{134, 97,12, 1},{136, 57,11, 2},{136, 57,11, 2}, // 124-127
    { 62,139, 2,11},{ 62,139, 2,11},{ 98,141, 1,12},{ 98,141, 1,12}, // 128-131
    { 90,142, 0,13},{143, 95,14, 0},{144, 97,13, 1},{144, 97,13, 1}, // 132-135
    { 68, 57,12, 2},{ 68, 57,12, 2},{ 62, 81, 2,12},{ 62, 81, 2,12}, // 136-139
    { 98,147, 1,13},{ 98,147, 1,13},{100,148, 0,14},{149, 95,15, 0}, // 140-143
    {150,107,14, 1},{150,107,14, 1},{108,151, 1,14},{108,151, 1,14}, // 144-147
    {100,152, 0,15},
    // contexts representing strong trend of 0s or 1s start from here
    {153, 95,16, 0},{154, 69,15, 1},{ 80,155, 1,15},{100,156, 0,16}, // 149-152
    {157, 95,17, 0},{158, 69,16, 1},{ 80,159, 1,16},{100,160, 0,17}, // 153-156
    {161, 95,18, 0},{162, 69,17, 1},{ 80,163, 1,17},{100,164, 0,18}, // 157-160
    {165, 95,19, 0},{166, 69,18, 1},{ 80,167, 1,18},{100,168, 0,19}, // 161-164
    {169, 95,20, 0},{170, 69,19, 1},{ 80,171, 1,19},{100,172, 0,20}, // 165-168
    {173, 95,21, 0},{174, 69,20, 1},{ 80,175, 1,20},{100,176, 0,21}, // 169-172
    {177, 95,22, 0},{178, 69,21, 1},{ 80,179, 1,21},{100,180, 0,22}, // 173-176
    {181, 95,23, 0},{182, 69,22, 1},{ 80,183, 1,22},{100,184, 0,23}, // 177-180
    {185, 95,24, 0},{186, 69,23, 1},{ 80,187, 1,23},{100,188, 0,24}, // 181-184
    {189, 95,25, 0},{190, 69,24, 1},{ 80,191, 1,24},{100,192, 0,25}, // 185-188
    {193, 95,26, 0},{194, 69,25, 1},{ 80,195, 1,25},{100,196, 0,26}, // 189-192
    {197, 95,27, 0},{198, 69,26, 1},{ 80,199, 1,26},{100,200, 0,27}, // 193-196
    {201, 95,28, 0},{202, 69,27, 1},{ 80,203, 1,27},{100,204, 0,28}, // 197-200
    {205, 95,29, 0},{206, 69,28, 1},{ 80,207, 1,28},{100,208, 0,29}, // 201-204
    // contexts with incremental counting start from here
    {209, 95,30, 0},{210, 69,29, 1},{ 80,211, 1,29},{100,212, 0,30}, // 205-208
    {213, 95,31, 0},{214, 69,30, 1},{ 80,215, 1,30},{100,216, 0,31}, // 209-212
    {217, 95,32, 0},{218, 69,31, 1},{ 80,219, 1,31},{100,220, 0,32}, // 213-216
    {221, 95,33, 0},{222, 69,32, 1},{ 80,223, 1,32},{100,224, 0,33}, // 217-220
    {225, 95,34, 0},{226, 69,33, 1},{ 80,227, 1,33},{100,228, 0,34}, // 221-224
    {229, 95,35, 0},{230, 69,34, 1},{ 80,231, 1,34},{100,232, 0,35}, // 225-228
    {233, 95,36, 0},{234, 69,35, 1},{ 80,235, 1,35},{100,236, 0,36}, // 229-232
    {237, 95,37, 0},{238, 69,36, 1},{ 80,239, 1,36},{100,240, 0,37}, // 233-236
    {241, 95,38, 0},{242, 69,37, 1},{ 80,243, 1,37},{100,244, 0,38}, // 237-240
    {245, 95,39, 0},{246, 69,38, 1},{ 80,247, 1,38},{100,248, 0,39}, // 241-244
    {249, 95,40, 0},{250, 69,39, 1},{ 80,251, 1,39},{100,252, 0,40}, // 245-248
    {249, 95,41, 0},{250, 69,40, 1},{ 80,251, 1,40},{100,252, 0,41}, // 249-252
    {1,2, 0,0},{1,2, 0,0},{1,2, 0,0} // 253-255 are unused (and in case we reach such a state we'll need to restart)
  };

  static constexpr uint8_t stateGroup[256] = {
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14, // 0-14
    18,23,24,24,26,25,27,26,26,25,27,26,28,28,29,15, //15-30
    18,23,25,25,26,26,27,27,29,15, //31-40
    18,23,25,24,25,26,26,27,28,27,29,15, //41-52
    18,23,24,24,24,25,25,27,27,28,28,28,29,15, //53-66
    19,22,24,23,24,25,25,26,26,27,27,28,29,28,30,16, //67-82
    19,22,24,23,24,28,29,28,30,16, //83-92
    19,22,24,23,24,28,29,28,30,16, //93-102
    19,22,24,23,24,28,29,28,30,16, //103-112
    19,22,24,23,24,28,29,28,30,16, //113-122
    19,22,24,22,23,29,30,28,30,16, //123-132
    19,22,23,22,23,29,30,29,30,16, //133-142
    19,22, 0, 0,30,16, //143-148
    19,22,30,16, 19,22,30,16, 19,22,30,16, 19,22,30,16, 19,22,30,16, //149-152 153-156 157-160 161-164 165-168 
    19,21,31,16, 19,21,31,16, 19,21,31,16, 19,21,31,16, 19,21,31,16, //169-172 173-176 177-180 181-184 185-188 
    19,21,31,16, 19,21,31,16, 19,21,31,16, 19,21,31,16, 19,21,31,17, //189-192 193-196 197-200 201-204 205-208 
    20,21,31,17, 20,21,31,17, 20,21,31,17, 20,21,31,17, 20,21,31,17, //209-212 213-216 217-220 221-224 225-228
    20,21,31,17, 20,21,31,17, 20,21,31,17, 20,21,31,17, 20,21,31,17, //229-232 233-236 237-240 241-244 245-248
    20,21,31,17, 0,0,0, //249-252 253-255
  };

  // this table needs tuning
  static constexpr uint8_t statePrio[256] = {
    0,1,1,2,2,2,2,5,3,3,3,3,3,3,5,
    6,4,4,4,4,4,4,4,4,4,4,4,4,4,4,6, //15-30
    9,7,7,7,7,7,7,7,7,9, //31-40
    11,8,8,8,8,8,8,8,8,8,8,11, //41-52
    13,10,10,10,10,10,10,10,10,10,10,10,10,13, //53-66
    15,12,12,12,12,12,12,12,12,12,12,12,12,12,12,15, //67-82
    17,14,14,14,14,14,14,14,14,17, //83-92
    19,16,16,16,16,16,16,16,16,19, //93-102
    21,18,18,18,18,18,18,18,18,21, //103-112
    23,20,20,20,20,20,20,20,20,23, //113-122
    25,22,22,22,22,22,22,22,22,25, //123-132
    27,24,24,24,24,24,24,24,24,27, //133-142
    29,26, 0, 0,26,29, //143-148
    31,28,28,31, 33,30,30,33, 35,32,32,35, 37,34,34,37, 39,36,36,39, //149-152 153-156 157-160 161-164 165-168 
    41,38,38,41, 43,40,40,43, 45,42,42,45, 47,44,44,47, 49,46,46,49, //169-172 173-176 177-180 181-184 185-188 
    51,48,48,51, 53,50,50,53, 55,52,52,55, 57,54,54,57, 59,56,56,59, //189-192 193-196 197-200 201-204 205-208 
    61,58,58,61, 63,60,60,63, 65,62,62,65, 67,64,64,67, 69,66,66,69, //209-212 213-216 217-220 221-224 225-228
    71,68,68,71, 73,70,70,73, 75,72,72,75, 77,74,74,77, 79,76,76,79, //229-232 233-236 237-240 241-244 245-248
    81,78,78,81, 0,0,0, //249-252 253-255
  };

public:
  static uint8_t getNextState(uint8_t state, int y);

  /**
    * Next state with probabilistic increment
    * @param oldState
    * @param y
    * @param rnd
    * @return
    */
  static uint8_t getNextState(uint8_t oldState, int y, Random &rnd);

  static void update(uint8_t *state, int y, Random &rnd);

  static uint8_t group(uint8_t state) ;

  static uint8_t prio(uint8_t state);
};


// ===== src/HashElementForContextMap.hpp =====

#include <cstdint>

struct HashElementForContextMap {
  union {

    uint8_t states[7];

    uint8_t priority;  //the first byte in any slot being the entry state serves as a priority for hash replacement strategy as well

    struct { // structure when in slot 0
      uint8_t bitState;  // state of bit0; it is also called the 'bytestate' as this is the first bit of a byte, thus it is always updated for every processed byte 
      uint8_t bitState0; // state of bit1 when bit0 was 0
      uint8_t bitState1; // state of bit1 when bit0 was 1
      uint8_t runcount;  // run count of byte1
      uint8_t byte1;     // the last seen byte
      uint8_t byte2;
      uint8_t byte3;
    } slot0;

    struct { // structure when in slot 1 or slot 2
      uint8_t bitState;   // state of bit2 (slot 1)  /  state of bit5 (slot 2)
      uint8_t bitState0;  // state of bit3 when bit2 was 0 (slot 1)  /  state of bit6 when bit5 was 0 (slot 2)
      uint8_t bitState1;  // state of bit3 when bit2 was 1 (slot 1)  /  state of bit6 when bit5 was 1 (slot 2)
      uint8_t bitState00; // state of bit4 when bit2+bit3 was 00 (slot 1)  /  state of bit7 when bit5+bit6 was 00 (slot 2)
      uint8_t bitState01; // state of bit4 when bit2+bit3 was 01 (slot 1)  /  state of bit7 when bit5+bit6 was 01 (slot 2)
      uint8_t bitState10; // state of bit4 when bit2+bit3 was 10 (slot 1)  /  state of bit7 when bit5+bit6 was 10 (slot 2)
      uint8_t bitState11; // state of bit4 when bit2+bit3 was 11 (slot 1)  /  state of bit7 when bit5+bit6 was 11 (slot 2)
    } slot1and2;
  
  };

  // priority for hash replacement strategy
  inline uint8_t prio() {
    return StateTable::prio(priority);
  }
};

static_assert (sizeof(HashElementForContextMap) == 7);


// ===== src/HashElementForStationaryMap.hpp =====

#include <cstdint>

struct HashElementForStationaryMap {

  uint32_t value;

  // priority for hash replacement strategy
  uint32_t prio() const {
    return value & 1023; // count
  }

};

static_assert(sizeof(HashElementForStationaryMap) == 4);

// ===== src/HashElementForBitHistoryState.hpp =====

#include <cstdint>

struct HashElementForBitHistoryState {
  uint8_t bitState;

  // priority for hash replacement strategy
  inline uint8_t prio() {
    return StateTable::prio(bitState);
  }
};

static_assert (sizeof(HashElementForBitHistoryState) == 1);

// ===== src/Bucket16.hpp =====

#include <cstdint>
#include <cstring>
#include <type_traits>

/**
 * Hash bucket to be used in a hash table
 * A hash bucket consists of a list of HashElements
 * Each hash element consists of a 16-bit checksum for collision detection
 * and an arbitrary-sized value:
 *   For HashElementForBitHistoryState: one bit-history byte   ( sizeof(HashElement) = 2+1 = 3 bytes )
 *   For LargeStationaryMap: two 16-bit counts                 ( sizeof(HashElement) = 2+4 = 6 bytes )
 *   For LargeIndirectContext: byte history (usually 4 bytes)  ( sizeof(HashElement) = 2+4 = 6 bytes )
 *   For ContextMap and ContextMap2: bit and byte statistics   ( sizeof(HashElement) = 2+7 = 9 bytes )
 *
 */

#pragma pack(push,1)
template<typename T>
struct HashElement {
  uint16_t checksum;
  T value;
};
#pragma pack(pop)

template<typename T, int ElementsInBucket>
class Bucket16 {
private:
  HashElement<T> elements[ElementsInBucket];

  // Optimized memmove with known parameters
  // Original:
  //  memmove(&elements[1], &elements[0], i * sizeof(HashElement<T>));
  // Optimized:
  // Implementation: iterate over compile-time indices (N-1 down to 0).
  // Each copy is guarded at runtime by `count`, but the loop itself is
  // fully unrolled by the compiler - no branch on the loop counter.
  // It translates to a series of (fully unrolled) 'mov' intructions.
  //
  // Is = { 0, 1, ..., ElementsInBucket-2 }
  // We copy element[N-1-Is] <- element[N-2-Is] for each Is where
  // (N-1-Is) <= count, i.e. we copy the last `count` positions right by 1.
  template<size_t... Is>
  ALWAYS_INLINE
  void shiftImpl(size_t count, std::index_sequence<Is...>) noexcept {
    // Fold expression : each clause is evaluated in order(left - to - right comma).
    // The copy only executes when the destination index falls within [1, count].
    ((Is < ElementsInBucket - 1 && (ElementsInBucket - 1 - Is) <= count
    ? (elements[ElementsInBucket - 1 - Is] = elements[ElementsInBucket - 2 - Is], 0)
    : 0), ...);
  }

  ALWAYS_INLINE
  void shiftElementsRight(size_t count) noexcept {
    shiftImpl(count, std::make_index_sequence<ElementsInBucket - 1>{});
  }

public:

  void reset() {
    for (size_t i = 0; i < ElementsInBucket; i++)
      elements[i] = {};
  }

  void stat(uint64_t& used, uint64_t& empty) {
    for (size_t i = 0; i < ElementsInBucket; i++)
      if (elements[i].checksum == 0)
        empty++;
      else
        used++;
  }
    
  T* find(uint16_t checksum, Random* rnd) {

    checksum += checksum == 0; //don't allow 0 checksums (0 checksums are used for empty slots)

    if (elements[0].checksum == checksum) //there is a high chance that we'll find it in the first slot, so go for it
      return &elements[0].value;

    for (size_t i = 1; i < ElementsInBucket; ++i) { //note: gcc fully unrolls this loop: it gives a tiny speed improvement for a cost of ~4K exe size
      const uint16_t thisChecksum = elements[i].checksum;
      if (thisChecksum == checksum) { // found matching checksum
        //shift elements down and move matching element to front
        HashElement<T> tmpElement = elements[i];
        shiftElementsRight(i);
        elements[0] = tmpElement;
        return &elements[0].value;
      }
      if (thisChecksum == 0) { // found empty slot
        //shift elements down by 1 (make room for the new element in the first slot)
        shiftElementsRight(i);
        elements[0].checksum = checksum;
        elements[0].value = {};
        return &elements[0].value;
      }
    }

    //no match and no empty slot -> overwrite an existing element with the new (empty) one

    //Replacement strategy:
    // - In case the hash elements represent an indirect context (we have content but no priority): overwrite the last used element
    // - In case the hash elements are counts/statistics: overwrite one of the the last used elements having low priority.
    //
    //   The priority of an element is established by the counts it represents: large counts usually mean that the element is worthy to keep.
    //   When an element is accessed recently it is probably worthy to keep as it represents recent statistics,
    //   when an element is accessed long ago it is probably obolete and may be overwritten.
    //   In order to keep track of the order the elements were accessed we move the most recently accessed element to the front: thus
    //   recently acessed elements are usually somewhere in the front of the bucket, rarely accessed element slowly move to the back. The
    //   last elements in the bucket are cantidates for eviction.
    //
    //   Overwriting the lowest priority element (with no regard of its position) is in favor of homogenous (semi-stationary) files.
    //   Overwriting the last element (with no regard of its priority) is in favor of mixed-content files.
    //   In order to favor both, we are using a probabilistic replacement startegy where most recently accessed and/or higher priority
    //   elements have higher chance to stay in the bucket = rarely accessed and/or low priority elements have higher chance to be 
    //   evicted (overwritten).
    //
    //   The 2 most recently accessed elements are always protected from overwriting.
    
    size_t minElementIdx = ElementsInBucket - 1; //index of element to be evicted
    if constexpr (std::is_same<T, HashElementForContextMap>::value ||
                  std::is_same<T, HashElementForStationaryMap>::value || 
                  std::is_same<T, HashElementForBitHistoryState>::value)
    {
      uint32_t RND = rnd->operator()(32); //note: we'll need only 4 times 6 = 24 bits
      if ((RND & 63) >= 1) {
        uint32_t minPrio = elements[ElementsInBucket - 1].value.prio();
        uint32_t thisPrio = elements[ElementsInBucket - 2].value.prio();
        if (thisPrio < minPrio) {
          minPrio = thisPrio;
          minElementIdx = ElementsInBucket - 2;
        }
        RND >>= 6;
        if ((RND & 63) >= 4) {
          thisPrio = elements[ElementsInBucket - 3].value.prio();
          if (thisPrio < minPrio) {
            minPrio = thisPrio;
            minElementIdx = ElementsInBucket - 3;
          }
          RND >>= 6;
          if ((RND & 63) >= 8) {
            thisPrio = elements[ElementsInBucket - 4].value.prio();
            if (thisPrio < minPrio) {
              minPrio = thisPrio;
              minElementIdx = ElementsInBucket - 4;
            }
            RND >>= 6;
            if ((RND & 63) >= 16) {
              thisPrio = elements[ElementsInBucket - 5].value.prio();
              if (thisPrio < minPrio) {
                minPrio = thisPrio;
                minElementIdx = ElementsInBucket - 5;
              }
            }
          }
        }
      }
    }

    //shift elements down by 1 (make room for the new element in the first slot)
    //at the same time owerwrite the element at position "minElementIdx" (the element to be evicted)
    shiftElementsRight(minElementIdx);
    elements[0].checksum = checksum;
    elements[0].value = {};
    return &elements[0].value;
  }
};

// ===== src/DivisionTable.hpp =====

/**
 * This class provides a static (common) 1024-element lookup table for integer division
 * Initialization will run multiple times, but the table is created only once
 * @todo Split into declaration and definition
 */
class DivisionTable {
public:
  static int* getDT() {
    static int dt[1024];
    for( int n = 0; n < 1024; ++n ) {
      dt[n] = (1 << 30) / (n + 2); // 1/(n+2) scaled by 30 bits
    }
    return dt;
  }
};

// ===== src/Hash.hpp =====

#include <cassert>
#include <cstdint>
#include <cstdio>

//////////////////////// Hash functions //////////////////////////
//
// All hash functions are constructed using multiplicative hashes
// - We usually hash small values
// - Multiplicative hashes promote entropy to the higher bits
// - When combining ( H(x) + H(y) ) entropy is still in higher bits
// - After combining they must be finalized by taking the higher
//   bits only to reduce the range to the desired hash table size

// Multipliers
// - They don't need to be prime, just large odd numbers
// - It's advantageous to have a multiplier such as m-1 is divisible by 4
//   See also: https://en.wikipedia.org/wiki/Linear_congruential_generator
// - It's advantageous when the multiplier is based on an irrational number to guarantee good (long) periodic behaviour
// - The golden ratio is usually preferred as a multiplier (PHI64) as it is the most irrational number

// Golden ratio of 2^64
constexpr uint64_t PHI64 = UINT64_C(0x9E3779B97F4A7C15); // 11400714819323198485

// Some more arbitrary magic numbers
constexpr uint64_t MUL64_1 = UINT64_C(0x993DDEFFB1462949);
constexpr uint64_t MUL64_2 = UINT64_C(0xE9C91DC159AB0D2D);
constexpr uint64_t MUL64_3 = UINT64_C(0x83D6A14F1B0CED75);
constexpr uint64_t MUL64_4 = UINT64_C(0xA14F1B0CED5A841D);
constexpr uint64_t MUL64_5 = UINT64_C(0xC0E51314A614F4E1);
constexpr uint64_t MUL64_6 = UINT64_C(0xDA9CC2600AE45A25);
constexpr uint64_t MUL64_7 = UINT64_C(0x826797AA04A65735);
constexpr uint64_t MUL64_8 = UINT64_C(0x2375BE54C41A08ED);
constexpr uint64_t MUL64_9 = UINT64_C(0xD39104E950564B39);
constexpr uint64_t MUL64_10 = UINT64_C(0x3091697D5E685621);
constexpr uint64_t MUL64_11 = UINT64_C(0x20EB84EE04A3C7E1);
constexpr uint64_t MUL64_12 = UINT64_C(0xF501F1D0944B2385);
constexpr uint64_t MUL64_13 = UINT64_C(0xE3E4E8AA829AB9B5);



/**
 * Finalizer (range reduction).
 * Keep the necessary number of bits after performing a
 * (combination of) multiplicative hash(es).
 * @param hash
 * @param hashBits
 * @return
 */
static ALWAYS_INLINE
uint32_t finalize64(const uint64_t hash, const int hashBits) {
  assert(hashBits>=0 && hashBits <= 32); // just a reasonable upper limit
  return static_cast<uint32_t>(hash >> (64 - hashBits));
}

/**
 * Get the next 8 or 16 bits following "hashBits" for checksum
 * @param hash
 * @param hashBits
 * @return
 */
static ALWAYS_INLINE
uint8_t checksum8(const uint64_t hash, const int hashBits) {
  constexpr int checksumBits = 8;
  return static_cast<uint8_t>(hash >> (64 - hashBits - checksumBits)) & ((1u << checksumBits) - 1);
}

static ALWAYS_INLINE
uint16_t checksum16(const uint64_t hash, const int hashBits) {
  constexpr int checksumBits = 16;
  return static_cast<uint16_t>(hash >> (64 - hashBits - checksumBits)) & ((1u << checksumBits) - 1);
}

static ALWAYS_INLINE
uint32_t checksum24(const uint64_t hash, const int hashBits) {
  constexpr int checksumBits = 24;
  return static_cast<uint32_t>(hash >> (64 - hashBits - checksumBits)) & ((1u << checksumBits) - 1);
}

//
// value hashing
//
// - Hash 1-13 64-bit (usually small) integers
//


static ALWAYS_INLINE
uint64_t hash(const uint64_t x0) {
  return (x0 + 1) * PHI64;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6;
}

static ALWAYS_INLINE 
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7;
}

static ALWAYS_INLINE 
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7, const uint64_t x8) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7 + (x8 + 1) * MUL64_8;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7, const uint64_t x8, const uint64_t x9) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7 + (x8 + 1) * MUL64_8 + (x9 + 1) * MUL64_9;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7, const uint64_t x8, const uint64_t x9, const uint64_t x10) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7 + (x8 + 1) * MUL64_8 + (x9 + 1) * MUL64_9 + (x10 + 1) * MUL64_10;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7, const uint64_t x8, const uint64_t x9, const uint64_t x10, const uint64_t x11) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7 + (x8 + 1) * MUL64_8 + (x9 + 1) * MUL64_9 + (x10 + 1) * MUL64_10 + (x11 + 1) * MUL64_11;
}

static ALWAYS_INLINE
uint64_t hash(const uint64_t x0, const uint64_t x1, const uint64_t x2, const uint64_t x3, const uint64_t x4, const uint64_t x5, const uint64_t x6,
     const uint64_t x7, const uint64_t x8, const uint64_t x9, const uint64_t x10, const uint64_t x11, const uint64_t x12) {
  return (x0 + 1) * PHI64 + (x1 + 1) * MUL64_1 + (x2 + 1) * MUL64_2 + (x3 + 1) * MUL64_3 + (x4 + 1) * MUL64_4 + (x5 + 1) * MUL64_5 +
         (x6 + 1) * MUL64_6 + (x7 + 1) * MUL64_7 + (x8 + 1) * MUL64_8 + (x9 + 1) * MUL64_9 + (x10 + 1) * MUL64_10 + (x11 + 1) * MUL64_11 +
         (x12 + 1) * MUL64_12;
}

/**
 * Call this function repeatedly for string hashing, or to combine a hash value and a (non-hash) value, or two hash values.
 * @param seed
 * @param x
 * @return
 */
static ALWAYS_INLINE
uint64_t combine64(const uint64_t seed, const uint64_t x) {
  return hash(seed + x);
}

// ===== src/Mixer.hpp =====



/**
  * Combines model predictions with a three-layer network, called a Mixer.
  *
  * The Mixer receives @p n stretched probabilities from the different models.
  * It combines them using adaptive weights and produces a single probability
  * representing the expected probability of the next bit being 1. The Mixer
  * output is passed to the SSE stage and then to the arithmetic coder.
  *
  * The Mixer uses integer arithmetic. Probabilities and weights are represented
  * using fixed-point numbers.
  *
  * The models supply the Mixer with a fixed number of mixer contexts (small
  * integers). Each mixer context selects one weight vector from its context set.
  * This lets the Mixer use different weights in different situations, such as
  * different file types or parts of a file. The mixer contexts are chosen by
  * the models rather than learned by the Mixer.
  *
  * 
  * Layers
  * ---
  * input      Receives @p n stretched probabilities from the models. 
  *            For each of the @p s context sets, one of the @p m weight vectors
  *            is selected. This produces @p s stretched outputs.
  *
  * middle     Receives the @p s outputs from the input layer and @p promoted
  *            inputs from strong models (so-called promoted inputs). It uses
  *            the same context selections as the input layer, so each output
  *            has its own set of context-dependent weights. This produces
  *            another @p s stretched outputs.
  *
  * combining  Receives the @p s outputs from the middle layer and the @p s
  *            outputs from the input layer directly (the skip connection),
  *            as well as the promoted inputs. It uses one weight vector to
  *            combine them into the final output.
  *
  * In addition to passing its stretched output probabilities forward,
  * each layer also stores a squashed version that update() can use to adjust
  * the weights based on how accurate the prediction was.
  * This helps the Mixer learn how to weight its inputs in each context,
  * which improves prediction accuracy over time.
  *
  * 
  * Weight scales
  * ---
  * The input layer receives raw model outputs, while the middle and combining
