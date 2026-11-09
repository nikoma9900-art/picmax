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
  * layers receive more stable, mature signals produced by mixing, as well as
  * promoted inputs from strong models. These signals have different magnitudes,
  * so the 3 layers need different weight scales.
  *
  * Each layer therefore scales its dot product — the sum of its inputs
  * multiplied by their weights — by its own predefined factor. This keeps the
  * weights in a useful range and gives all weights in the layer a similar
  * effective learning rate.
  *
  *
  * Input layout for each layer
  * ---
  * Each layer's input vector is padded to a multiple of the SIMD width so it
  * can be processed efficiently with SIMD instructions. Padding slots are kept
  * at zero and therefore have no effect on the dot product.
  *
  * The models fill the input layer by appending inputs with add() for each bit.
  * The input slots are refilled from the start for each new bit.
  *
  * input       model inputs            [ 0 .. n               )
  *             SIMD padding            [ n .. padded size     )
  *
  * middle      input layer outputs     [ 0 .. s               )
  *             promoted inputs         [ s .. s+promoted      )
  *             SIMD padding            [ s+promoted .. padded size )
  *
  * combining   middle layer outputs    [ 0 .. s               )
  *             skip connection         [ s .. 2s              )
  *             promoted inputs         [ 2s .. 2s+promoted     )
  *             SIMD padding            [ 2s+promoted .. padded size )
  *
  * 
  * Weight initialization
  * ---
  * The weights are initialized to a small positive value instead of zero. This
  * gives each input a small initial contribution while leaving room for training
  * to increase or decrease the weight. Because the inputs are often correlated,
  * training may eventually make some weights very small or even negative to
  * cancel information that is already represented by another input.
  *
  * Starting with a small positive value therefore gives the weights a useful
  * neutral starting point: some can grow from there, while others can shrink
  * and eventually become negative.
  *
  * 
  * Skip connection
  * ---
  * The combining layer sees the input layer's outputs directly, not only through
  * the middle layer. The corresponding input and middle outputs are strongly
  * correlated because they are based on overlapping information and are trained
  * toward the same target.
  *
  * Feeding both correlated signals directly to the combining layer makes learning
  * less efficient. Instead, the skip input is represented as the difference
  * between the input and middle outputs. This gives the combining layer one
  * signal representing the middle layer's prediction and another representing
  * how the input layer differs from it. The difference is typically smaller and
  * less correlated with the middle output, which makes the two signals easier to
  * learn from together.
  *
  * This is why slot s+i carries the difference between input output i and middle
  * output i.
  *
  * Using the difference does not limit what the combining layer can represent.
  * With w_m as the weight for the middle output and w_s as the weight for the
  * skip slot,
  *
  * w_m*mid + w_s*(in - mid) == (w_m - w_s)*mid + w_s*in
  *
  * so any combination of the middle and input outputs can still be represented.
  *
  * Unlike the other weights, the skip weights are initialized to zero. A
  * difference has no preferred direction: a positive weight favors cases where
  * the input output is higher than the middle output, while a negative weight
  * favors the opposite. Zero is therefore a natural neutral starting point for
  * this signal. It also leaves the initial prediction unchanged until training
  * learns a useful correction.
  *
  * This initialization was chosen based on compression results: initializing
  * the skip weights like the other weights improved large files but hurt small
  * files, while starting them at zero gave better overall behavior.
  *
  */

class Mixer : protected IPredictor
{
protected:

  // Fixed-point scale used for the learning rates.
  static constexpr int LR_SCALE = 1 << (16 + 2);
  static constexpr int MIN_LEARNING_RATE_UNITS = 1;
  static constexpr int MAX_LEARNING_RATE_UNITS = 8;
  static constexpr int MAX_LEARNING_RATE = MAX_LEARNING_RATE_UNITS * LR_SCALE - 1;

  // Converts a learning rate expressed in integer units to the fixed-point
  // representation used by the training code. The -1 prevents overflow.
  static constexpr int learningRate(const int units) { return units * LR_SCALE - 1; }

  // Default minimum learning rates reached by the learning rate
  // decay in each layer.
  static constexpr int DEFAULT_END_RATE_INPUT = 6;
  static constexpr int DEFAULT_END_RATE_MIDDLE = 2;
  static constexpr int DEFAULT_END_RATE_COMBINING = 1;

  /**
  * Context selections made for one bit.
  *
  * The input and middle layers share the same selections, so output i in both
  * layers corresponds to the same mixer context. Each selection identifies
  * one weight vector within the available vectors.
  */
  struct ContextSet
  {
    const uint32_t m;    /**< number of weight vectors available */
    const uint32_t s;    /**< number of mixer contexts */
    Array<uint32_t> cxt; /**< selected weight-vector indices */
    size_t count;        /**< number of selections made so far this bit (0..s) */
    uint32_t base;       /**< start of the next range of weight vectors */

    ContextSet(const uint32_t m, const uint32_t s) : m(m), s(s), cxt(s), count(0), base(0) {}
    ContextSet(const ContextSet&) = delete;
    ContextSet& operator=(const ContextSet&) = delete;

    /** Selects context @p cx from the next @p range weight vectors. */
    void select(const uint32_t cx, const uint32_t range) {
      assert(count < s);
      assert(cx < range);
      assert(base + range <= m);
      cxt[count++] = base + cx;
      base += range;
    }

    /** Starts a new set of context selections. */
    void reset() {
      count = 0;
      base = 0;
    }
  };

  /** A contiguous range of slots in a layer's input vector. */
  struct Block
  {
    size_t offset;
    size_t count;
    size_t end() const { return offset + count; }
  };

  /**
  * All parameters, weights, predictions and temporary data for one layer.
  */
  struct Layer
  {
    ContextSet& contexts;  /**< context selections used to choose weight vectors */
    const size_t n;        /**< number of input slots, including SIMD padding */
    const int maxInput;    /**< maximum allowed absolute input value */
    const int errorLimit;  /**< training is skipped when the absolute error is within this limit */

    int scaleFactor;                /**< scale applied to the layer's dot product */
    int rate;                       /**< current learning rate */
    int lowerLimitOfLearningRate;   /**< minimum learning rate reached by decay */

    Array<short, 64> tx;   /**< input vector, padded with zero slots for SIMD */
    Array<short, 64> wx;   /**< weight vectors, one for each context */
    Array<short> pr;       /**< squashed 12-bit predictions used by update() */
    Array<short> out;      /**< stretched outputs routed to later layers */

    Block routed;          /**< slots receiving outputs from the preceding layer */
    Block skip;            /**< slots receiving the skip connection */
    Block promoted;        /**< slots receiving promoted inputs */

    Layer(ContextSet& contexts, size_t n, int maxInput, int errorLimit, short initialWeight,
      int lowerLimit) :
      contexts(contexts), n(n),
      maxInput(maxInput),
      errorLimit(errorLimit),
      scaleFactor(0),
      rate(MAX_LEARNING_RATE),
      lowerLimitOfLearningRate(lowerLimit),
      tx(n), wx(n* contexts.m), pr(contexts.s), out(contexts.s),
      routed{}, skip{}, promoted{} {
      for (uint32_t i = 0; i < contexts.s; ++i) {
        pr[i] = 2048; // initial p = 0.5
        out[i] = 0;
      }
      for (size_t i = 0; i < n * contexts.m; ++i) {
        wx[i] = initialWeight;
      }
    }

    Layer(const Layer&) = delete;
    Layer& operator=(const Layer&) = delete;
  };

  const Shared* const shared;
  const size_t numInputs;            /**< number of add() calls per bit; inputLayer.n rounds this up */
  const size_t numPromotedInputs;    /**< number of slots available to promote() */
  size_t numPromoted;                /**< promote() calls since the last reset */
  size_t numAdded;                   /**< add() calls since the last reset */

  ContextSet bitContexts;   /**< the contexts selected per bit for the input and middle layers */
  ContextSet singleContext; /**< the single context used by the combining layer */

  Layer inputLayer;       /**< combines the model inputs */
  Layer middleLayer;      /**< remixes input-layer outputs and promoted inputs */
  Layer combiningLayer;   /**< combines middle, skip, and promoted inputs */

  /**
  * SIMD implementations used by all layers.
  *
  * dotProduct computes one dot product, dotProduct2 computes two dot products
  * over the same input vector, and train() updates one weight vector.
  */
  virtual int dotProduct(const short* t, const short* w, size_t n) = 0;
  virtual int dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) = 0;
  virtual void train(const short* t, short* w, size_t n, int e) = 0;

  /**
  * Checks that @p layer has valid dimensions for the SIMD kernels and that
  * its dot products fit safely in their accumulators.
  * Used in asserts only.
  */
  static bool isWellFormed(const Layer& layer, int simdWidth);

  /** Sets every weight in @p block of @p layer to @p value. */
  static void setWeightBlock(Layer& layer, const Block& block, short value);

  /**
  * Computes the outputs of @p layer using the selected weight vectors.
  * The stretched outputs are stored in @p layer.out and their squashed
  * probabilities in @p layer.pr.
  */
  void forward(Layer& layer);

  /** Copies the stretched outputs of @p from into @p block of @p to. */
  static void routeOutputs(const Layer& from, Layer& to, const Block& block);

  /**
  * Fills the combining layer's skip block with the difference between the input layer
  * and middle layer output.
  */
  void routeSkipConnection();

public:
  /**
  * @param n inputs to the input layer, i.e. add() calls per bit. The input
  *          vector is rounded up to a multiple of @p simdWidth.
  * @param m number of weight vectors available to the input and middle layers
  * @param s number of mixer contexts selected per bit
  * @param promoted number of extra inputs available through promote()
  * @param simdWidth number of shorts in one SIMD vector
  */
  Mixer(const Shared* sh, int n, int m, int s, int promoted, int simdWidth);

  // The layers refer to context sets inside *this*.
  Mixer(const Mixer&) = delete;
  Mixer& operator=(const Mixer&) = delete;
  Mixer(Mixer&&) = delete;
  Mixer& operator=(Mixer&&) = delete;

  ~Mixer() override;

  /**
  * Computes the final 12-bit probability for the next bit (1..4095).
  * All inputs and context selections for the current bit must have been
  * supplied before calling p().
  */
  virtual int p();

  /** Sets the dot-product scale factors for the input, middle, and combining layers. */
  void setScaleFactor(int sf0, int sf1, int sf2);

  /**
  * Sets the minimum learning rates reached by decay in the input, middle,
  * and combining layers.
  */
  void setLowerLimitOfLearningRate(int lr0, int lr1, int lr2);

  /**
  * Adds one input to the promoted blocks of both the middle and combining
  * layers. The value must be a stretched prediction in the range -2047..2047.
  */
  void promote(int x);

  /** Updates the weights of all layers using the error from the last prediction. */
  void update() override;

  /**
  * Adds one model prediction to the input layer. Call n times per bit.
  * The value must be a stretched prediction in the range -2047..2047.
  */
  void add(int x);

  /**
  * Selects context @p cx from a range of @p range weight vectors. Call @p s
  * times per bit; the selections are used by both the input and middle layers.
  */
  void set(uint32_t cx, uint32_t range);

  /** Starts a new bit by resetting the per-bit input, promotion, and context-selection state. */
  void reset();
};

// ===== src/Stretch.hpp =====

#include <cinttypes>

/**
 * Inverse of @ref squash. d = ln(p/(1-p)), d scaled by 8 bits, p by 12 bits.
 * d has range -2047 to 2047 representing -8 to 8. p has range 0 to 4095.
 */

int stretch(int p);


// ===== src/LargeStationaryMap.hpp =====


/**
 * Map for modelling contexts of (nearly-)stationary data.
 * The context is a hash. For each bit modelled, a 22-bit probability
 * estimate and a 10-bit counter are stored packed into uint32_t.
 *
 */

class LargeStationaryMap : IPredictor
{
public:
  static constexpr int MIXERINPUTS = 3;

private:
  const Shared* const shared;
  Random rnd;
  const int* dt;
  Array<Bucket16<HashElementForStationaryMap, 7>> data;
  const uint32_t hashBits;
  int scale;

  const uint32_t numContexts; /**< Maximum supported contexts */
  uint32_t currentContextIndex; /**< Number of context indexes present in cxt array (0..numContexts-1) */
  Array<uint32_t*> contextPointers; /**< context index of last prediction per context */

public:
  /**
    * Construct using 2^hashBits * sizeof(Bucket16) bytes of memory for storing a maximum of 2^hashBits * ElementsInBucket
    * That is:
    *   hashBits    |       memory          |   maximum number of contexts
    *       10      |  2^10 * 42 =   43 KB  |         7 K  (~12 bits)
    *       11      |  2^11 * 42 =   86 KB  |        14 K  (~13 bits)
    *       12      |  2^12 * 42 =  172 KB  |        28 K  (~14 bits)
    *       13      |  2^13 * 42 =  344 KB  |        57 K  (~15 bits)
    *       14      |  2^14 * 42 =  688 KB  |       114 K  (~16 bits)
    *       15      |  2^15 * 42 =  1.3 MB  |       229 K  (~17 bits)
    *       16      |  2^16 * 42 =  2.7 MB  |       458 K  (~18 bits)
    *       17      |  2^17 * 42 = 11.0 MB  |       1.8 M  (~19 bits)
    *       18      |  2^18 * 42 = 22.0 MB  |       3.6 M  (~20 bits)
    *       19      |  2^19 * 42 = 44.0 MB  |       7.3 M  (~21 bits)
    *       20      |  2^20 * 42 = 88.0 MB  |      14.6 M  (~22 bits)
    *      ...              ...                         ...
    *
    * @param scale
    */
  LargeStationaryMap(const Shared* const sh, const int contexts, const int hashBits, const int scale = 64);

  /**
    * ctx must be a hash
    * @param ctx
    */
  void set(const uint64_t contextHash);
  void setscale(const int scale);
  void reset();
  void update() override;
  void update(uint32_t* cp);
  void mix(Mixer& m);
  void subscribe();
  void skip();

};

// ===== src/AdaptiveMap.hpp =====

#include <cstdint>

/**
 * This is the base class for StateMap and APM.
 * Purpose: common members are here
 */
class AdaptiveMap {
protected:
  const Shared * const shared;
  Array<uint32_t> t; /**< cxt -> prediction in high 22 bits, count in low 10 bits */
  int *dt; /**< Pointer to division table */
  AdaptiveMap(const Shared* const sh, int n);
  void update(uint32_t *p, int limit);
};

// ===== src/StateMap.hpp =====


enum class StateMapType {
  Generic, BitHistory, Run
};

/**
 * A @ref StateMap maps a context to a probability.
 */
class StateMap : public AdaptiveMap, protected IPredictor {
protected:
  const uint32_t numContextSets; /**< Number of context sets */
  const uint32_t numContextsPerSet; /**< Number of contexts in each context set */
  uint32_t currentContextSetIndex; /**< Number of context indexes present in cxt array (0..numContextSets-1) */
  Array<uint32_t> cxt; /**< context index of last prediction per context set */
public:
  int limit; //1..1023
 

  /**
    * Creates a @ref StateMap with @ref n contexts using 4*n bytes memory.
    * @param s
    * @param n number of contexts
    * @param lim
    * @param mapType
    */
  StateMap (const Shared* const sh, int s, int n, int lim, StateMapType mapType);
  ~StateMap() override = default;

  void update() override;

  /**
    * Call @ref p1() when there is only 1 context set.
    * No need to call @ref subscribe().
    * @param cx
    * @return
    */
  int p1(uint32_t cx);

  /**
    * Call @ref p2() for each context when there are more context sets, finally call @ref subscribe() once.
    * sm.p(y, cx, limit) converts state @ref cx (0..n-1) to a probability (0..4095)
    * that the next y=1, updating the previous prediction with y (0..1).
    * limit (1..1023, default 1023) is the maximum count for computing a
    * prediction.  Larger values are better for stationary sources.
    * @param s
    * @param cx
    * @return
    */
  int p2(uint32_t s, uint32_t cx);
  void subscribe();

  /**
    * Call @ref skip() instead of @ref p2() when the context is unknown or uninteresting.
    * @remark no need to call @ref skip() when there is only 1 context or all contexts will be skipped.
    * @param s
    */
  void skip(uint32_t s);
  void print() const;
};


// ===== src/model/Image1BitModel.hpp =====


/**
 * Model for 1-bit (bilevel) images: scanned pages, faxes, line art, dithered photographs.
 *
 * The image is coded pixel by pixel, row by row. For each pixel the model gives the mixer:
 *
 *  - indexed contexts: small templates of the pixels around the current one, each looked up in a
 *    table of bit histories;
 *  - hashed contexts: templates too wide or too deep to index, looked up in a hash table;
 *  - a few direct predictors: how much ink is around, and how long the runs are along the row, up
 *    the column and along the diagonals;
 *  - four 2D match models, which find where the current neighbourhood appeared earlier in the image
 *    and predict what followed it there.
 *
 * It also selects the mixer's weight sets, and hands a summary of the neighbourhood to the SSE stage.
 *
 * Reading the template diagrams
 * -----------------------------
 * The templates in the .cpp are shown as pictures of the pixels they read:
 *
 *      3  ...xxx..          the number on the left: how many rows up (0 = the current row)
 *      2  ..xxxxx.          '?' : the pixel being coded (nothing right of it is known yet)
 *      1  ..xxxxxxx         'x' : a pixel the template reads
 *      0  xxxxx?            '.' : a pixel it does not read
 *
 * Other letters and digits are explained next to their picture.
 */
class Image1BitModel
{
private:

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Sizes
  //////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr int N = 41;      /**< number of indexed contexts */
  static constexpr int nMatch = 4;  /**< number of match models */

  /**< The number of slots of each indexed context, in the order setIndexedContexts() sets them. All
       contexts share one table (t), and each one owns the slice of it named here. */
  static constexpr int C =
    //the small templates around the current pixel
    (1 << 2) + (1 << 4) + (1 << 6) + (1 << 8) + (1 << 8) + (1 << 8) + (1 << 8) + (1 << 9) +
    (1 << 10) + (1 << 12) + (1 << 12) +
    //how much ink is around, at three scales
    5 + 13 + 41 +
    //the dither phase, and two more small templates
    (1 << 8) + (1 << 10) + (1 << 13) +
    //the adaptive templates
    (1 << 7) + (1 << 8) +
    //the 4 nearest pixels and the one two rows up
    32 +
    //the column, the sparse template, the dither phase over 16 rows, and the wide templates
    (1 << 16) + (1 << 16) + (1 << 11) + (1 << 14) + (1 << 14) + (1 << 15) + (1 << 15) + (1 << 15) +
    (1 << 15) + (1 << 14) + (1 << 14) + (1 << 15) + (1 << 16) + (1 << 16) + (1 << 17) +
    //the vertical runs
    (1 << 14) + (1 << 15) +
    //the slanted directions
    (1 << 12) + (1 << 16) +
    //the shape of the ink above
    (1 << 14) +
    //the reference line
    (1 << 12);

  /**< Most indexed contexts are also read through a hashed map, which learns differently. These
       ones are not, as for them the second reading does not help. (bit i = indexed context i) */
  static constexpr uint64_t noMirror =
    1ull << 6 | 1ull << 18 | 1ull << 20 | 1ull << 23 | 1ull << 25 | 1ull << 27 | 1ull << 29 |
    1ull << 32 | 1ull << 34 | 1ull << 36 | 1ull << 38;

  /**< number of hashed contexts: 57 of their own, and the indexed ones not in noMirror */
  static constexpr int nLSM = 55 + N - 11 + 2;

public:
  static constexpr int MIXERINPUTS =
    2 * N +                 // the indexed contexts
    9 * 2 +                 // the direct predictors
    nMatch +                // the match models
    nLSM * LargeStationaryMap::MIXERINPUTS;
  static constexpr int MIXERCONTEXTS = 16 + 64 + 41 + 32 + 64 + 16 + 64 + 512 + 256 + 256 + 49 + 256 + 33; // 1659
  static constexpr int MIXERCONTEXTSETS = 13;

  Image1BitModel(Shared* const sh);
  Image1BitModel(const Image1BitModel&) = delete;
  Image1BitModel& operator=(const Image1BitModel&) = delete;
  void setParam(int widthInBytes); /**< the row width; a change of width starts a new image */
  void update();
  void mix(Mixer& m);

private:
  Shared* const shared;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Where we are, and the rows above
  //////////////////////////////////////////////////////////////////////////////////////////////////

  uint32_t w{};                   /**< row width in bytes */
  uint32_t xByte = 0, yRow = 0;   /**< position of the current pixel: its byte in the row (the bit is bpos), and its row */

  /**< The pixels already coded, one register per row, the newest pixel in bit 0:
         r0         the current row: bit k is k+1 pixels to the left
         r1..r16    the rows 1..16 above: bit 8 is the current column, bit 8-dx is dx columns to the
                    right (so bits 0..7 are the 8 columns ahead, bits 9.. the columns behind)
         r1b..r8b   the rows 1..8 above, one byte further ahead: bit j is column 16-j, which brings
                    the columns +9..+16 into reach
       They are 64 bits wide, so templates and the adaptive template can reach far to the left. */
  uint64_t r0 = 0, r1 = 0, r2 = 0, r3 = 0, r4 = 0, r5 = 0, r6 = 0, r7 = 0, r8 = 0;
  uint64_t r9 = 0, r10 = 0, r11 = 0, r12 = 0, r13 = 0, r14 = 0, r15 = 0, r16 = 0;
  uint64_t r1b = 0, r2b = 0, r3b = 0, r4b = 0, r5b = 0, r6b = 0, r7b = 0, r8b = 0;

  /**< the pixel dy rows up (0..16) and dx columns to the right (up to +8; below 0 on the current row) */
  uint32_t at(int dy, int dx) const;
  /**< the n pixels of a sparse template, the first one in the highest bit */
  uint32_t sparse(const int8_t(*tpl)[2], int n) const;
  /**< the pixel dy rows up and dx columns to the right, read from the buffer; 0 if not coded yet */
  template<typename B> int px(const B& buf, int bpos, int dy, int dx) const;
  /**< n (up to 16) pixels starting at pixel q of the image, the first one in the highest bit; pixels
       not coded yet, or before the image, read as 0 */
  template<typename B> uint32_t pixelsAt(const B& buf, int64_t q, int n, uint32_t curByteIdx) const;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Runs, and the reference line
  //////////////////////////////////////////////////////////////////////////////////////////////////

  uint32_t runLength = 0;              /**< identical pixels ending just left of the current one (0 at the start of a row) */
  uint32_t runValue = 0;               /**< the colour of that run */
  uint32_t prevRun1 = 0, prevRun2 = 0; /**< the lengths of the two runs before it (0 at the start of a row) */

  static constexpr uint32_t maxRowBytes = 8192;   /**< widest row analysed (65536 pixels) */

  /**< The row above, indexed by where it changes colour (see buildRefLine): nc0[x] / nc1[x] is the
       first pixel at or after x where the row turns white / black. */
  Array<uint8_t> refBytes{ maxRowBytes };
  Array<uint32_t> nc0{ maxRowBytes * 8 + 2 };
  Array<uint32_t> nc1{ maxRowBytes * 8 + 2 };
  bool refValid = false;    /**< false on the first row of an image, and for rows too wide to index */
  template<typename B> void buildRefLine(const B& buf);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Repeat periods, for the adaptive templates
  //////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr uint32_t minRowBytes = 16;     /**< narrower rows are not analysed */
  static constexpr uint32_t maxHPeriod = 48;      /**< horizontal periods searched: 2..48 pixels */
  static constexpr uint32_t minVPeriod = 9;       /**< vertical periods searched: 9..64 rows (the rows 1..8 above are read directly anyway) */
  static constexpr uint32_t maxVPeriod = 64;
  static constexpr int minProminence = 64;        /**< a peak smaller than this is not a period */

  Array<uint32_t> hAcc{ maxHPeriod + 1 };          /**< score per horizontal period, smoothed over the last rows */
  Array<uint32_t> vAcc{ maxVPeriod + 1 };          /**< score per vertical period, likewise */
  Array<uint8_t> rowBytes{ maxRowBytes };          /**< the row just finished */
  Array<uint64_t> rowWords{ maxRowBytes / 8 + 2 }; /**< the same, packed 64 pixels to a word */
  uint32_t hPeriod = 0, vPeriod = 0;               /**< the periods found; 0 = none */
  uint32_t qualH = 0, qualV = 0;                   /**< how well the image repeats at them, 0..3 */
  template<typename B> void updateRepeatPeriods(const B& buf);

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Match models
  //
  // Where the current neighbourhood was seen before, and what came next there. This finds repeated
  // letters and repeated halftone cells anywhere earlier in the image. Each of the four looks the
  // neighbourhood up under a key of a different shape (see runMatchModels).
  //////////////////////////////////////////////////////////////////////////////////////////////////

  static constexpr int matchBits = 22;             /**< log2 of the table size per key, in 4-byte words (2^22 words = 16 MB) */
  static constexpr int matchSlots = 3;             /**< positions remembered per bucket */
  static constexpr int bucketBits = matchBits - 2; /**< a bucket is 16 bytes */
  static constexpr uint32_t maxResidual = 16;      /**< a candidate differing from here in more of the 40 compared pixels is not taken */

  /**< the last places a key was seen, most recent first */
  struct MatchBucket
  {
    uint32_t pos[matchSlots];  /**< bpos << 29 | byte index within the image; 0 = empty */
    uint8_t check[matchSlots]; /**< 8 more bits of the key, to tell apart keys that share the bucket */
    uint8_t unused;
  };
  static_assert(sizeof(MatchBucket) == 16, "a match bucket should be 16 bytes");
  Array<MatchBucket> matchTable{ static_cast<uint64_t>(nMatch) << bucketBits };

  uint32_t matchHashIdx[nMatch] = {};  /**< the bucket of the current key */
  uint8_t matchCheck[nMatch] = {};     /**< the check byte of the current key */
  uint32_t matchByteIdx[nMatch] = {}, matchBpos[nMatch] = {}; /**< the source: the pixel the match predicts from */
  uint32_t matchLen[nMatch] = {};      /**< pixels predicted correctly since the match was taken; 0 = no match */
  uint32_t matchSrcX[nMatch] = {};     /**< the source's byte column within its row */
  uint32_t matchResid[nMatch] = {};    /**< how closely the source resembled the current neighbourhood when taken, 0..3 */
  int predictedBit[nMatch] = { -1, -1, -1, -1 }; /**< the pixel each match expects; -1 = none */
  uint32_t lastByteIdx = 0, lastBpos = 0;        /**< the pixel the current keys belong to */

  /**< how many of the 40 pixels around a candidate source differ from those around the current pixel */
  template<typename B> uint32_t residual(const B& buf, uint32_t byteIdx, uint32_t bp, uint32_t curByteIdx) const;

  //////////////////////////////////////////////////////////////////////////////////////////////////
  // Contexts and predictors
  //////////////////////////////////////////////////////////////////////////////////////////////////

  LargeStationaryMap mapL;  /**< the hashed contexts */
  StateMap stateMap;        /**< bit history -> probability, one set per indexed context */
  StateMap matchStateMap;   /**< the match models' situation -> probability, one set per key */
  Random rnd;
  Array<uint32_t> cxt{ N };     /**< the slot each indexed context selected for the current pixel */
  Array<uint8_t> t{ C };        /**< bit history per slot */
  Array<uint16_t> counts{ C };  /**< recent counts of 0s (high byte) and 1s (low byte) per slot */

  /**< What mix() reads off the neighbourhood of the current pixel, for the contexts, the mixer and
       the SSE stage. Each value is described where it is computed. */
  struct Features
  {
    //the pixels right around the current one: readNeighbourhood()
    uint32_t surrounding4, mCtx6, surrounding12, surrounding24, frame16, dil6, ero6;
    int bitcount04, bitcount12, bitcount24, bitcount40;
    //the runs and the reference line: trackRuns()
    uint32_t runIdx, curCol, vb1, vb2;
    //the columns above: readColumns()
    uint32_t col8, col16, col8L, col8R;
    int vrun, colCount;
    uint32_t vrunL, vrunR, vrun16, vrun16L, vrun16R, vrunLL, vrunRR, vrunLLL, vrunRRR;
    //the slanted directions: readDirections()
    uint32_t diagNW, diagNE, drunNW, drunNE, diagNW2, diagNE2, diagNW16, drunNW16;
    uint32_t steepNW, steepNE, flatNW, flatNE, srunNW, srunNE, frunNW;
    uint32_t vsteepNW, vsteepNE, vflatNW, vsrunNW, vfrunNW;
    uint32_t steepNW16, flatNW16, srunNW16, frunNW16;
    //the adaptive template pixels: readAdaptiveTemplate()
    uint32_t atH0, atH1, atH2, atV0, atV1, atV2, atV3;
    //the match models: runMatchModels()
    uint32_t lenBucketA, lenBucketB, matchVote, matchVote4, matchMode;
  } f{};

  //the steps of mix(), in the order it runs them
  template<typename B> void shiftRows(const B& buf, int y, int bpos);
  void trackRuns(int y, bool rowStart, int bpos);
  void readColumns();
  void readDirections();
  template<typename B> void readAdaptiveTemplate(const B& buf, int bpos);
  template<typename B> void runMatchModels(const B& buf, int bpos, bool rowStart);
  void readNeighbourhood(int y);
  void setIndexedContexts(int y, int bpos);
  void setHashedContexts(int y, int bpos);
  void addInputs(Mixer& m);
  void setMixerContexts(Mixer& m, int y, int bpos);
  void publishToSSE();

  /**< a direct predictor: turns a pair of counts into two mixer inputs */
  void add(Mixer& m, uint32_t n0, uint32_t n1);
};


// ===== src/Ilog.hpp =====


/**
 * ilog(x) = round(log2(x) * 16), 0 <= x < 64K
 */
class Ilog {
public:
  static Ilog& getInstance();
  int log(uint16_t x) const;
private:
  /**
    * Compute lookup table by numerical integration of 1/x
    */
  Ilog() {
    uint32_t x = 14155776;
    for( uint32_t i = 2; i < 65536; ++i ) {
      x += 774541002 / (i * 2 - 1); // numerator is 2^29/ln 2
      t[i] = x >> 24;
    }
  }

  /**
    * Copy constructor is private so that it cannot be called
    */
  Ilog(Ilog const & /*unused*/) {}

  /**
    * Assignment operator is private so that it cannot be called
    */
  Ilog& operator=(Ilog const & /*unused*/) { return *this; }

  Array<uint8_t> t = Array<uint8_t>(65536);
};

/**
 * llog(x) accepts 32 bits
 * @param x
 * @return
 */
int llog(uint32_t x);


// ===== src/ContextMap2.hpp =====

// TODO(epsteina): update this documentation
/**
context map for large contexts.
maps to a bit history state, a 3 mostRecentlyUsed byte history, and 1 byte RunStats.

Bit and byte histories are stored in a hash table with 64 byte buckets.
The buckets are indexed by a context ending after 0, 2 or 5 bits of the
current byte. Thus, each byte modeled results in 3 main memory accesses
per context, with all other accesses to cache.

On a byte boundary (bit 0), only 3 of the 7 bit history states are used.
Of the remaining 4 bytes, 1 byte is used as a run length (the consecutive
occurrences of the previously seen byte), 3 are used to store the last
3 distinct bytes seen in this context. The byte history is then combined
with the bit history states to provide additional states that are then
mapped to predictions.
*/


#define CM_USE_NONE 0
#define CM_USE_RUN_STATS 1
#define CM_USE_BYTE_HISTORY 2
#define CM_DEFERRED 64
#define CM_SKIPPED_CONTEXT 128

class ContextMap2 : IPredictor {
public:
  static constexpr int MIXERINPUTS = 4;
  static constexpr int MIXERINPUTS_RUN_STATS = 1;
  static constexpr int MIXERINPUTS_BYTE_HISTORY = 2;

private:

  struct ContextInfo {
    HashElementForContextMap* hashElementInSlot0; /**< pointer to current bit history states, byte history and byte run statistics in slot0 */
    HashElementForContextMap* hashElementInSlot012; /**< pointer to current bit history states in current slot (either slot0 or slot1 or slot2) */
    uint32_t tableIndex; /**< @ref C whole byte context hashes */
    uint16_t tableChecksum; /**< @ref C whole byte context checksums */
    uint8_t flags;
    HashElementForContextMap bitStateTmp;
  };

  const Shared * const shared;
  Random rnd;
  const uint32_t C; /**< max number of contexts */
  Array<Bucket16<HashElementForContextMap, 7>> hashTable; /**< bit and byte histories (statistics) */
  Array<ContextInfo> contextInfoList;
  StateMap runMap;
  StateMap stateMap;
  StateMap bhMap8B;
  StateMap bhMap12B;
  uint32_t index; /**< next context to set by @ref ContextMap2::set(), resets to zero after every round */
  const uint32_t mask;
  const int hashBits;
  int scale;
  uint8_t contextflagsAll;

  void updatePendingContextsInSlot(HashElementForContextMap* const p, uint32_t c);
  void updatePendingContexts(uint32_t ctx, uint16_t checksum, uint32_t c);
  size_t getStateByteLocation(const uint32_t bpos, const uint32_t c0);

public:
  int order = 0; // is set after mix()
  /**
    * Construct using @ref size bytes of memory for @ref contexts contexts.
    * @param size bytes of memory to use
    * @param contexts max number of contexts
    * @param scale
    * @param uw
    */
  ContextMap2(const Shared* const sh, uint64_t size, uint32_t contexts, int scale);

  /**
    * Set next whole byte context to @ref ctx.
    * @param ctx
    */
  void set(uint8_t ctxflags, const uint64_t ctx);
  void skip(const uint8_t ctxflags);
  void skipn(const uint8_t ctxflags, const int n);
  void update() override;
  void setScale(const int Scale);
  void mix(Mixer &m);
};


// ===== src/OLS.hpp =====

#include <cstdint>


/**
 * Exponentially-forgetting OLS (Ordinary Least Squares) predictor
 * Base class with reference implementations
 *
 * @tparam T floating-point type (float)
 */

template <typename T>
class OLS
{
protected:
  static constexpr T min_diagonal = static_cast<T>(1E-8); // Minimum acceptable pivot magnitude

  const size_t n;               // State dimension
  const size_t nPadded;         // Padded row size in matrices and vectors for SIMD alignment
  const size_t solveInterval;   // Recompute weights after every how many updates
  const T lambda;               // Retention factor (0 < lambda < 1, such as 0.99)
  const T nu;                   // Regularization parameter, such as 0.001
                                
  Array<T, 32> x;               // Current feature vector (padded)
  Array<T, 32> w;               // State estimate (coefficient vector, padded)
  Array<T, 32> b;               // Information vector (padded)
  Array<T, 32> mCovariance;     // Covariance / information matrix (padded rows)
  Array<T, 32> mCholesky;       // Temporary matrix to perform Cholesky decomposition (padded rows)

  size_t samplesSinceLastSolve; // 0 <= samplesSinceLastSolve < solveInterval
  size_t featureIndex;          // Current index for adding features, 0 <= featureIndex < n

  // Helper to compute padded dimension
  static constexpr size_t computePadding(size_t n);

public:
  OLS(size_t n, size_t solveInterval, T lambda, T nu);

  virtual void add(T val);  // Builds up the feature vector for the next prediction, should be called n times
  virtual T predict();      // Makes prediction using the added features
  virtual void update(T y); // Incorporates the previous prediction's true value (y)

protected:
  virtual bool factor();
  virtual void solve();
};

// Explicit instantiation declarations
extern template class OLS<float>;

// Convenience type alias
using OLS_float = OLS<float>;

// ===== src/OLS_factory.hpp =====

#include <memory>

std::unique_ptr<OLS_float> create_OLS_float(SIMDType simd, size_t n, size_t solveInterval, float lambda, float nu);

// ===== src/ResidualMap.hpp =====

/**
 * Residual histogram model: tracks the distribution of (partial_actual_value − predicted_value) residuals.
 * The partial_actual_value is the already known bits of the actual byte.
 * Calculating the predicted_value is the responsibility of the caller.
 * Determining the context is also the responsibility of the caller.
 * The distribution is modelling the probability of the next bit in the partial byte being a "1".
 * The context is direct (not hashed). For each byte context modelled, the exact next bit counts are stored.
 *
 * For each context we maintain a running prefix-sum histogram (cumulative counts) over 256 residual bins.
 * Residuals of the full byte would be centered around 0 usually following a Laplacian distribution.
 * A perfect prediction of the full byte gives residual 0.
 * The partial residual uses only the bits known so far, so it converges toward the full residual as more bits are decoded.
 * 
 * The histogram is circular (mod 256) to handle underflows of the difference (partial_actual_value − predicted_value).
 * As a speed optimization, the center is moved from #0 to #192. Since residuals cluster near 0 (mapped to bin #192),
 * the suffix increment loop [bin, BINS) is shorter on average when the center is near the high end of the array.
 * It also reduces how often a query window wraps around, keeping lookups cache-friendly.
 *
 * Read  (mix):    O(numContexts); O(1) per context - four array lookups and two subtractions per window.
 * Write (update): O(BINS) - increment all suffix entries by 1. Sequential, cache-friendly.
 * Halve (aging):  O(BINS) - halve all entries when the total count would not fit in 16 bits.
 *
 * Layout: sums[base+k] = count[0]+...+count[k], k=0..BINS-1.
 *         sums[base+BINS-1] = total.
 */
class ResidualMap : IPredictor
{
public:
  static constexpr int MIXERINPUTS = 2;
  static constexpr int BINS = 256;

private:
  const Shared* const shared;
  int* dt; /**< Pointer to division table */
  int scale;

  const uint32_t numContexts;
  const uint32_t histogramsPerContext;
  uint32_t currentContextIndex;

  Array<short>    predictions;  // pixel value prediction per active context
  Array<uint32_t> bases;        // base offset into sums[] per active context; UINT32_MAX = skipped
  Array<uint16_t> sums;         // prefix-sum histograms: BINS entries per context per distroset

public:
  ResidualMap(const Shared* const sh, const int numContexts, const int histogramsPerContext, const int scale = 64);

  void setscale(const int scale);

  // set: 'prediction' is the predicted byte value; 'distro' selects the histogram set for this context.
  void set(const short prediction, const uint32_t histogram_id);
  void skip();

  // either set() or skip() must be called exactly once per context slot (0..numContexts-1) before mix().
  void mix(Mixer& m);

  // update(): Called once per byte (at bitPosition==0) via UpdateBroadcaster.
  // Walks all active contexts and increments the suffix of their histogram
  // at the bin corresponding to (actual_byte − prediction), then ages if needed.
  void update() override;


  void print();
};

// ===== src/model/ImageModelsCommon.hpp =====

#include <algorithm>
#include <cstdint>
#include <cstdlib>

// PNG Paeth predictor from the World Wide Web Consortium / PNG Development Group spec.
static inline uint8_t paeth(uint8_t const W, uint8_t const N, uint8_t const NW) {
  int p = W + N - NW;
  int pW = abs(p - W);
  int pN = abs(p - N);
  int pNW = abs(p - NW);
  if (pW <= pN && pW <= pNW) {
    return W;
  }
  if (pN <= pNW) {
    return N;
  }
  return NW;
}


// CALIC-style Gradient Adjusted Predictor (GAP).
static inline int gap(
  uint8_t const W, uint8_t const N,
  uint8_t const NW, uint8_t const NE,
  uint8_t const WW, uint8_t const NNE,
  uint8_t const NN
) {
  int dH = abs(W - WW) + abs(N - NW) + abs(NE - N);
  int dV = abs(W - NW) + abs(N - NN) + abs(NE - NNE);

  if (dH > dV) {
    return N;
  }
  if (dV > dH) {
    return W;
  }
  return (N + W - NW);
}

//signed difference between two 8-bit pixel values, quantized
//used by 8-bit and 24-bit image models
ALWAYS_INLINE
int DiffQt(const uint8_t a, const uint8_t b) {
  int d = (a > b) ? (a - b) : (b - a);
  if (d <= 2)d = d;
  else if (d <= 5)d = 3; //3..5
  else if (d <= 9)d = 4; //6..9
  else if (d <= 14)d = 5; //10..14
  else if (d <= 23)d = 6; //15..23
  else d = 7; //24..255
  const int sign = a > b ? 8 : 0;
  return (sign | d);
}

// ===== src/model/Image24BitModel.hpp =====

#include <cmath>

/**
 * Model 24/32-bit image data
 */
class Image24BitModel {
private:
  static constexpr int nRM = 122;
  static constexpr int nOLS = 6;
  static constexpr int nLSM = 40;
  static constexpr int nCM = 31;
  Ilog *ilog = &Ilog::getInstance();

public:
  static constexpr int MIXERINPUTS =
    nLSM * LargeStationaryMap::MIXERINPUTS +
    (nRM * 3) * ResidualMap::MIXERINPUTS +
    (nOLS * 2) * ResidualMap::MIXERINPUTS +
    nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS); // 1031
  static constexpr int MIXERCONTEXTS =
    (1 + 8) + 8 + (8 * 8) + (8 * 8) + 16 + 512 + 256 + (16 * 8) + (8 * 4) + 1024 + (4 * 8) + 256
    + (nRM + nOLS) + (nRM + nOLS) + 32 * 32 + 32 * 32 + 512 + 64 * 8 + 64 * 4 + 8 * 8 * 4; // 6241
  static constexpr int MIXERCONTEXTSETS = 20; 

  Shared * const shared;

  // probability maps
  ContextMap2 cm;
  ResidualMap mapR1, mapR2, mapR3, mapOLS1, mapOLS2;
  LargeStationaryMap mapL;

  // Per-predictor prediction error for each decoded pixel.
  // Stores uint8 rabs(prediction - actual) for all nRM and nOLS predictors at every pixel position.
  // Read back via PredErr(ctxIndex, relX, relY) to estimate how well each predictor
  // performed on causal neighbors (W, N, NW, NE, WW, NN of the current pixel, same color plane).
  // The averaged absolute error across those neighbors feeds mapR1's histogram selection,
  // giving it a spatially-local, per-predictor confidence signal:
  // low value = predictor was accurate nearby.
  // Sized in init() to cover PRED_ERR_ROWS rows: nextPowerOf2(PRED_ERR_ROWS * w * nRM).
  static constexpr size_t PRED_ERR_BUF_ROWS = 3; // three rows (including the current row) - we need to reach the prediction error of W, N, NW, NE, WW, NN
  RingBuffer<uint8_t> predErrBuf;

  // Per-pixel accumulated decoding cost (loss), one byte per pixel per color plane.
  // Stores (loss >> 3) after each decoded byte, reflecting how hard the last pixel was
  // to compress. Read back via Ls(relX, relY); six causal neighbors (W, N, WW, NN, NW, NE)
  // are summed into lossQ, capturing local image complexity.
  // lossQ feeds mapR2's histogram selection,
  // allowing statistical maps to adapt to smooth vs. noisy/high-frequency regions.
  // Sized in init() to cover LOSS_BUF_ROWS rows: nextPowerOf2(LOSS_BUF_ROWS * w).
  static constexpr size_t LOSS_BUF_ROWS = 3; // three rows (including the current row) - we need to reach the prediction error of NN
  RingBuffer<uint8_t> lossBuf;

  static constexpr size_t BEST_PRED_ROWS = 3; // three rows (including the current row) - we need to reach the prediction error of W, N and W+1, N+1
  RingBuffer<uint8_t> bestPredictorIndexes;

  uint32_t loss = 0;  // decoding cost for the current byte, accumulated bit by bit (0..1023 over 8 bits)
  uint32_t lossQ = 0; // sum of lossBuf[] over 6 causal neighbors (W, N, WW, NN, NW, NE), capped at 639;
                      // measures local image complexity: low = smooth region, high = noisy/detailed region
  uint8_t lossQ4 = 0; // lossQ quantized to 3 bits

  // pixel neighborhood
  uint8_t WWWWWW = 0, WWWWW = 0, WWWW = 0, WWW = 0, WW = 0, W = 0;
  uint8_t NWWWW = 0, NWWW = 0, NWW = 0, NW = 0, N = 0, NE = 0, NEE = 0, NEEE = 0, NEEEE = 0;
  uint8_t NNNWWW = 0, NNWWW = 0, NNWW = 0, NNW = 0, NN = 0, NNE = 0, NNEE = 0, NNEEE = 0;
  uint8_t NNNWW = 0, NNNW = 0, NNN = 0, NNNE = 0, NNNEE = 0, NNNEEE = 0;
  uint8_t NNNNW = 0, NNNN = 0, NNNNE = 0;
  uint8_t NNNNN = 0;
  uint8_t NNNNNN = 0;
  uint8_t WWp1 = 0, Wp1 = 0, p1 = 0, NWp1 = 0, Np1 = 0, NEp1 = 0, NNp1 = 0;
  uint8_t WWp2 = 0, Wp2 = 0, p2 = 0, NWp2 = 0, Np2 = 0, NEp2 = 0, NNp2 = 0;

  int info = 0;
  uint32_t alpha = 0;
  int color = 0;
  int stride = 3;
  uint32_t ctx[2]{}, padding = 0, x = 0, w = 0, line = 0;
  uint32_t lastPos = 0;
  uint32_t predictions[nRM + nOLS]{};

  uint8_t ctx_best_direction{};
  uint8_t ctx_best_residual{};
  uint8_t ctx_best_direction_p1{};
  uint8_t ctx_best_direction_p2{};

  // OLS

  static constexpr float lambda[nOLS] = {0.98f, 0.87f, 0.9f, 0.8f, 0.9f, 0.7f};
  static constexpr int num[nOLS] = {32, 12, 15, 10, 14, 8};
  static constexpr float nu = 0.001f;
  std::unique_ptr<OLS_float> ols[nOLS][4]; // 4: for RGBA color components

  const uint8_t* olsCtx1[32] = { &WWWWWW, &WWWWW, &WWWW, &WWW, &WW, &W, &NWWWW, &NWWW, &NWW, &NW, &N, &NE, &NEE, &NEEE, &NEEEE, &NNWWW,
                              &NNWW, &NNW, &NN, &NNE, &NNEE, &NNEEE, &NNNWW, &NNNW, &NNN, &NNNE, &NNNEE, &NNNNW, &NNNN, &NNNNE, &NNNNN,
                              &NNNNNN };
  const uint8_t* olsCtx2[12] = { &WWW, &WW, &W, &NWW, &NW, &N, &NE, &NEE, &NNW, &NN, &NNE, &NNN };
  const uint8_t* olsCtx3[15] = { &N, &NE, &NEE, &NEEE, &NEEEE, &NN, &NNE, &NNEE, &NNEEE, &NNN, &NNNE, &NNNEE, &NNNN, &NNNNE, &NNNNN };
  const uint8_t* olsCtx4[10] = { &N, &NE, &NEE, &NEEE, &NN, &NNE, &NNEE, &NNN, &NNNE, &NNNN };
  const uint8_t* olsCtx5[14] = { &WWWW, &WWW, &WW, &W, &NWWW, &NWW, &NW, &N, &NNWW, &NNW, &NN, &NNNW, &NNN, &NNNN };
  const uint8_t* olsCtx6[8] = { &WWW, &WW, &W, &NNN, &NN, &N, &p1, &p2 };
  const uint8_t** olsCtxs[nOLS] = { &olsCtx1[0], &olsCtx2[0], &olsCtx3[0], &olsCtx4[0], &olsCtx5[0], &olsCtx6[0] };

  Image24BitModel(Shared* const sh, uint64_t size);

  uint8_t Px(int relX, int relY, int colorShift) const;
  uint8_t Ls(int relX, int relY) const;
  uint8_t GetPredErr(uint32_t ctxIndex, int relX, int relY) const;
  uint32_t GetPredErrAvg(const uint32_t predictorIndex) const;
  void MakePrediction(int i, uint8_t base1, uint8_t base2, int prediction);
  void MakePredictionC(int i, int prediction);
  void MakePredictionAvg(int i, int base1, int base2);
  void MakePredictionTrend(int i, int base1, int other1, int base2);
  void MakePredictionSmooth(int i, int base1, int other1, int base2);

  /**
   * New image.
   */
  void init();
  void setParam(int width, uint32_t alpha0);
  void update();
  void mix(Mixer &m);
};

// ===== src/model/Image4BitModel.hpp =====


/**
 * Model for 4-bit image data
 */
class Image4BitModel : IPredictor {
private:
  static constexpr int S = 14; /**< number of contexts */
  const Shared * const shared;
  Random rnd;
  Array<Bucket16<HashElementForBitHistoryState, 7>> hashTable;
  const uint32_t mask;
  const int hashBits;
  Array<uint64_t> hashes;
  StateMap sm;
  LargeStationaryMap mapL;
  uint8_t *cp[S] {}; /**< context pointers  */
  uint8_t WW = 0, W = 0, NWW = 0, NW = 0, N = 0, NE = 0, NEE = 0, NNWW = 0, NNW = 0, NN = 0, NNE = 0, NNEE = 0;
  int w = 0, col = 0, line = 0, run = 0, prevColor = 0;

public:
  static constexpr int MIXERINPUTS =
    1 + // individual m.add
    S * 3 + 
    S * LargeStationaryMap::MIXERINPUTS;
  static constexpr int MIXERCONTEXTS = 256 + 512 + 512 + 1024 + 16 + 1; /**< 2321 */
  static constexpr int MIXERCONTEXTSETS = 6;
  explicit Image4BitModel(const Shared* const sh, uint64_t size);
  void setParam(int widthInBytes);
  void update() override;
  void mix(Mixer &m);
};


// ===== src/IndirectContext.hpp =====

#include <cassert>
#include <cstdint>

template<typename T>
class IndirectContext {
private:
  Array<T> data;
  T *ctx;
  const uint32_t ctxMask, inputMask, inputBits, contextBits;

public:
  IndirectContext(const int bitsPerContext, const int inputBits, const int contextBits = sizeof(T)*8) :
    data(UINT64_C(1) << bitsPerContext), ctx(&data[0]),
    ctxMask((UINT32_C(1) << bitsPerContext) - 1), 
    inputMask((UINT32_C(1) << inputBits) - 1), 
    inputBits(inputBits),
    contextBits(contextBits) {
    assert(bitsPerContext > 0 && bitsPerContext <= 20);
    assert(inputBits > 0 && inputBits <= 8);
    assert(contextBits <= sizeof(T)*8);
    if (contextBits < sizeof(T) * 8) // need for leading bit -> include it
      reset(); 
  }

  void reset() {
    for (uint64_t i = 0; i < data.size(); ++i) {
      data[i] = contextBits < sizeof(T) * 8 ? 1 : 0; // 1: leading bit to indicate the number of used bits
    }
  }

  void operator+=(const uint32_t i) {
    assert(i <= inputMask);
    // note: when the context is fully mature, we need to keep the leading bit in front of the contextbits as the MSB
    T leadingBit = (*ctx) & (1 << contextBits); 
    (*ctx) <<= inputBits;
    (*ctx) |= i | leadingBit;
    (*ctx) &= (1 << (contextBits + 1)) - 1;
  };

  void operator=(const uint32_t i) {
    ctx = &data[i & ctxMask];
  }

  T& operator()() {
    return *ctx;
  };
};


// ===== src/IndirectMap.hpp =====

#include <cassert>
#include <cstdint>

class IndirectMap : IPredictor {
public:
  static constexpr int MIXERINPUTS = 2;

private:
  const Shared * const shared;
  Random rnd;
  Array<uint8_t> data;
  StateMap sm;
  const uint32_t mask;
  const uint32_t maskBits;
  const uint32_t stride;
  const uint32_t bTotal;
  uint32_t b {};
  uint32_t bCount {};
  uint32_t context {};
  uint8_t *cp;
  int scale;

public:
  IndirectMap(const Shared* const sh, int bitsOfContext, int inputBits, int scale, int limit);
  void setDirect(uint32_t ctx);
  void set(uint64_t ctx);
  void update() override;
  void setScale(int Scale);
  void mix(Mixer &m);
};


// ===== src/SmallStationaryContextMap.hpp =====


/**
 * Map for modelling contexts of (nearly-)stationary data.
 * The context is looked up directly. For each bit modelled, a 16bit prediction is stored.
 * The adaptation rate is controlled by the caller, see @ref SmallStationaryContextMap::mix().
 *
 * Uses (2^(BitsOfContext+1))*((2^InputBits)-1) bytes of memory.
 */
class SmallStationaryContextMap : IPredictor {
public:
  static constexpr int MIXERINPUTS = 2;

private:
  const Shared * const shared;
  Array<uint16_t> data;
  const uint32_t mask;
  const uint32_t stride;
  const uint32_t bTotal;
  uint32_t b {};
  uint32_t context {};
  uint32_t bCount {};
  uint16_t *cp {};
  const int rate;
  int scale;

public:
  /**
    * Construct using (2^(BitsOfContext+1))*((2^InputBits)-1) bytes of memory.
    * @param bitsOfContext How many bits to use for each context. Higher bits are discarded.
    * @param inputBits How many bits [1..8] of input are to be modelled for each context. New contexts must be set at those intervals.
    * @param rate
    * @param scale
    */
  SmallStationaryContextMap(const Shared* const sh, int bitsOfContext, int inputBits, int rate, int scale);
  void set(uint32_t ctx);
  void reset();
  void update() override;
  void mix(Mixer &m);
};


// ===== src/StationaryMap.hpp =====


/**
 * Map for modelling contexts of (nearly-)stationary data.
 * The context is looked up directly. For each bit modelled, the exact counts of 0s and 1s are stored.
 * 
 */
class StationaryMap : IPredictor {
public:
  static constexpr int MIXERINPUTS = 3;

private:
  const Shared * const shared;
  Array<uint32_t> data;
  const uint32_t mask, stride, bTotal;
  int scale;
  uint32_t context;
  uint32_t bCount;
  uint32_t b;
  uint32_t *cp;

public:
  /**
    * Construct using (2^(BitsOfContext+2))*((2^InputBits)-1) bytes of memory.
    * @param bitsOfContext How many bits to use for each context. Higher bits are discarded.
    * @param inputBits How many bits [1..8] of input are to be modelled for each context. New contexts must be set at those intervals.
    * @param scale
    */
  StationaryMap(const Shared* const sh, const int bitsOfContext, const int inputBits, const int scale = 64);

  /**
    * ctx must be a direct context (no hash)
    * @param ctx
    */
  void set(uint32_t ctx);
  void setScale(int scale);
  void reset();
  void update() override;
  void mix(Mixer &m);
};


// ===== src/model/Image8BitModel.hpp =====

#include <cstdint>

/**
 * Model for 8-bit image data (grayscale/indexed)
 */
class Image8BitModel
{
private:
  static constexpr int nSM = 2;
  static constexpr int nRM = 74;
  static constexpr int nOLS = 5;
  static constexpr int nPltMaps = 4;
  static constexpr int nCM = 46 + nPltMaps;
  static constexpr int nIM = 5;

public:
  static constexpr int MIXERINPUTS =
    nSM * StationaryMap::MIXERINPUTS +
    (nRM * 3) * ResidualMap::MIXERINPUTS +
    (nOLS * 2) * ResidualMap::MIXERINPUTS +
    nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS) +
    nPltMaps * SmallStationaryContextMap::MIXERINPUTS +
    nIM * IndirectMap::MIXERINPUTS; // 738
  static constexpr int MIXERCONTEXTS = 512 + 16 + 32 + 255 + 1024 + 64 + 128 + 256; /**< 2287 */
  static constexpr int MIXERCONTEXTSETS = 8;

  Shared* const shared;
  ContextMap2 cm;
  StationaryMap map[nSM];
  ResidualMap mapR1, mapR2, mapR3, mapOLS1, mapOLS2;
  SmallStationaryContextMap pltMap[nPltMaps];  /**< palette maps, not used for grayscale images */
  IndirectMap sceneMap[nIM];
  IndirectContext<uint8_t> iCtx[nPltMaps]; /**< palette contexts, not used for grayscale images */
  Array<short> jumps{ 0x8000 };
  //pixel neighborhood
  uint8_t WWWWWW = 0, WWWWW = 0, WWWW = 0, WWW = 0, WW = 0, W = 0;
  uint8_t NWWWW = 0, NWWW = 0, NWW = 0, NW = 0, N = 0, NE = 0, NEE = 0, NEEE = 0, NEEEE = 0;
  uint8_t NNWWW = 0, NNWW = 0, NNW = 0, NN = 0, NNE = 0, NNEE = 0, NNEEE = 0;
  uint8_t NNNWW = 0, NNNW = 0, NNN = 0, NNNE = 0, NNNEE = 0;
  uint8_t NNNNW = 0, NNNN = 0, NNNNE = 0;
  uint8_t NNNNN = 0;
  uint8_t NNNNNN = 0;

  uint8_t res = 0; /**< expected residual */
  uint8_t prvFrmPx = 0; /**< corresponding pixel in previous frame */
  uint8_t prvFrmPrediction = 0; /**< prediction for corresponding pixel in previous frame */
  uint32_t lastPos = 0;
  uint32_t isGray = 0;
  int w = 0;
  int ctx = 0;
  int col = 0;
  int line = 0;
  int x = 0;
  int jump = 0;
  int framePos = 0;
  int prevFramePos = 0;
  int frameWidth = 0;
  int prevFrameWidth = 0;
  int columns[2] = { 1, 1 }, column[2]{};

  uint32_t predictions[nRM + nOLS]{};

  // Per-predictor prediction error for each decoded pixel.
  // Stores uint8 rabs(prediction - actual) for all nRM and nOLS predictors at every pixel position.
  // Read back via PredErr(ctxIndex, relX, relY) to estimate how well each predictor
  // performed on causal neighbors (W, N, NW, NE, WW, NN of the current pixel).
  // The averaged absolute error across those neighbors feeds mapR1's histogram selection,
  // giving it a spatially-local, per-predictor confidence signal:
  // low value = predictor was accurate nearby.
  // Sized in init() to cover PRED_ERR_ROWS rows: nextPowerOf2(PRED_ERR_ROWS * w * nRM).
  static constexpr size_t PRED_ERR_BUF_ROWS = 3; // three rows (including the current row) - we need to reach the prediction error of W, N, NW, NE, WW, NN
  RingBuffer<uint8_t> predErrBuf;

  // Per-pixel accumulated decoding cost (loss), one byte per pixel.
  // Stores (loss >> 3) after each decoded byte, reflecting how hard the last pixel was
  // to compress. Read back via Ls(relX, relY); six causal neighbors (W, N, WW, NN, NW, NE)
  // are summed into lossQ, capturing local image complexity.
  // lossQ feeds mapR2's histogram selection,
  // allowing statistical maps to adapt to smooth vs. noisy/high-frequency regions.
  // Sized in init() to cover LOSS_BUF_ROWS rows: nextPowerOf2(LOSS_BUF_ROWS * w).
  static constexpr size_t LOSS_BUF_ROWS = 3; // three rows (including the current row) - we need to reach the prediction error of NN
  RingBuffer<uint8_t> lossBuf;

  uint32_t loss = 0;  // decoding cost for the current byte, accumulated bit by bit (0..1023 over 8 bits)
  uint32_t lossQ = 0; // sum of lossBuf[] over 6 causal neighbors (W, N, WW, NN, NW, NE), capped at 639;
  // measures local image complexity: low = smooth region, high = noisy/detailed region
  uint8_t lossQ4 = 0; // lossQ quantized to 3 bits

  static constexpr float lambda[nOLS] = { 0.996f, 0.87f, 0.93f, 0.8f, 0.9f };
  static constexpr int num[nOLS] = { 32, 12, 15, 10, 14 };
  static constexpr float nu = 0.001f;
  std::unique_ptr<OLS_float> ols[nOLS];
  std::unique_ptr<OLS_float> sceneOls;

  const uint8_t* olsCtx1[32] = { &WWWWWW, &WWWWW, &WWWW, &WWW, &WW, &W, &NWWWW, &NWWW, &NWW, &NW, &N, &NE, &NEE, &NEEE, &NEEEE, &NNWWW,
                                &NNWW, &NNW, &NN, &NNE, &NNEE, &NNEEE, &NNNWW, &NNNW, &NNN, &NNNE, &NNNEE, &NNNNW, &NNNN, &NNNNE, &NNNNN,
                                &NNNNNN };
  const uint8_t* olsCtx2[12] = { &WWW, &WW, &W, &NWW, &NW, &N, &NE, &NEE, &NNW, &NN, &NNE, &NNN };
  const uint8_t* olsCtx3[15] = { &N, &NE, &NEE, &NEEE, &NEEEE, &NN, &NNE, &NNEE, &NNEEE, &NNN, &NNNE, &NNNEE, &NNNN, &NNNNE, &NNNNN };
  const uint8_t* olsCtx4[10] = { &N, &NE, &NEE, &NEEE, &NN, &NNE, &NNEE, &NNN, &NNNE, &NNNN };
  const uint8_t* olsCtx5[14] = { &WWWW, &WWW, &WW, &W, &NWWW, &NWW, &NW, &N, &NNWW, &NNW, &NN, &NNNW, &NNN, &NNNN };
  const uint8_t** olsCtxs[nOLS] = { &olsCtx1[0], &olsCtx2[0], &olsCtx3[0], &olsCtx4[0], &olsCtx5[0] };

  Image8BitModel(Shared* const sh, uint64_t size);
  uint8_t Ls(int relX, int relY) const;
  uint8_t GetPredErr(uint32_t ctxIndex, int relX, int relY) const;
  uint32_t GetPredErrAvg(const uint32_t predictorIndex) const;
  void MakePrediction(int i, uint8_t base1, uint8_t base2, int prediction);
  void MakePredictionC(int i, int prediction);
  void MakePredictionAvg(int i, int base1, int base2);
  void MakePredictionTrend(int i, int base1, int other1, int base2);
  void MakePredictionSmooth(int i, int base1, int other1, int base2);
  void init(int pos);
  void setParam(int info0, uint32_t gray0);
  void mix(Mixer& m);
};

// ===== src/APM.hpp =====


/**
 * An APM maps a probability and a context to a new probability.
 */
class APM : public AdaptiveMap, IPredictor {
private:
  const int N; /**< Number of contexts */
  const int steps;
  int cxt; /**< context index of last prediction */

public:
  int limit; //1..1023
  /**
    * Creates with @ref n contexts using 4*s*n bytes memory.
    * @param n the number of contexts
    * @param s the number of steps
    */
  APM(const Shared* const sh, int n, int s, int limit);
  ~APM() override = default;

  /**
    * a.update() updates probability map. y=(0..1) is the last bit
    */
  void update() override;

  /**
    * Returns a new probability (0..4095) like with @ref StateMap.
    * cx=(0..n-1) is the context.
    * pr=(0..4095) is considered part of the context.
    * The output is computed by interpolating @ref pr into @ref steps ranges nonlinearly
    * with smaller ranges near the ends. The initial output is pr.
    * lim=(0..1023): set a lower limit (like 255) for faster adaptation.
    * @param pr the initial probability. Considered part of the context.
    * @param cx the context
    * @param lim limit; lower limit means faster adaptation
    * @return the adjusted probability
    */
  int p(int pr, int cx);
};


// ===== src/BH.hpp =====

#include <cstdint>

/**
 * A BH maps a 32 bit hash to an array of b bytes (2-byte checksum and b-2 values)
 * The bucket selector index must be a multiplicative hash.
 *
 * If a collision is detected, up to "searchLimit" nearby locations in the same
 * cache line are tested and the first matching checksum or
 * empty element is returned.
 *
 * Collision detection and resolution policy:
 * 2 byte checksum with LRU replacement
 * If neither a match nor an empty element is found the lowest priority
 * element is replaced by the new element (except last 2 MRU elements).

 * @tparam B
 */
template<uint64_t B>
class BH {
  uint32_t searchLimit = 8;
  Array<uint8_t> t; /**< elements */
  const uint32_t mask; /**< size-1 */
  const int hashBits;

public:
  /**
    * Creates @ref n element table with @ref b bytes each. @ref n must be a power of 2.
    * The first two bytes of each element is reserved for a checksum to detect collisions.
    * The remaining b-2 bytes are values, prioritized by the first value. This byte is 0 to mark an unused element.
    * @param n number of elements in the table
    */
  explicit BH(uint64_t n) : t(n * B), mask(uint32_t(n - 1)), hashBits(ilog2(mask + 1)) {
    assert(B >= 2 && n > 0 && isPowerOf2(n));
  }

  /**
    * Returns a pointer to the ctx'th element, such that
    * t[ctx][-2]..t[ctx][-1] is a checksum of @ref ctx, t[ctx][0] is the priority, and
    * t[ctx][1..B-3] are other values (0-255).
    * @tparam B number of bytes in each slot including the checksum and priority
    * @param ctx bucket selector
    * @return pointer to the ctx'th element
    */
  uint8_t* operator[](uint64_t ctx);
};

template<uint64_t B>
inline uint8_t* BH<B>::operator[](const uint64_t ctx) {
  const uint16_t chk = checksum16(ctx, hashBits);
  const uint32_t i = finalize64(ctx, hashBits) * searchLimit & mask;
  uint8_t *p = nullptr;
  uint16_t *cp = nullptr;
  uint32_t j = 0;
  for( j = 0; j < searchLimit; ++j ) {
    p = &t[(i + j) * B];
    cp = reinterpret_cast<uint16_t *>(p);
    if( p[2] == 0 ) { // prio/state byte is zero -> empty slot
      *cp = chk; //occupy
      break;
    }
    if( *cp == chk ) {
      break; // found
    }
  }
  // move to front what we've found
  if( j == 0 ) {
    return p + 2; // we are already at the front -> nothing to do
  }
  static uint8_t tmp[B]; // temporary array for element to move to front
  if( j == searchLimit ) { // element was not found
    --j; // candidate to overwrite is the last one
    memset(tmp, 0, B); // zero all
    memcpy(tmp, &chk, 2); // 2 checksum bytes
    /*for (int k = j - 1; k > 2; k--) { // try to find a lower priority element, but protect the top 2
      if (t[(i + j) * B + 2] > t[(i + k) * B + 2])
        j = k;
    }*/
    if (searchLimit > 2 && t[(i + j) * B + 2] > t[(i + j - 1) * B + 2]) {
      --j;
    }
  } else { // element was found, or empty slot occupied
    memcpy(tmp, cp, B);
  }
  memmove(&t[(i + 1) * B], &t[i * B], j * B);
  memcpy(&t[i * B], tmp, B);
  return &t[i * B + 2];
}


// ===== src/Mixer_Scalar.hpp =====


class Mixer_Scalar : public Mixer
{
public:
  Mixer_Scalar(const Shared* sh, int n, int m, int s, int promoted);
protected:
  int  dotProduct(const short* t, const short* w, size_t n) override;
  int  dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) override;
  void train(const short* t, short* w, size_t n, int e) override;
};

// ===== src/Mixer_SSE2.hpp =====


#ifdef X64_SIMD_AVAILABLE

class Mixer_SSE2 : public Mixer
{
public:
  Mixer_SSE2(const Shared* sh, int n, int m, int s, int promoted);
protected:
  int  dotProduct(const short* t, const short* w, size_t n) override;
  int  dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) override;
  void train(const short* t, short* w, size_t n, int e) override;
};

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer_AVX2.hpp =====


#ifdef X64_SIMD_AVAILABLE

class Mixer_AVX2 : public Mixer
{
public:
  Mixer_AVX2(const Shared* sh, int n, int m, int s, int promoted);
protected:
  int  dotProduct(const short* t, const short* w, size_t n) override;
  int  dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) override;
  void train(const short* t, short* w, size_t n, int e) override;
};

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer_AVX512.hpp =====


#ifdef X64_SIMD_AVAILABLE

class Mixer_AVX512 : public Mixer
{
public:
  Mixer_AVX512(const Shared* sh, int n, int m, int s, int promoted);
protected:
  int  dotProduct(const short* t, const short* w, size_t n) override;
  int  dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) override;
  void train(const short* t, short* w, size_t n, int e) override;
};

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer_Neon.hpp =====


#ifdef ARM_NEON_AVAILABLE

class Mixer_Neon : public Mixer
{
public:
  Mixer_Neon(const Shared* sh, int n, int m, int s, int promoted);
protected:
  int  dotProduct(const short* t, const short* w, size_t n) override;
  int  dotProduct2(const short* t, const short* w0, const short* w1, size_t n, int& sum1) override;
  void train(const short* t, short* w, size_t n, int e) override;
};

#endif // ARM_NEON_AVAILABLE

// ===== src/MixerFactory.hpp =====


class MixerFactory
{
private:
  const Shared* const shared;
public:
  MixerFactory(const Shared* const sh);
  Mixer* createMixer(int n, int m, int s, int promoted) const;
};

// ===== src/StationaryMapForJpegModel.hpp =====

#include <cassert>
#include <cstdint>
#include <cstring>

/**
 * A bank of maps for contexts whose statistics change slowly (nearly stationary data).
 * For each bit, it counts how many 0s and 1s were seen in the same context, and
 * uses those counts to predict the next bit.
 *
 * One bank serves numContexts contexts from a single shared hash table.
 *
 * Terms used below:
 *
 *   element  The statistics of one context: a small bit tree of
 *            (2^InputBits)-1 counters (each a 16 bit n0 and a 16 bit n1),
 *            plus a 24 bit checksum that identifies the context.
 *   group    ElementsPerGroup elements that share one hash table bucket.
 *            A context may be stored in any element of its group.
 *   chunk    The InputBits bits that are modelled with one context, i.e.
 *            the bits between two calls of set().
 *
 * A group is 256 bytes (four cache lines). Everything needed to find the right
 * element is in the first cache line:
 *
 *   line 0    chkLo[ElementsPerGroup]     low 16 bits of each checksum, 0 = empty element
 *             root[ElementsPerGroup]      counters[0] (the root of the bit tree) of each element
 *             order                       the ranking of the elements, one byte per rank
 *             chkHi[ElementsPerGroup]     high 8 bits of each checksum
 *   line 1-3  tail[ElementsPerGroup][...] counters[1..] of each element
 *
 * So searching the group, comparing checksums and choosing an element to
 * replace all read only line 0, which set() has already prefetched. The lookup
 * prefetches the tail of the chosen element; it is first needed on the second
 * bit of the chunk.
 *
 * Ranking: the elements of a group are ranked from most recently used (rank 0)
 * to least recently used. Only the ranking bytes are reordered; the elements
 * themselves never move. An element keeps its index, checksum, root and tail
 * for as long as it lives. Moving an element to the front is a few shifts on
 * one 64 bit word and copies no counters, so the pointers that ActiveContext
 * keep into the group stay valid for the whole chunk.
 *
 * Lookup: the group is searched in rank order for the context's checksum. If
 * found, that element is moved to the front. If not found, an empty element is
 * used; if there is none, the least used of the lower-ranked elements is
 * replaced. The Protected top-ranked elements are never replaced. This way the
 * element claimed last is always safe, and two contexts that alternate in the
 * same group cannot keep evicting each other. No extra flag is needed for
 * this: the rank alone tells whether an element is protected.
 *
 * Protection also limits the harm when several contexts land in the same group on
 * the same chunk. Right after a context's lookup its element is at rank 0, so with
 * Protected = k it cannot be replaced until k more contexts have looked up the
 * same group. If even more contexts collide, the element may be replaced, and two
 * contexts then share it until the end of the chunk.
 *
 * A replaced element is cleared first, so it only ever holds the statistics of
 * one context: nothing leaks from the contexts that used it before.
 *
 * New elements always go to the front, so in every group all used elements are
 * before all empty ones. The search can therefore stop at the first
 * empty element.
 */
class StationaryMapForJpegModel : IPredictor
{
public:
  static constexpr int MIXERINPUTS = 3; // per context

private:
  static constexpr uint32_t InputBits = 3; // it's specifically designed for the JpegModel -> it's fixed + the memory layout also depends on it, 
  static constexpr uint32_t Counters = (1u << InputBits) - 1; // counters in one bit tree (one element)
  static constexpr uint32_t ElementsPerGroup = 8; // ranked from most to least recently used
  static constexpr uint32_t Protected = 1; // this many top-ranked elements are never replaced
  static constexpr int ChecksumBits = 24;

  static_assert(ElementsPerGroup == 8, "the ranking stores one byte per rank in a 64 bit word");
  static_assert(Protected >= 1 && Protected < ElementsPerGroup, "at least one element must be protected and at least one replaceable");
  static_assert(ChecksumBits == 24, "chkLo + chkHi hold exactly 24 bits");

  /**
   * The starting ranking: element 0 at rank 0, element 1 at rank 1, and so on.
   *
   * A ranking is 8 bytes, one per rank: byte r (counting from the lowest byte)
   * holds the index of the element at rank r. In Identity every byte holds its
   * own position, so reading it byte by byte from the lowest gives 0, 1, ..., 7:
   *
   *   Identity = 0x 07 06 05 04 03 02 01 00
   *   rank:         7  6  5  4  3  2  1  0
   *
   * Every valid ranking is a reordering of these 8 values: each element index
   * appears at exactly one rank.
   *
   * Group::order stores the ranking XORed with Identity, not the ranking itself:
   *
   *   stored  = ranking ^ Identity   (when writing)
   *   ranking = stored  ^ Identity   (when reading; XOR undoes itself)
   *
   * The reason is reset(): it zeroes the whole table with a single memset. For
   * a zeroed group, reading gives 0 ^ Identity = Identity, which is a valid
   * starting ranking. Every element is empty (its chkLo is 0), so the order of
   * the elements does not matter yet, as long as each index appears once.
   *
   * If the ranking were stored directly, a zeroed group would read as
   * 0x0000000000000000: element 0 at every rank. Elements 1 to 7 would then
   * never be found by the search or chosen for replacement, and the group would
   * behave as if it had only one element. Nothing would fail or crash; the
   * model would just silently get worse.
   */
  static constexpr uint64_t Identity = UINT64_C(0x0706050403020100);

  struct alignas(64) Group
  {
    // First cache line: everything the lookup needs.
    uint16_t chkLo[ElementsPerGroup]; // low 16 bits of the checksum; 0 means the element is empty
    uint32_t root[ElementsPerGroup]; // counters[0] of each element: n0 in the high half, n1 in the low half
    uint64_t order; // the ranking XOR Identity; byte r of the ranking is the index of the element at rank r
    uint8_t chkHi[ElementsPerGroup]; // high 8 bits of the checksum; only read when chkLo matches
    // Remaining cache lines: only the tail of the element in use is touched.
    uint32_t tail[ElementsPerGroup][Counters - 1]; // counters[1..] of each element
  };

  static_assert(sizeof(Group) == 256, "unexpected padding in Group");

  /** The state of one context during the current chunk. */
  struct ActiveContext
  {
    Group* group; // the group of the current context
    uint32_t* counter; // the counter of the current bit: set by mix(), updated by update()
    uint32_t* tail; // counters[1] of the element in use; valid after the first mix() of the chunk
    uint32_t chk; // the 24 bit checksum of the current context; its low 16 bits are never 0
  };

  const Shared* const shared;
  Array<Group, 64> data;
  Array<ActiveContext> activeContexts;
  Random rnd;
  const uint32_t numContexts;
  const int groupBits; // log2 of the number of groups
  int scale;
  uint32_t bCount; // bits modelled so far in this chunk; used by asserts only
  uint32_t b; // the node of the bit tree for the current bit
  bool subscribed;

  /** Returns the index of the element at rank r. */
  static ALWAYS_INLINE uint32_t elementAt(const uint64_t ranking, const uint32_t r) {
    return static_cast<uint32_t>(ranking >> (8 * r)) & 0xff;
  }

  /** Returns the ranking with rank r moved to the front; ranks 0..r-1 each move back by one. */
  static ALWAYS_INLINE uint64_t moveToFront(const uint64_t ranking, const uint32_t r) {
    const uint64_t mask = ~UINT64_C(0) >> (8 * (ElementsPerGroup - 1 - r)); // the bytes of ranks 0..r
    return (ranking & ~mask) | ((ranking << 8) & mask) | ((ranking >> (8 * r)) & 0xff);
  }

  /** (Roughly) how many chunks an element has been used for. Elements with fewer uses are replaced first. */
  static ALWAYS_INLINE uint32_t useCount(const uint32_t root) {
    return (root >> 16) + (root & 0xffff); // the root counter is updated exactly once per chunk
  }

  /** Clears element e of group g and assigns it to checksum chk. */
  static void claim(Group* g, uint32_t e, uint32_t chk);

  /** Chooses the rank of the element to replace when the group is full and the context was not found. */
  uint32_t victimRank(const Group* g, uint64_t ranking);

  /** Finds (or claims) the element of ctx's context and points ctx.counter and ctx.tail at it. */
  void lookup(ActiveContext& ctx);

public:
  /**
    * Uses 2^bitsOfMemory bytes of memory in total, shared by all contexts.
    * @param numContexts the number of contexts in this bank
    * @param bitsOfMemory log2 of the table size in bytes
    * @param inputBits must equal InputBits; call set() once every inputBits bits
    * @param scale is input scaling, 64 = neutral
    */
  StationaryMapForJpegModel(const Shared* sh, uint32_t numContexts, int bitsOfMemory, int inputBits, int scale = 64);

  /**
    * Starts a new chunk of InputBits bits for context i and prefetches its group.
    * ctxHash must be a hash, not a raw context value: pass the result of hash() directly.
    */
  void set(uint32_t i, uint64_t ctxHash);

  /**
    * Adds MIXERINPUTS inputs to m for context i and returns a scaled stretched probability.
    */
  int mix(Mixer& m, uint32_t i);

  void setScale(int scale);
  void reset();
  void update() override;
};

// ===== src/model/JpegModel.hpp =====

#include <cstdint>

// Check that the data is valid JPEG. If it is not, silently stop decoding the
// image: an embedded image hands over to the image that contains it, the
// top-level image to the other models.
// Only for use in JpegModel::update() (it needs pos, and returns from it),
// which never touches a Mixer.
#define JASSERT(x) \
  if (!(x)) { \
    if (idx > 0) { \
      finishImage(pos); \
    } \
    else { \
      images[idx].jpeg = 0; \
    } \
    return bail(); \
  }

// One entry of a Huffman decoding table: the codes of one length.
// huf[Tc][Th][m] holds the smallest code of length m+1, the largest code of
// that length + 1, and where their symbols start in hBuf.
// Tc: 0 = DC, 1 = AC. Th: table number 0-3. m: 0-15.
struct HUF
{
  uint32_t min, max;
  int val;
};

struct JPEGImage
{
  uint32_t offset, // position of the SOI marker
    jpeg, // 0: no JPEG, 1: in the headers, 2: SOS seen and data starts
    nextJpeg, // jpeg > 1, updated at every byte boundary
    app, // bytes to skip before parsing resumes: the rest of the current marker
    // segment plus the next marker's 4 byte header, so that the parser sees
    // that header complete in buf(4)..buf(1)
    sof, sos, data, // positions in buf of the SOF marker, the SOS marker and the image data
    dri, // position in buf of the DRI marker, 0 if none
    htSize; // number of entries in ht
  int ht[8]; // positions in buf of the DHT markers
  uint8_t qTable[256]; // the 4 quantization tables, 64 entries each, every entry stored minus 1
  int qMap[10]; // block of the MCU -> quantization table number
};

/**
 * The result of JpegModel::update() for the current bit. It tells the caller
 * which mixer to use, and so which models give inputs to it. Each mixer always
 * gets the same sequence of add() and set() calls.
 */
enum class JpegResult
{
  NoJpeg,      // not in a JPEG stream (any more): use the general mixer and the general models
  Special,     // in a JPEG stream, but JpegModel does not model this bit (stuffing, marker, restart
  // padding, or the rest of a byte after decoding stopped): add specialInput() to the
  // mixer instead of calling mix()
  EntropyCoded // in the entropy coded image data: call mix()
};

/**
 * Model for JPEG images. update() finds JPEG streams and tells whether the
 * current bit is entropy coded; mix() adds the mixer inputs for such bits.
 * The model partly decodes the image, and uses the decoded coefficients as
 * context for the Huffman coded symbols.
 * Only baseline and 8 bit extended Huffman coded DCT images are supported.
 */
class JpegModel
{
private:
  static constexpr int N = 42; // number of bit history contexts
  static constexpr int nSM = 86; // number of stationary maps

public:
  // 2 per bit history context + 1 per stationary map + 17 at the end of mix()
  // (smx, 2 from the inner mixer, 14 APM corrections)
  static constexpr int MIXERINPUTS = 2 * N + nSM + 17;
  // Must match the m.set() calls at the end of mix(), in number and in size.
  static constexpr int MIXERCONTEXTSETS = 13;
  static constexpr int MIXERCONTEXTS =
    8192 + // symbol and coefficient
    8192 + // symbol and frequency
    1024 + // predicted size of the coefficient
    4096 + // size of the in-block neighbours
    512 +  // where the next larger coefficient is expected
    24 +   // number of missing in-block neighbours
    48 +   // position inside the symbol
    64 +   // does the prediction reach the announced size category
    192 +  // coefficient the decoder is at
    48 +   // how many contexts are still unseen
    192 +  // coefficient this bit belongs to
    192 +  // frequency and position inside the symbol
    256;   // coefficient this bit belongs to, and component

  // What specialInput() and specialContext() give to the caller's Special mixer.
  // The context is c0 (the partial byte, 1..255) plus an offset for the kind of bit:
  //   0: byte stuffing and marker bits
  //   256: restart interval padding
  //   512: the rest of a byte after decoding stopped (see bail())
  static constexpr int SPECIALMIXERINPUTS = 1;
  static constexpr int SPECIALMIXERCONTEXTS = 768;
  static constexpr int SPECIALMIXERCONTEXTSETS = 1;

private:
  //////////////////////////////////////////////////////////////////////////////
  // Parser state
  //////////////////////////////////////////////////////////////////////////////

  Random rnd;
  enum
  {
    SOF0 = 0xc0, SOF1, SOF2, SOF3, DHT, RST0 = 0xd0, SOI = 0xd8, EOI, SOS, DQT, DNL, DRI, APP0 = 0xe0, COM = 0xfe, FF
  }; // the second byte of the 2 byte markers
  static const int maxEmbeddedLevel = 3; // a JPEG may contain a thumbnail, which may contain another one
  JPEGImage images[maxEmbeddedLevel]{};
  int idx = -1; // the image in images[] being decoded; -1 means "not initialized yet"
  uint32_t lastPos = 0; // pos at the last byte boundary; a jump means other data came in between

  // for parsing quantization tables
  int dqtState = -1;
  uint32_t dqtEnd = 0, qNum = 0, qNum16 = 0, qNum16b = 0;

  // zigzag position -> u (horizontal frequency)
  static constexpr uint8_t zzu[64] = {
        0, 1, 0, 0, 1, 2, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 6, 5, 4, 3, 2, 1, 0, 1, 2, 3, 4, 5, 6, 7,
        7, 6, 5, 4, 3, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 5, 6, 7, 7, 6, 7 };
  // zigzag position -> v (vertical frequency)
  static constexpr uint8_t zzv[64] = {
        0, 0, 1, 2, 1, 0, 0, 1, 2, 3, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 5, 4, 3, 2, 1, 0, 0, 1, 2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 2, 1,
        2, 3, 4, 5, 6, 7, 7, 6, 5, 4, 3, 4, 5, 6, 7, 7, 6, 5, 6, 7, 7 };

  //////////////////////////////////////////////////////////////////////////////
  // Huffman decoder state
  //////////////////////////////////////////////////////////////////////////////

  uint32_t huffCode = 0; // the bits of the current symbol so far: Huffman code and extra bits
  uint32_t huffBits = 0; // number of bits in huffCode
  uint32_t huffSize = 0; // length of the Huffman code alone, without the extra bits
  // The decoded symbol (RS), or -1 while its Huffman code is still being read.
  // AC: the high 4 bits are r, the number of zeros before the next nonzero
  // coefficient; the low 4 bits are s, the number of extra bits after the
  // code, which give the value of that coefficient. ZRL (F0) is 16 zeros.
  // DC: rs is s alone.
  int rs = -1;
  int hbCount = 2; // which of the 3 bits of the current bit history group we are on (see mix()); init value is 2 so that mix() does a lookup

  // Position in the MCU (0-639). The low 6 bits are the coefficient in zigzag
  // order (0 = DC, 1-63 = AC); the higher bits are the block within the MCU,
  // which selects the Huffman tables.
  uint32_t mcuPos = 0;

  Array<HUF> huf{ 128 }; // Tc*64 + Th*16 + m -> min, max, val (see HUF)
  int mcuSize = 0; // number of coefficients in an MCU
  int hufSel[2][10]{ {0} }; // DC/AC, block of the MCU -> Huffman table number
  Array<uint8_t> hBuf{ 2048 }; // Tc*1024 + Th*256 + index -> symbol (RS)

  //////////////////////////////////////////////////////////////////////////////
  // Image geometry and decoded coefficients
  //////////////////////////////////////////////////////////////////////////////

  Array<uint32_t> color{ 10 }; // block of the MCU -> component (0-3)
  Array<int> pred{ 4 }; // component -> DC value of its last block (DC values are coded as differences from it)
  int dc = 0; // DC value of the current block
  int width = 0; // image width in MCUs
  int row = 0, column = 0; // current MCU (column 0 to width-1)
  Array<int> ls{ 10 }; // block of the MCU -> distance in coefficients to the previous block of the same component
  // block of the MCU -> distance in coefficients to the west / north block of the same component
  Array<int> blockW{ 10 }, blockN{ 10 };
  Array<int> samplingFactors{ 4 }; // component, as its index in the scan (SOS) header -> sampling factors (H in the high 4 bits, V in the low 4 bits)
  Array<int> zPos{ 64 }; // u + 8v -> zigzag position

  // Circular buffer of the decoded coefficients, in a compact 8 bit form:
  // DC: (dc + 1023) >> 3, i.e. [-1023..1024] -> [0..255].
  // AC: the zeros of a run: (zeros left in the run, counting down to 1) << 4 | s.
  //     The coefficient after them: s << 4 | 0b1100 | its first 2 extra bits.
  //     It is nonzero, except after ZRL (F0): then it is the 16th zero, stored
  //     as 0b1100. Apart from that, 0b11 in bits 2-3 only occurs for a nonzero
  //     coefficient, because s <= 10.
  RingBuffer<uint8_t> coefficientBuffer{ 0x20000 };
  RingBuffer<int> cBuf2{ 0x20000 }; // the same coefficients as plain values
  int cPos = 0; // write position in coefficientBuffer and cBuf2
  int rs1 = 0; // the last completed symbol (RS)
  int resetPos = 0; // MCU index where the current restart interval started
  int resetLen = 0; // restart interval in MCUs: from DRI, then measured at each RSTn; 0 if unknown
  // Sums of s (the extra bit counts, a rough measure of how many bits a block takes):
  int sSum = 0;  // of the current block so far
  int sSum1 = 0; // of the luma blocks of this MCU completed so far
  int sSum2 = 0; // sSum1, or at the first block of an MCU, sSum3
  int sSum3 = 0; // of the last completed luma block

  //////////////////////////////////////////////////////////////////////////////
  // Predictions for the coefficient being decoded
  //
  // All of these are set by updatePredictions(), when a symbol completes, and
  // describe the next coefficient.
  //////////////////////////////////////////////////////////////////////////////

  // The north (sumU) and west (sumV) blocks projected onto this one, minus the
  // coefficients of this block decoded so far (each is removed as soon as it
  // is known). Indexed by u (sumU) and v (sumV).
  Array<int> sumU{ 8 }, sumV{ 8 };
  // What the neighbour blocks say this coefficient should be:
  // 0 from the west, 2 from the north, 1 and 3 from both.
  Array<int> advPred{ 4 };
  // How many zigzag positions ahead a coefficient larger than this one is
  // expected (* 2, + 1 if positive); 0 if none within 9 positions.
  // 0..2 use the same neighbours as advPred[0..2]; 3..5 the same with a larger threshold.
  // At the image border only runPred[0..2] (and advPred[0..2]) are replaced;
  // runPred[3..5] and advPred1/2/3[] are not.
  Array<int> runPred{ 6 };
  // The same as advPred[], one, two and three zigzag positions ahead. They are
  // computed by the runPred[] loop anyway, and are read by the stationary maps.
  Array<int> advPred1{ 3 }, advPred2{ 3 }, advPred3{ 3 };
  // The already decoded neighbours of this coefficient within the block, each
  // rescaled to this coefficient's quantization step, as a signed log. A
  // nonzero value gets +17 more, so that after the coarse divisions in the
  // contexts (lcp / 22 etc.) small values do not fall into the same bucket as 0.
  // 65535 when that neighbour would be outside the block.
  // With (u, v) the frequencies of this coefficient:
  //   lcp[0] (u-1, v)    lcp[1] (u, v-1)
  //   lcp[2] (u-2, v)    lcp[3] (u, v-2)
  //   lcp[4] (u-1, v-1)  lcp[5] (0, v)    lcp[6] (u, 0)
  // lcp[4..6] are 65535 unless u > 0 and v > 0.
  Array<int> lcp{ 7 };
  // The same coefficient in the blocks of this MCU before this one, from the
  // other components. For DC, the coded difference where it is known.
  //   prevCoef:   average of the previous component's blocks (Y for Cb, Cb
  //               for Cr) as a signed log, plus the number of blocks averaged
  //               << 20. The count is 0 for luma, which has no previous
  //               component, and 1 or more with subsampling; in the high bits
  //               it survives the divisions in the contexts.
  //   prevCoef2:  average of the luma blocks, as a signed log. Only for
  //               components 2 and up; for component 1 prevCoef already is luma.
  //   prevCoefRs: the coefficientBuffer entry of the previous component's
  //               blocks, r and s averaged separately; without such blocks,
  //               the entry 64 positions back.
  int prevCoef = 0, prevCoef2 = 0, prevCoefRs = 0;

  // Which neighbour blocks of the current block are inside the image.
  // bit 0: west (sumV[] is built from it), bit 1: north (sumU[]).
  // 3 everywhere except at the left and top border.
  uint32_t nbAvail = 3;

  // Masks of the predictions that carry no information. They are used as
  // mixer contexts, so the mixer can use different weights when those inputs
  // are empty.
  uint32_t lma = 0; // bit i set when lcp[i] is missing (== 65535); 7 bits
  uint32_t ama = 0; // bit i set when advPred[i] is zero; 3 bits

  // advPred[] and lcp[] for the coefficient the current bit really belongs to.
  // They differ from the arrays above only during the extra bits of an AC
  // symbol with a run of zeros: mcuPos still points at the first zero, but the
  // extra bits describe the nonzero coefficient r positions later.
  // alignPredictors() moves the predictions there as soon as rs is decoded.
  // Also, alAdv[3] leaves out a neighbour block outside the image; advPred[3]
  // does not.
  // Read by the APM keys and some mixer contexts. cxt[] and the stationary maps
  // still read advPred[] and lcp[].
  int alAdv[4]{};
  int alLcp[7]{}; // Same layout as lcp[]

  //////////////////////////////////////////////////////////////////////////////
  // Context model
  //////////////////////////////////////////////////////////////////////////////

  // Context hash -> bit histories. To save hash lookups, a context is looked
  // up only once every 3 bits (see hbCount). The slot it returns holds 7 bit
  // histories, one for each node of the 3 bit tree: context + {"", 0, 00, 01, 1, 10, 11}.
  BH<9> t;
  Array<uint64_t> cxt{ N }; // context hashes
  Array<uint8_t*> cp{ N }; // pointers to the bit history of each context for the current bit
  // Keyed by the position in the image (MCU, row, column) and the code so far.
  // Pays off for MJPEG, where the next frame often repeats the same position.
  IndirectMap MJPEGMap;
  StationaryMapForJpegModel sMap;
  StateMap sm; // bit history -> probability, one per context in cxt[]
  // A direct probability for the symbol state and the coefficient, next to
  // the inner mixer; one input of the outer mixer.
  StateMap smx;
  Mixer* m1; // the inner mixer
  APM apm1, apm2, apm3, apm4, apm5, apm6, apm7, apm8, apm9, apm10, apm11, apm12, apm13, apm14;
  Ilog* ilog = &Ilog::getInstance();
  Shared* const shared;

  //////////////////////////////////////////////////////////////////////////////
  // Values computed by update() for the current bit and used by mix()
  //
  // Only valid when the last update() returned JpegResult::EntropyCoded.
  //////////////////////////////////////////////////////////////////////////////

  uint32_t comp = 0; // color component of the current block
  uint32_t coef = 0; // zigzag position | component << 6
  uint32_t hc = 0; // the Huffman code so far, plus the DC/AC and luma flags
  uint32_t symKey = 0; // like hc, but shorter once the symbol is known (see update())
  int zu = 0, zv = 0; // horizontal / vertical frequency of the current coefficient
  bool firstCol = false; // no block to the west
  uint32_t phase = 0; // 0: Huffman code, 1: sign bit, 2: magnitude bits
  uint32_t predCat = 0; // predicted size category of the coefficient, 0..15
  int alZz = 0; // zigzag position of the coefficient the current bit belongs to
  int coldCount = 0; // how many of the N bit history contexts have never been seen

  int specialPrediction = 0; // mixer input to use when update() returns Special
  int specialCtx = 0; // mixer context to use when update() returns Special

  // True when cp[] points at the bit histories used for the previous
  // prediction. The next update() call must train them with the actual bit.
  // Note: if a non-JPEG block comes between two JPEG blocks, we are not called
  // in between, so the first update() of the second block trains cp[] with an
  // unrelated bit. That is only a few wrong updates per stream, and not worth
  // a check on every bit. A block-level reset would fix it, if one is ever added.
  bool predicted = false;

  JpegResult lastResult = JpegResult::NoJpeg; // what update() returned for the current bit

  JpegResult setResult(const JpegResult result) {
    lastResult = result;
    return result;
  }

  // Called by update() when JpegModel does not model this bit. If the current
  // byte started in the image data, stay on the Special mixer until the end of
  // the byte, so the mixer only changes at a byte boundary. Otherwise hand the
  // bit to the general models.
  JpegResult bail() {
    specialCtx = 512 + shared->State.c0;
    return setResult(images[idx].nextJpeg != 0 ? JpegResult::Special : JpegResult::NoJpeg);
  }

  /** Stop decoding the current image and go back to the image that contains
   * it (or to the general models when there is none). The containing image
   * then skips the bytes of this one as part of the marker segment they are in.
   */
  void finishImage(uint32_t pos);

  /** Load the standard Huffman tables, for an image that has none of its own. */
  void loadStandardHuffmanTables();

  /** Sign-preserving log: sign(x) * ilog(|x| + 1). */
  int signedLog(const int x) const {
    return (x < 0 ? -1 : +1) * ilog->log(abs(x) + 1);
  }

  /** Like signedLog(), with 17 more for a nonzero value (see lcp[]). */
  int neighbourLog(const int x) const {
    return (x < 0 ? -1 : +1) * (ilog->log(abs(x) + 1) + (x != 0 ? 17 : 0));
  }

  /**
   * The extrapolation from both neighbour blocks (advPred[3]) for zigzag
   * position zz2 of the current block, before the log scaling and before the
   * DC correction. Only uses the neighbour blocks that avail marks (same bits
   * as nbAvail): with only one, it uses that one doubled, like advPred[0] and
   * advPred[2]; with none, it returns 0. sumU[] and sumV[] must be up to date.
   */
  int extrapolateDct(int zz2, int q, uint32_t avail) const;

  /**
   * Fill out[0..6] with the in-block neighbours of zigzag position zz2 in the
   * layout of lcp[], while the decoder is at position zz (zz <= zz2). The
   * neighbours from zz on are inside a run of zeros: they are 0, and cBuf2
   * does not hold them yet.
   */
  void inBlockNeighbours(int zz, int zz2, int q, int* out) const;

  /**
   * Compute everything the model knows about the next coefficient: from the
   * neighbour blocks, from the coefficients of this block decoded so far, and
   * from the same coefficient in the other color components. Call it when a
   * symbol completes.
   */
  void updatePredictions();

  /**
   * Recompute alAdv[] and alLcp[] for the nonzero coefficient after a run of
   * r zeros starting at mcuPos. Call it as soon as rs is known, before its
   * extra bits are predicted.
   */
  void alignPredictors(int r);

  /**
   * Add the inputs of bit history context i: 2 to m and 1 to m1, always,
   * whatever the state is. Also counts the contexts never seen before.
   * cp[i] must already point at the bit history for this bit.
   */
  void addContextInputs(Mixer& m, const int i) {
    const uint8_t s = *cp[i];
    if (s == 0) {
      // Never seen before. Add 0 instead of a stretched probability near 2048:
      // a context that has not learned anything yet adds more noise than
      // information. This is what makes the higher order contexts worth having
      // on small files.
      sm.skip(i);
      m.add(0);
      m.add(0);
      m1->add(0);
      ++coldCount;
    }
    else {
      const int p = sm.p2(i, s);
      m.add((p - 2048) >> 3);
      // A state that has seen only one bit gets half the usual weight.
      int st = stretch(p);
      st >>= (1 + static_cast<int>(s <= 2));
      m.add(st);
      m1->add(st);
    }
  }

public:
  explicit JpegModel(Shared* const sh, const MixerFactory* const mf, uint64_t size);
  ~JpegModel();
  JpegModel(const JpegModel&) = delete; // owns m1
  JpegModel& operator=(const JpegModel&) = delete;

  /**
   * Parse and Huffman-decode the JPEG stream up to the current bit, train the
   * bit histories used for the previous prediction, and set
   * shared->State.JPEG.state. Never touches a Mixer.
   */
  JpegResult update();

  /**
   * Add the mixer inputs and contexts for an entropy coded bit. Only valid
   * right after update() returned JpegResult::EntropyCoded. Always makes
   * exactly MIXERINPUTS add() calls and MIXERCONTEXTSETS set() calls.
   */
  void mix(Mixer& m);

  /** The mixer input to use when update() returned Special. */
  int specialInput() const {
    assert(lastResult == JpegResult::Special);
    return specialPrediction;
  }

  /** The mixer context to use when update() returned Special, in 0..SPECIALMIXERCONTEXTS-1. */
  int specialContext() const {
    assert(lastResult == JpegResult::Special);
    return specialCtx;
  }
};

// ===== src/HashElementForMatchPositions.hpp =====

#include <cstdint>

struct HashElementForMatchPositions {
  static constexpr size_t N = 3;
  uint32_t matchPositions[N];
  void Add(uint32_t pos) {
    if (N > 1) {
      memmove(&matchPositions[1], &matchPositions[0], (N - 1) * sizeof(matchPositions[0]));
    }
    matchPositions[0] = pos;
  }
};

static_assert(sizeof(HashElementForMatchPositions) == 3 * 4);

// ===== src/model/MatchInfo.hpp =====


struct MatchInfo {
  uint32_t length = 0;
  uint32_t index = 0;
  uint32_t lengthBak = 0;
  uint32_t indexBak = 0;
  uint8_t expectedByte = 0;
  bool delta = false;

  bool isInNoMatchMode() const;
  bool isInPreRecoveryMode() const;
  bool isInRecoveryMode() const;
  uint32_t recoveryModePos() const;
  uint64_t prio() const;
  bool isBetterThan(const MatchInfo* other) const;

  void update(Shared* shared, uint32_t minlen_rm);
  void registerMatch(uint32_t pos, uint32_t LEN, uint32_t LEN1);
};


// ===== src/model/MatchModel.hpp =====



/**
 * Predict the next bit based on a preceding long matching byte sequence
 *
 * This model monitors byte sequences and keeps their most recent positions in a hashtable.
 * When the current byte sequence matches an older sequence (having the same hash) the model predicts the forthcoming bits.
 */
class MatchModel {
private:
  static constexpr int nCM = 2;
  static constexpr int nST = 3;
  static constexpr int nLSM = 1;
  static constexpr int nSM = 1;
  Shared * const shared;
  Array<HashElementForMatchPositions> hashtable;
  StateMap stateMaps[nST];
  ContextMap2 cm;
  LargeStationaryMap mapL;
  StationaryMap map[nSM];
  static constexpr uint32_t iCtxBits = 6;
  IndirectContext<uint8_t> iCtx;
  uint32_t ctx[nST] {0};
  const int hashBits;
  Ilog *ilog = &Ilog::getInstance();

  static constexpr int MINLEN_RM = 3; //minimum length in recovery mode before we "fully recover"
  static constexpr int LEN1 = 5;  //note: this length is modelled by NormalModel
  static constexpr int LEN2 = 7;  //note: this length is *not* modelled by NormalModel
  static constexpr int LEN3 = 9;  //note: this length is *not* modelled by NormalModel

  static constexpr size_t N = 4; // maximum number of match candidates
  Array<MatchInfo> matchCandidates{ N };
  uint32_t numberOfActiveCandidates = 0;

  bool isMatch(const uint32_t pos, const uint32_t MINLEN) const;
  void AddCandidates(HashElementForMatchPositions* matches, uint32_t LEN);

public:
  static constexpr int MIXERINPUTS = 
    2 + // direct mixer inputs based on expectedBit
    nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS) + 
    nST * 2 +
    nLSM * LargeStationaryMap::MIXERINPUTS +
    nSM * StationaryMap::MIXERINPUTS; // 24
  static constexpr int MIXERCONTEXTS = 20;
  static constexpr int MIXERCONTEXTSETS = 1;
  MatchModel(Shared* const sh, const uint64_t hashtablesize, const uint64_t mapmemorysize);
  void update();
  void mix(Mixer &m);
};


// ===== src/StateMapPair.hpp =====


/**
 * A @ref StateMapPair maps a context to two probabilities.
 */
class StateMapPair : AdaptiveMap, IPredictor {
private:
  uint32_t cxt{};

public:
  int limit_slow; //1..1023, optimal:  1023
  int limit_fast; //1..1023, optimal: 4..64

  /**
   * Creates a @ref StateMap with @ref n contexts using 4*2*n bytes memory.
   * @param n number of contexts
   * @param lim_slow
   * @param lim_fast
   */
  StateMapPair(const Shared* const sh, int n, int lim_slow, int lim_fast);
  ~StateMapPair() override = default;

  void reset();
  void update() override;

  void p(uint32_t cx, int& p_slow, int& p_fast);
};


// ===== src/model/NormalModel.hpp =====


/**
 * Model for order 0-14 contexts
 * Contexts are hashes of previous 0..14 bytes.
 * Order 0..6, 8 and 14 are used for prediction.
 * Note: order 7+ contexts are modeled by matchModel as well.
 */
class NormalModel {
private:
  static constexpr int nCM = 9;
  static constexpr int nSM = 2*4;
  Shared * const shared;
  ContextMap2 cm;
  StateMapPair smOrder0;
  StateMapPair smOrder1;
public:
  static constexpr int MIXERINPUTS =
    nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_RUN_STATS + ContextMap2::MIXERINPUTS_BYTE_HISTORY) + 
    nSM; //71
  static constexpr int MIXERCONTEXTS_PRE = 64;
  static constexpr int MIXERCONTEXTS_POST = 1024 + 256 + 512 + 256 + 256 + 1536; //3840
  static constexpr int MIXERCONTEXTSETS_PRE = 1;
  static constexpr int MIXERCONTEXTSETS_POST = 6;
  NormalModel(Shared* const sh, const uint64_t cmSize);
  void reset();

  /**
    * update order 1..14 context hashes.
    * Note: order 0 context does not need an update so its hash never changes.
    */
  void updateHashes();
  void mix(Mixer &m);

  /**
    * setting more mixer contexts in the generic case (i.e. not using the special models such as image, audio, jpeg models)
    * @param m
    */
  void mixPost(Mixer &m);
};


// ===== src/model/IContextModel.hpp =====

class IContextModel {
public:
  virtual int p() = 0;
};


// ===== src/Models.hpp =====


/**
 * This is a factory class for lazy object creation for models.
 * Objects created within this class are instantiated on first use and guaranteed to be destroyed.
 */
class Models {
private:
  Shared* const shared;
  const MixerFactory* const mixerFactory;
public:
  explicit Models(Shared* const sh, MixerFactory* mf);
  auto normalModel() -> NormalModel&;
  auto matchModel() -> MatchModel&;
  auto jpegModel() -> JpegModel&;
  auto image24BitModel() -> Image24BitModel&;
  auto image8BitModel() -> Image8BitModel&;
  auto image4BitModel() -> Image4BitModel&;
  auto image1BitModel() -> Image1BitModel&;
};


// ===== src/APM1.hpp =====

#include <cstdint>
#include <cassert>

/**
 * APM1 maps a probability and a context into a new probability that bit y will next be 1.
 * After each guess it updates its state to improve future guesses.
 * Probabilities are scaled 12 bits (0-4095).
 */
class APM1 : IPredictor {
private:
  const Shared * const shared;
  int index; /**< last p, context */
  const int n; /**< number of contexts */
  Array<uint16_t> t; /**< [n][33]:  p, context -> p */
  const int rate;

public:
  /**
    * Creates an instance with @ref n contexts and learning rate @ref r.
    * @ref r determines the learning rate. Smaller = faster, must be in the range (0, 32).
    * Uses 66*n bytes memory.
    * @param n the number of contexts
    * @param r the learning rate
    */
  APM1(const Shared* const sh, int n, int r);
  /**
    * Returns adjusted probability in context @ref cx (0 to n-1).
    * @param pr initial (pre-adjusted) probability
    * @param cxt the context
    * @return adjusted probability
    */
  int p(int pr, int cxt);
  void update() override;
};


// ===== src/APMPost.hpp =====

#include <cstdint>
#include <cassert>

/**
 * APMPost maps a 12-bit probability (0-4095) to an n-bit probability (as defined by the Arithmetic Encoder)
 * After each guess it updates its state to improve future guesses.
 */
class APMPost : IPredictor {
private:
  const Shared * const shared;
  uint32_t index; /**< last p, context */
  const uint32_t n; /**< number of contexts */
  Array<uint64_t> t;

public:
  /**
    * Creates an instance with @ref n contexts.
    * @param n the number of contexts
    */
  APMPost(const Shared* sh, uint32_t n);
  /**
    * Returns adjusted probability in context @ref cx (0 to n-1).
    * @param pr initial (pre-adjusted) probability
    * @param cxt the context
    * @return adjusted probability
    */
  uint32_t p(uint32_t pr, uint32_t cxt);
  void update() override;

  void print() {
    for (int i = 0; i < 4096; i++) {
      printf("%d\t%d\t%d\n",i,(int)(t[i]>>32), (int)(t[i]&0xffffffff));
    }
  }
};


// ===== src/SSE.hpp =====


/**
 * Filter the context model with APMs
 */
class SSE
{
private:
  Shared* const shared;
  struct
  {
    struct
    {
      APM APMs[4];
      APM1 APM1s[2];
      APMPost APMPostA, APMPostB;
    } Color, Palette;
    struct
    {
      APM APMs[3];
      APMPost APMPostA, APMPostB;
    } Gray;
    struct
    {
      APM APMs[4];
      APM1 APM1s[3];
      APMPost APMPostFinal;
    } Bilevel;
  } Image;
  struct
  {
    APM APMs[1];
    APMPost APMPostA, APMPostB;
  } Jpeg;
  struct
  {
    APM APMs[4];
    APM1 APM1s[3];
    APMPost APMPostA, APMPostB;
  } Generic;

public:
  explicit SSE(Shared* const sh);
  uint32_t p(uint32_t pr_orig);
};


// ===== src/model/ContextModel.hpp =====


/**
 * This combines all the context models with a Mixer.
 */
class ContextModel {
  Shared * const shared;
  Models * const models;
  const MixerFactory* const mixerFactory;

  IContextModel* selectedContextModel = nullptr;

public:
  ContextModel(Shared* const sh, Models* const models, const MixerFactory* const mixerFactory);
  int p();
};


// ===== src/PredictorMain.hpp =====


/**
 * A Predictor which estimates the probability that the next bit of uncompressed data is 1 
 * in the main data stream with the help of model(s), mixer(s) and an SSE stage. 
 */
class PredictorMain : public Predictor {
private:
  Models *models;
  MixerFactory* mixerFactory;
  ContextModel* contextModel;
  SSE sse;
public:
  PredictorMain(Shared* const sh);
  ~PredictorMain();
  uint32_t p();
};


// ===== src/model/ContextModelBlock.hpp =====


class ContextModelBlock {

private:
  Shared* const shared;
  Mixer* m;

public:
  ContextModelBlock(Shared* const sh, const MixerFactory* const mf);
  int p();

  ~ContextModelBlock();

};


// ===== src/PredictorBlock.hpp =====


/**
 * A Predictor which estimates the probability that the next bit of metadata is 1 
 * with the help of a model and a mixer.
 * Metadata = (block type, block info and block size) 
 */
class PredictorBlock : public Predictor {
private:
  MixerFactory mixerFactory;
  ContextModelBlock contextModelBlock;
public:
  PredictorBlock(Shared* const sh);
  uint32_t p();
};


// ===== src/Encoder.hpp =====


typedef enum {
    COMPRESS, DECOMPRESS
} Mode;

/**
 * An Encoder encodes or decodes bytes using a Predictor and arithmetic encoding.
 * If shared->level is 0, then data is stored in 'archive' without arithmetic coding.
 */
class Encoder {
private:
  ArithmeticEncoder ari;
  const Mode mode; /**< Compress or decompress? */
  File* archive; /**< Compressed data file */
  File *alt; /**< decompressByte() source in COMPRESS mode */
  float p1 {}, p2 {}; /**< percentages for progress indicator: 0.0 .. 1.0 */
  bool doEncoding; /**<  false when compression level is 0 */

  void updateModels(Predictor* predictor, uint32_t p, int y);

public:

  Predictor *predictorMain;
  Predictor *predictorBlock;

  /**
    * Encoder(COMPRESS, f) creates encoder for compression to archive @ref f, which
    * must be open past any header for writing in binary mode.
    * Encoder(DECOMPRESS, f) creates encoder for decompression from archive @ref f,
    * which must be open past any header for reading in binary mode.
    * @param m the mode to operate in
    * @param f the file to read from or write to
    */
  Encoder(Predictor* predictorBlock, Predictor* predictorMain, bool doEncoding, Mode m, File *f);
  Mode getMode() const;

  /**
    * Returns current length of archive
    * @return length of archive so far
    */
  uint64_t size() const;

  /**
    * Should be called exactly once after compression is done and
    * before closing @ref f. It does nothing in DECOMPRESS mode.
    */
  void flush();

  /**
    * Sets alternate source to @ref f for decompressByte() in COMPRESS mode (for testing transforms).
    * @param f
    */
  void setFile(File *f);
    
  /**
    * compressByte(c) in COMPRESS mode compresses one byte.
    * @param c the byte to be compressed
    */
  void compressByte(Predictor *predictor, uint8_t c);

  /**
    * decompressByte() in DECOMPRESS mode decompresses and returns one byte.
    * @return the decompressed byte
    */
  uint8_t decompressByte(Predictor *predictor);

  void initContextForBlockModel(BlockType blockType, int blockInfo);
  void setContextForBlockModel(uint8_t context);
  void appendToBlockTypeHistoryForBlockModel(BlockType blockType);

  void setStatusRange(float perc1, float perc2);
  void printStatus(uint64_t n, uint64_t size) const;
  void printStatus() const;
};

// ===== src/file/FileName.hpp =====


/**
 * A class to represent a filename.
 */
class FileName : public String {
public:
  explicit FileName(const char *s = "");
  int lastSlashPos() const;
  void keepFilename();
  void keepPath();

  /**
    * Prepare path string for screen output.
    */
  void replaceSlashes();
};


// ===== src/file/fileUtils2.hpp =====


/**
 * Verify that the specified file exists and is readable, determine file size
 * @todo Large file support
 * @param filename
 * @return
 */
static uint64_t getFileSize(const char *filename) {
  FileDisk f;
  f.open(filename, true);
  f.setEnd();
  const uint64_t fileSize = f.curPos();
  f.close();
  if((fileSize >> 31) != 0 ) {
    quit("Large files not supported.");
  }
  return fileSize;
}

static void appendToFile(const char *filename, const char *s) {
  FILE *f = openFile(filename, APPEND);
  if( f == nullptr ) {
    printf("Warning: Could not log compression results to %s\n", filename);
  } else {
    fprintf(f, "%s", s);
    fclose(f);
  }
}


// ===== src/file/FileTmp.hpp =====


/**
 * This class is responsible for temporary files in RAM or on disk
 * Initially it uses RAM for temporary file content.
 * In case of the content size in RAM grows too large, it is written to disk,
 * the RAM is freed and all subsequent file operations will use the file on disk.
 */
class FileTmp : public File {
private:
  Array<uint8_t> *contentInRam; /**< content of file */
  uint64_t filePos;
  uint64_t fileSize;
  FileDisk *fileOnDisk;

  void forgetContentInRam();
  void forgetFileOnDisk();

#ifdef NDEBUG
  static constexpr uint32_t MAX_RAM_FOR_TMP_CONTENT = 64 * 1024 * 1024; //64 MB (per file)
#else
  static constexpr uint32_t MAX_RAM_FOR_TMP_CONTENT = 64; // to trigger switching to disk earlier - for testing
#endif
  /**
    * switch: ram->disk
    */
  void ramToDisk();
public:
  FileTmp();
  ~FileTmp() override;

  /**
    *  This method is forbidden for temporary files.
    */
  bool open(const char * /*filename*/, bool /*mustSucceed*/) override;

  /**
    *  This method is forbidden for temporary files.
    */
  void create(const char * /*filename*/) override;
  void close() override;
  int getchar() override;
  void putChar(uint8_t c) override;
  uint64_t  blockRead(uint8_t *ptr, uint64_t count) override;
  void blockWrite(uint8_t *ptr, uint64_t count) override;
  void setpos(uint64_t newPos) override;
  void setEnd() override;
  uint64_t curPos() override;
  bool  eof() override;
};


// ===== src/filter/Filter.hpp =====

#include <cstdint>

enum class FMode {
    FDECOMPRESS, FCOMPARE, FDISCARD
};

class Filter {
protected:
  Encoder *encoder = nullptr;
public:
  virtual void encode(File *in, File *out, uint64_t size, int info, int &headerSize) = 0;
  virtual uint64_t decode(File *in, File *out, FMode fMode, uint64_t size, uint64_t &diffFound) = 0;

  void setEncoder(Encoder &en) {
    encoder = &en;
  }

  virtual ~Filter() = default;
};


// ===== src/filter/TextParserStateInfo.hpp =====

#include <cstdint>

struct TextParserStateInfo {
public:
  uint64_t Start{0};
  uint64_t End{UINT64_MAX}; /**< position of last char with a valid UTF8 state: marks the end of the detected TEXT block */
  uint32_t invalidCount{}; /**< adaptive count of invalid UTF8 sequences seen recently */
  uint8_t EOLType{0}; /**< 0: none or CR-only;   1: CRLF-only (applicable to EOL transform);   2: mixed or LF-only */
  uint8_t UTF8State{0}; /**< state of utf8 parser; 0: valid;  12: invalid;  any other value: yet uncertain (more bytes must be read) */

  static constexpr uint32_t TEXT_FRAGMENT_MIN_SIZE = 1024;/**< size of minimum allowed text block (in bytes) */
  static constexpr uint32_t TEXT_SMALL_BLOCK_SIZE = 128; /**< size of minimum allowed error-free text block (in bytes) */
  static constexpr uint32_t TEXT_MAX_MISSES = 8; /**< threshold: max allowed number of invalid UTF8 sequences seen recently before reporting "fail" */
  static constexpr uint32_t TEXT_ADAPT_RATE = 256; /**< smaller (like 32) = illegal sequences are allowed to come more often, larger (like 1024) = more rigorous detection */

  /**
    * UTF8 validator
    * Based on: http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
    * Control characters (0x00-0x1f) and 0x7f are not allowed (except for tab/cr/lf)
    */
  constexpr static int utf8Accept = 0;
  constexpr static int utf8Reject = 12;

  constexpr static uint8_t utf8StateTable[] = {
          // byte -> character class
          // character_class = utf8StateTable[byte]
          12,12,12,12,12,12,12,12,12,0, 0,12,12, 0,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12,12, // 00..1f
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 20..3f
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, // 40..5f
          0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,12, // 60..7f
          1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, // 80..9f
          7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, // a0..bf
          8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // c0..df
          10, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 4, 3, 3, 11, 6, 6, 6, 5, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, // e0..ff
          // validating automaton
          // new_state = utf8StateTable[256 + old_state + character_class]
           0, 12, 24, 36, 60, 96, 84, 12, 12, 12, 48, 72, // state  0-11
          12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12, // state 12-23
          12,  0, 12, 12, 12, 12, 12,  0, 12,  0, 12, 12, // state 24-35
          12, 24, 12, 12, 12, 12, 12, 24, 12, 24, 12, 12, // state 36-47
          12, 12, 12, 12, 12, 12, 12, 24, 12, 12, 12, 12, // state 48-59
          12, 24, 12, 12, 12, 12, 12, 12, 12, 24, 12, 12, // state 60-71
          12, 12, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 72-83
          12, 36, 12, 12, 12, 12, 12, 36, 12, 36, 12, 12, // state 84-95
          12, 36, 12, 12, 12, 12, 12, 12, 12, 12, 12, 12 // state 96-108
  };
  void reset(uint64_t startPos);
  uint64_t realLength() const;
  bool isLargeText() const;
  bool isSmallText() const;
};

// ===== src/filter/gif.hpp =====

#include <cstdint>


#define LZW_TABLE_SIZE 9221

#define LZW_FIND(k) \
  { \
    offset     = ( int ) finalize64((k) * PHI64, 13); \
    int stride = (offset > 0) ? LZW_TABLE_SIZE - offset : 1; \
    while (true) { \
      if ((index = table[offset]) < 0) { \
        index = -offset - 1; \
        break; \
      } else if (dict[index] == int(k)) { \
        break; \
      } \
      offset -= stride; \
      if (offset < 0) \
        offset += LZW_TABLE_SIZE; \
    } \
  }

#define LZW_RESET \
  { \
    for (int i = 0; i < LZW_TABLE_SIZE; table[i] = -1, i++) \
      ; \
  }

static int encodeGif(File *in, File *out, uint64_t len, int &headerSize) {
  int codeSize = in->getchar();
  int diffPos = 0;
  int clearPos = 0;
  int bsize = 0;
  int code = 0;
  int offset = 0;
  uint64_t beginIn = in->curPos();
  uint64_t beginOut = out->curPos();
  Array<uint8_t> output(4096);
  headerSize = 6;
  out->putChar(headerSize >> 8);
  out->putChar(headerSize & 255);
  out->putChar(bsize);
  out->putChar(clearPos >> 8);
  out->putChar(clearPos & 255);
  out->putChar(codeSize);
  Array<int> table(LZW_TABLE_SIZE);
  for( int phase = 0; phase < 2; phase++ ) {
    in->setpos(beginIn);
    int bits = codeSize + 1;
    int shift = 0;
    int buffer = 0;
    int blockSize = 0;
    int maxcode = (1u << codeSize) + 1;
    int last = -1;
    Array<int> dict(4096);
    LZW_RESET
    bool end = false;
    while((blockSize = in->getchar()) > 0 && in->curPos() - beginIn < len && !end ) {
      for( int i = 0; i < blockSize; i++ ) {
        buffer |= in->getchar() << shift;
        shift += 8;
        while( shift >= bits && !end ) {
          code = buffer & ((1u << bits) - 1);
          buffer >>= bits;
          shift -= bits;
          if((bsize == 0) && code != (1u << codeSize)) {
            headerSize += 4;
            out->put32(0);
          }
          if( bsize == 0 ) {
            bsize = blockSize;
          }
          if( code == (1 << codeSize)) {
            if( maxcode > (1 << codeSize) + 1 ) {
              if((clearPos != 0) && clearPos != 69631 - maxcode ) {
                return 0;
              }
              clearPos = 69631 - maxcode;
            }
            bits = codeSize + 1, maxcode = (1u << codeSize) + 1, last = -1;
            LZW_RESET
          } else if( code == (1u << codeSize) + 1 ) {
            end = true;
          } else if( code > maxcode + 1 ) {
            return 0;
          } else {
            int j = (code <= maxcode ? code : last);
            int size = 1;
            while( j >= (1 << codeSize)) {
              output[4096 - (size++)] = dict[j] & 255;
              j = dict[j] >> 8;
            }
            output[4096 - size] = j;
            if( phase == 1 ) {
              out->blockWrite(&output[4096 - size], size);
            } else {
              diffPos += size;
            }
            if( code == maxcode + 1 ) {
              if( phase == 1 ) {
                out->putChar(j);
              } else {
                diffPos++;
              }
            }
            if( last != -1 ) {
              if( ++maxcode >= 8191 ) {
                return 0;
              }
              if( maxcode <= 4095 ) {
                int key = (static_cast<uint32_t>(last) << 8) + j;
                int index = -1;
                LZW_FIND(key)
                dict[maxcode] = key;
                table[(index < 0) ? -index - 1 : offset] = maxcode;
                if( phase == 0 && index > 0 ) {
                  headerSize += 4;
                  j = diffPos - size - static_cast<int>(code == maxcode);
                  out->put32(j);
                  diffPos = size + static_cast<int>(code == maxcode);
                }
              }
              if( maxcode >= ((1 << bits) - 1) && bits < 12 ) {
                bits++;
              }
            }
            last = code;
          }
        }
      }
    }
  }
  diffPos = static_cast<int>(out->curPos());
  out->setpos(beginOut);
  out->putChar(headerSize >> 8);
  out->putChar(headerSize & 255);
  out->putChar(255 - bsize);
  out->putChar((clearPos >> 8) & 255);
  out->putChar(clearPos & 255);
  out->setpos(diffPos);
  return static_cast<int>(in->curPos() - beginIn == len - 1);
}

#define GIF_WRITE_BLOCK(count) \
  { \
    output[0] = (count); \
    if (mode == FMode::FDECOMPRESS) \
      out->blockWrite(&output[0], (count) + 1); \
    else if (mode == FMode::FCOMPARE) \
      for (int j = 0; j < (count) + 1; j++) \
        if (output[j] != out->getchar() && ! diffFound) { \
          diffFound = outsize + j + 1; \
          return 1; \
        } \
    outsize += (count) + 1; \
    blockSize = 0; \
  }

#define GIF_WRITE_CODE(c) \
  { \
    buffer += (c) << shift; \
    shift += bits; \
    while (shift >= 8) { \
      output[++blockSize] = buffer & 255; \
      buffer >>= 8; \
      shift -= 8; \
      if (blockSize == bsize) GIF_WRITE_BLOCK(bsize); \
    } \
  }

static int decodeGif(File *in, uint64_t size, File *out, FMode mode, uint64_t &diffFound) {
  int diffCount = in->getchar();
  int curDiff = 0;
  Array<int> diffPos(4096);
  diffCount = ((diffCount << 8) + in->getchar() - 6) / 4;
  int bsize = 255 - in->getchar();
  int clearPos = in->getchar();
  clearPos = (clearPos << 8) + in->getchar();
  clearPos = (69631 - clearPos) & 0xffff;
  int codesize = in->getchar();
  int bits = codesize + 1;
  int shift = 0;
  int buffer = 0;
  int blockSize = 0;
  if( diffCount > 4096 || clearPos <= (1 << codesize) + 2 ) {
    return 1;
  }
  int maxcode = (1u << codesize) + 1;
  int input = 0;
  int code = 0;
  int offset = 0;
  Array<int> dict(4096);
  Array<int> table(LZW_TABLE_SIZE);
  LZW_RESET
  for( int i = 0; i < diffCount; i++ ) {
    diffPos[i] = in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    diffPos[i] = (diffPos[i] << 8) + in->getchar();
    if( i > 0 ) {
      diffPos[i] += diffPos[i - 1];
    }
  }
  Array<uint8_t> output(256);
  size -= 6 + diffCount * 4;
  int last = in->getchar();
  int total = static_cast<int>(size) + 1;
  int outsize = 1;
  if( mode == FMode::FDECOMPRESS ) {
    out->putChar(codesize);
  } else if( mode == FMode::FCOMPARE ) {
    if( codesize != out->getchar() && (diffFound == 0)) {
      diffFound = 1;
    }
  }
  if( diffCount == 0 || diffPos[0] != 0 ) GIF_WRITE_CODE(1u << codesize) else {
    curDiff++;
  }
  while( size != 0 && (input = in->getchar()) != EOF) {
    size--;
    int key = (static_cast<uint32_t>(last) << 8) + input;
    int index = (code = -1);
    if( last < 0 ) {
      index = input;
    } else LZW_FIND(key)
    code = index;
    if( curDiff < diffCount && total - static_cast<int>(size) > diffPos[curDiff] ) {
      curDiff++, code = -1;
    }
    if( code < 0 ) {
      GIF_WRITE_CODE(last)
      if( maxcode == clearPos ) {
        GIF_WRITE_CODE(1u << codesize)
        bits = codesize + 1, maxcode = (1u << codesize) + 1;
        LZW_RESET
      } else {
        ++maxcode;
        if( maxcode <= 4095 ) {
          dict[maxcode] = key;
          table[(index < 0) ? -index - 1 : offset] = maxcode;
        }
        if( maxcode >= (1 << bits) && bits < 12 ) {
          bits++;
        }
      }
      code = input;
    }
    last = code;
  }
  GIF_WRITE_CODE(last)
  GIF_WRITE_CODE((1u << codesize) + 1)
  if( shift > 0 ) {
    output[++blockSize] = buffer & 255;
    if( blockSize == bsize ) GIF_WRITE_BLOCK(bsize)
  }
  if( blockSize > 0 ) GIF_WRITE_BLOCK(blockSize)
  if( mode == FMode::FDECOMPRESS ) {
    out->putChar(0);
  } else if( mode == FMode::FCOMPARE ) {
    if( 0 != out->getchar() && (diffFound == 0)) {
      diffFound = outsize + 1;
    }
  }
  return outsize + 1;
}


// ===== src/filter/LZWEntry.hpp =====

struct LZWentry {
  short prefix;
  short suffix;
};


// ===== src/filter/LZWDictionary.hpp =====

#include <cstdint>

class LZWDictionary {
private:
  static constexpr int hashSize = 9221;
  LZWentry dictionary[4096] {};
  short table[hashSize] {};
  uint8_t buffer[4096] {};

public:
  int index;
  LZWDictionary();
  void reset();
  int findEntry(int prefix, int suffix);
  void addEntry(int prefix, int suffix, int offset = -1);
  int dumpEntry(File *f, int code);
};


// ===== src/filter/lzw.hpp =====


#define LZW_RESET_CODE 256
#define LZW_EOF_CODE 257


class LZWFilter : Filter {
public:
  void encode(File *in, File *out, uint64_t  /*size*/, int  /*info*/, int & /*headerSize*/) override {
    LZWDictionary dic;
    int parent = -1;
    int code = 0;
    int buffer = 0;
    int bitsPerCode = 9;
    int bitsUsed = 0;
    bool done = false;
    while( !done ) {
      buffer = in->getchar();
      if( buffer < 0 ) {
        return;// 0;
      }
      for( int j = 0; j < 8; j++ ) {
        code += code + ((buffer >> (7 - j)) & 1), bitsUsed++;
        if( bitsUsed >= bitsPerCode ) {
          if( code == LZW_EOF_CODE ) {
            done = true;
            break;
          }
          if( code == LZW_RESET_CODE ) {
            dic.reset();
            parent = -1;
            bitsPerCode = 9;
          } else {
            if( code < dic.index ) {
              if( parent != -1 ) {
                dic.addEntry(parent, dic.dumpEntry(out, code));
              } else {
                out->putChar(code);
              }
            } else if( code == dic.index ) {
              int a = dic.dumpEntry(out, parent);
              out->putChar(a);
              dic.addEntry(parent, a);
            } else {
              return;// 0;
            }
            parent = code;
          }
          bitsUsed = 0;
          code = 0;
          if((1u << bitsPerCode) == dic.index + 1 && dic.index < 4096 ) {
            bitsPerCode++;
          }
        }
      }
    }
    // return 1;
  }

  uint64_t decode(File * /*in*/, File * /*out*/, FMode  /*fMode*/, uint64_t  /*size*/, uint64_t & /*diffFound*/) override {
    return 0;
  }

};

static int encodeLzw(File *in, File *out, uint64_t size, int &headerSize) {
  LZWDictionary dic;
  int parent = -1;
  int code = 0;
  int buffer = 0;
  int bitsPerCode = 9;
  int bitsUsed = 0;
  bool done = false;
  while( !done ) {
    buffer = in->getchar();
    if( buffer < 0 ) {
      return 0;
    }
    for( int j = 0; j < 8; j++ ) {
      code += code + ((buffer >> (7 - j)) & 1), bitsUsed++;
      if( bitsUsed >= bitsPerCode ) {
        if( code == LZW_EOF_CODE ) {
          done = true;
          break;
        }
        if( code == LZW_RESET_CODE ) {
          dic.reset();
          parent = -1;
          bitsPerCode = 9;
        } else {
          if( code < dic.index ) {
            if( parent != -1 ) {
              dic.addEntry(parent, dic.dumpEntry(out, code));
            } else {
              out->putChar(code);
            }
          } else if( code == dic.index ) {
            int a = dic.dumpEntry(out, parent);
            out->putChar(a);
            dic.addEntry(parent, a);
          } else {
            return 0;
          }
          parent = code;
        }
        bitsUsed = 0;
        code = 0;
        if((1 << bitsPerCode) == dic.index + 1 && dic.index < 4096 ) {
          bitsPerCode++;
        }
      }
    }
  }
  return 1;
}

static inline void writeCode(File *f, const FMode mode, int *buffer, uint64_t *pos, int *bitsUsed, const int bitsPerCode, const int code,
                             uint64_t *diffFound) {
  *buffer <<= bitsPerCode;
  *buffer |= code;
  (*bitsUsed) += bitsPerCode;
  while((*bitsUsed) > 7 ) {
    const uint8_t b = *buffer >> (*bitsUsed -= 8);
    (*pos)++;
    if( mode == FMode::FDECOMPRESS ) {
      f->putChar(b);
    } else if( mode == FMode::FCOMPARE && b != f->getchar()) {
      *diffFound = *pos;
    }
  }
}

static uint64_t decodeLzw(File *in, File *out, FMode mode, uint64_t &diffFound) {
  LZWDictionary dic;
  uint64_t pos = 0;
  int parent = -1;
  int code = 0;
  int buffer = 0;
  int bitsPerCode = 9;
  int bitsUsed = 0;
  writeCode(out, mode, &buffer, &pos, &bitsUsed, bitsPerCode, LZW_RESET_CODE, &diffFound);
  while((code = in->getchar()) >= 0 && diffFound == 0 ) {
    int index = dic.findEntry(parent, code);
    if( index < 0 ) { // entry not found
      writeCode(out, mode, &buffer, &pos, &bitsUsed, bitsPerCode, parent, &diffFound);
      if( dic.index > 4092 ) {
        writeCode(out, mode, &buffer, &pos, &bitsUsed, bitsPerCode, LZW_RESET_CODE, &diffFound);
        dic.reset();
        bitsPerCode = 9;
      } else {
        dic.addEntry(parent, code, index);
        if( dic.index >= (1 << bitsPerCode)) {
          bitsPerCode++;
        }
      }
      parent = code;
    } else {
      parent = index;
    }
  }
  if( parent >= 0 ) {
    writeCode(out, mode, &buffer, &pos, &bitsUsed, bitsPerCode, parent, &diffFound);
  }
  writeCode(out, mode, &buffer, &pos, &bitsUsed, bitsPerCode, LZW_EOF_CODE, &diffFound);
  if( bitsUsed > 0 ) { // flush buffer
    const uint8_t b = uint8_t(buffer << (8 - bitsUsed));  // shift remaining bits to MSB
    pos++;
    if( mode == FMode::FDECOMPRESS ) {
      out->putChar(b);
    } else if( mode == FMode::FCOMPARE && b != out->getchar()) {
      diffFound = pos;
    }
  }
  return pos;
}

// ===== src/filter/png.hpp =====

#include <cstdint>

/**
 * 8/24/32-bit png image data encode/decode
 * filter bytes from individual lines go to a separate header
 */
class PngFilter : public Filter {
private:
  int stride = 3; //1: Gray/Indexed, 3: RGB, 4: RGBA
  int width = 0;
public:

  void setWidth(int w) {
    this->width = w;
  }
  void setStride(int stride) {
    this->stride = stride;
  }

  void encode(File *in, File *out, uint64_t size, int width, int & headerSize) override {
    int lineWidth = width + 1; //including filter byte
    headerSize = static_cast<int>(size / lineWidth); // = number of rows
    RingBuffer<uint8_t> filterBuffer(nextPowerOf2(headerSize));
    RingBuffer<uint8_t> pixelBuffer(nextPowerOf2(size - headerSize));
    assert(filterBuffer.size() >= headerSize);
    assert(pixelBuffer.size() >= size - headerSize);
    for( int line = 0; line < headerSize; line++ ) {
      uint8_t filter = in->getchar();
      filterBuffer.add(filter);
      for (int x = 0; x < width; x++) {
        uint8_t c1 = in->getchar();
        switch (filter) {
          case 0: {
            break;
          }
          case 1: {
            c1=(static_cast<uint8_t>(c1 + (x < stride ? 0 : pixelBuffer(stride))));
            break;
          }
          case 2: {
            c1=(static_cast<uint8_t>(c1 + (line == 0 ? 0 : pixelBuffer(width))));
            break;
          }
          case 3: {
            c1 = (static_cast<uint8_t>(c1 + (((line == 0 ? 0 : pixelBuffer(width)) + (x < stride ? 0 : pixelBuffer(stride))) >> 1)));
            break;
          }
          case 4: {
            c1 = (static_cast<uint8_t>(c1 + paeth(
              x < stride ? 0 : pixelBuffer(stride),
              line == 0 ? 0 : pixelBuffer(width),
              line == 0 || x < stride ? 0 : pixelBuffer(width + stride))));
            break;
          }
          default:
            //fail: unexpected filter code.
            return;
        }
        pixelBuffer.add(c1);
      }
    }
    uint32_t len1 = filterBuffer.getpos();
    uint32_t len2 = pixelBuffer.getpos();
    for (uint32_t i = 0; i < len1; i++)
      out->putChar(filterBuffer[i]);
    for (uint32_t i = 0; i < len2; i++)
      out->putChar(pixelBuffer[i]);
  }

  uint64_t decode(File *in, File *out, FMode fMode, uint64_t size, uint64_t &diffFound) override {
    int lineWidth = width + 1; //including filter byte
    int headerSize = static_cast<int>(size / lineWidth); // = number of rows
    RingBuffer<uint8_t> filterBuffer(nextPowerOf2(headerSize));
    RingBuffer<uint8_t> pixelBuffer(nextPowerOf2(size - headerSize));
    assert(filterBuffer.size() >= headerSize);
    assert(pixelBuffer.size() >= size - headerSize);
    for (int line = 0; line < headerSize; line++) {
      uint8_t filter = in->getchar();
      filterBuffer.add(filter);
    }
    uint32_t p = 0;
    for (int line = 0; line < headerSize; line++) {
      uint8_t filter = filterBuffer[line];
      if (fMode == FMode::FDECOMPRESS) {
        out->putChar(filter);
      }
      else if (fMode == FMode::FCOMPARE) {
        p++;
        if (filter != out->getchar() && (diffFound == 0)) {
          diffFound = p;
        }
      }
      for (int x = 0; x < width; x++) {
        uint8_t c1 = in->getchar();
        uint8_t c = c1;
        switch (filter) {
          case 0: {
            break;
          }
          case 1: {
            c1 = (static_cast<uint8_t>(c1 - (x < stride ? 0 : pixelBuffer(stride))));
            break;
          }
          case 2: {
            c1 = (static_cast<uint8_t>(c1 - (line == 0 ? 0 : pixelBuffer(width))));
            break;
          }
          case 3: {
            c1 = (static_cast<uint8_t>(c1 - (((line == 0 ? 0 : pixelBuffer(width)) + (x < stride ? 0 : pixelBuffer(stride))) >> 1)));
            break;
          }
          case 4: {
            c1 = (static_cast<uint8_t>(c1 - paeth(
              x < stride ? 0 : pixelBuffer(stride),
              line == 0 ? 0 : pixelBuffer(width),
              line == 0 || x < stride ? 0 : pixelBuffer(width + stride))));
            break;
          }
          default:
            //fail: unexpected filter code.
            break;
        }
        pixelBuffer.add(c);
        if (fMode == FMode::FDECOMPRESS) {
          out->putChar(c1);
        }
        else if (fMode == FMode::FCOMPARE) {
          p++;
          if (c1 != out->getchar() && (diffFound == 0)) {
            diffFound = p;
          }
        }
      }
    }
    return size;
  }
};


// ===== src/filter/zlib.hpp =====

#include <zlib.h>

static int parseZlibHeader(int header) {
  switch( header ) {
    case 0x2815:
      return 0;
    case 0x2853:
      return 1;
    case 0x2891:
      return 2;
    case 0x28cf:
      return 3;
    case 0x3811:
      return 4;
    case 0x384f:
      return 5;
    case 0x388d:
      return 6;
    case 0x38cb:
      return 7;
    case 0x480d:
      return 8;
    case 0x484b:
      return 9;
    case 0x4889:
      return 10;
    case 0x48c7:
      return 11;
    case 0x5809:
      return 12;
    case 0x5847:
      return 13;
    case 0x5885:
      return 14;
    case 0x58c3:
      return 15;
    case 0x6805:
      return 16;
    case 0x6843:
      return 17;
    case 0x6881:
      return 18;
    case 0x68de:
      return 19;
    case 0x7801:
      return 20;
    case 0x785e:
      return 21;
    case 0x789c:
      return 22;
    case 0x78da:
      return 23;
    default:
      return -1;
  }
}

static int zlibInflateInit(z_streamp strm, int zh) {
  if( zh == -1 ) {
    return inflateInit2(strm, -MAX_WBITS);
  }
  return inflateInit(strm);
}

MTFList mtf(81);

static int encodeZlib(File *in, File *out, uint64_t len, int &headerSize) {
  const int block = 1u << 16;
  const int limit = 128;
  uint8_t zin[block * 2];
  uint8_t zOut[block];
  uint8_t zRec[block * 2];
  uint8_t diffByte[81 * limit];
  uint64_t diffPos[81 * limit];

  // Step 1 - parse offset type form zlib stream header
  uint64_t posBackup = in->curPos();
  uint32_t h1 = in->getchar();
  uint32_t h2 = in->getchar();
  in->setpos(posBackup);
  int zh = parseZlibHeader(h1 * 256 + h2);
  int memLevel = 0;
  int cLevel = 0;
  int cType = zh % 4;
  int window = zh == -1 ? 0 : MAX_WBITS + 10 + zh / 4;
  int minCLevel = window == 0 ? 1 : cType == 3 ? 7 : cType == 2 ? 6 : cType == 1 ? 2 : 1;
  int maxCLevel = window == 0 ? 9 : cType == 3 ? 9 : cType == 2 ? 6 : cType == 1 ? 5 : 1;
  int index = -1;
  int nTrials = 0;
  bool found = false;

  // Step 2 - check recompressibility, determine parameters and save differences
  z_stream mainStrm;
  z_stream recStrm[81];
  int diffCount[81];
  int recPos[81];
  int mainRet = Z_STREAM_END;
  mainStrm.zalloc = Z_NULL;
  mainStrm.zfree = Z_NULL;
  mainStrm.opaque = Z_NULL;
  mainStrm.next_in = Z_NULL;
  mainStrm.avail_in = 0;
  if( zlibInflateInit(&mainStrm, zh) != Z_OK ) {
    return 0;
  }
  for( int i = 0; i < 81; i++ ) {
    cLevel = (i / 9) + 1;
    // Early skip if invalid parameter
    if( cLevel < minCLevel || cLevel > maxCLevel ) {
      diffCount[i] = limit;
      continue;
    }
    memLevel = (i % 9) + 1;
    recStrm[i].zalloc = Z_NULL;
    recStrm[i].zfree = Z_NULL;
    recStrm[i].opaque = Z_NULL;
    recStrm[i].next_in = Z_NULL;
    recStrm[i].avail_in = 0;
    int ret = deflateInit2(&recStrm[i], cLevel, Z_DEFLATED, window - MAX_WBITS, memLevel, Z_DEFAULT_STRATEGY);
    diffCount[i] = (ret == Z_OK) ? 0 : limit;
    recPos[i] = block * 2;
    diffPos[i * limit] = 0xFFFFFFFFFFFFFFFF;
    diffByte[i * limit] = 0;
  }

  for( uint64_t i = 0; i < len; i += block ) {
    uint32_t blSize = min(uint32_t(len - i), block);
    nTrials = 0;
    for( int j = 0; j < 81; j++ ) {
      if( diffCount[j] >= limit ) {
        continue;
      }
      nTrials++;
      if( recPos[j] >= block ) {
        recPos[j] -= block;
      }
    }
    // early break if nothing left to test
    if( nTrials == 0 ) {
      break;
    }
    memmove(&zRec[0], &zRec[block], block);
    memmove(&zin[0], &zin[block], block);
    in->blockRead(&zin[block], blSize); // Read block from input file

    // Decompress/inflate block
    mainStrm.next_in = &zin[block];
    mainStrm.avail_in = blSize;
    do {
      mainStrm.next_out = &zOut[0];
      mainStrm.avail_out = block;
      mainRet = inflate(&mainStrm, Z_FINISH);
      nTrials = 0;

      // Recompress/deflate block with all possible parameters
      for( int j = mtf.getFirst(); j >= 0; j = mtf.getNext()) {
        if( diffCount[j] >= limit ) {
          continue;
        }
        nTrials++;
        recStrm[j].next_in = &zOut[0];
        recStrm[j].avail_in = block - mainStrm.avail_out;
        recStrm[j].next_out = &zRec[recPos[j]];
        recStrm[j].avail_out = block * 2 - recPos[j];
        int ret = deflate(&recStrm[j], mainStrm.total_in == len ? Z_FINISH : Z_NO_FLUSH);
        if( ret != Z_BUF_ERROR && ret != Z_STREAM_END && ret != Z_OK ) {
          diffCount[j] = limit;
          continue;
        }

        // Compare
        int end = 2 * block - static_cast<int>(recStrm[j].avail_out);
        int tail = max(mainRet == Z_STREAM_END ? static_cast<int>(len) - static_cast<int>(recStrm[j].total_out) : 0, 0);
        for( int k = recPos[j]; k < end + tail; k++ ) {
          if((k < end && i + k - block < len && zRec[k] != zin[k]) || k >= end ) {
            if( ++diffCount[j] < limit ) {
              const int p = j * limit + diffCount[j];
              diffPos[p] = i + k - block;
              assert(k < int(sizeof(zin) / sizeof(*zin)));
              diffByte[p] = zin[k];
            }
          }
        }
        // Early break on perfect match
        if( mainRet == Z_STREAM_END && diffCount[j] == 0 ) {
          index = j;
          found = true;
          break;
        }
        recPos[j] = 2U * block - recStrm[j].avail_out;
      }
    } while( mainStrm.avail_out == 0 && mainRet == Z_BUF_ERROR && nTrials > 0 );
    if((mainRet != Z_BUF_ERROR && mainRet != Z_STREAM_END) || nTrials == 0 ) {
      break;
    }
  }
  int minCount = (found) ? 0 : limit;
  for( int i = 80; i >= 0; i-- ) {
    cLevel = (i / 9) + 1;
    if( cLevel >= minCLevel && cLevel <= maxCLevel ) {
      deflateEnd(&recStrm[i]);
    }
    if( !found && diffCount[i] < minCount ) {
      minCount = diffCount[index = i];
    }
  }
  inflateEnd(&mainStrm);
  if( minCount == limit ) {
    return 0;
  }
  mtf.moveToFront(index);

  // Step 3 - write parameters, differences and precompressed (inflated) data
  out->putChar(diffCount[index]);
  out->putChar(window);
  out->putChar(index);
  for( int i = 0; i <= diffCount[index]; i++ ) {
    const int v = i == diffCount[index] ? int(len - diffPos[index * limit + i]) :
                  int(diffPos[index * limit + i + 1] - diffPos[index * limit + i]) - 1;
    out->put32(v);
  }
  for( int i = 0; i < diffCount[index]; i++ ) {
    out->putChar(diffByte[index * limit + i + 1]);
  }

  in->setpos(posBackup);
  mainStrm.zalloc = Z_NULL;
  mainStrm.zfree = Z_NULL;
  mainStrm.opaque = Z_NULL;
  mainStrm.next_in = Z_NULL;
  mainStrm.avail_in = 0;
  if( zlibInflateInit(&mainStrm, zh) != Z_OK ) {
    return 0;
  }
  for( uint64_t i = 0; i < len; i += block ) {
    uint32_t blSize = min(uint32_t(len - i), block);
    in->blockRead(&zin[0], blSize);
    mainStrm.next_in = &zin[0];
    mainStrm.avail_in = blSize;
    do {
      mainStrm.next_out = &zOut[0];
      mainStrm.avail_out = block;
      mainRet = inflate(&mainStrm, Z_FINISH);
      out->blockWrite(&zOut[0], block - mainStrm.avail_out);
    } while( mainStrm.avail_out == 0 && mainRet == Z_BUF_ERROR);
    if( mainRet != Z_BUF_ERROR && mainRet != Z_STREAM_END ) {
      break;
    }
  }
  inflateEnd(&mainStrm);
  headerSize = diffCount[index] * 5 + 7;
  return static_cast<int>(mainRet == Z_STREAM_END);
}

static int decodeZlib(File *in, uint64_t size, File *out, FMode mode, uint64_t &diffFound) {
  const int block = 1u << 16;
  const int limit = 128;
  uint8_t zin[block];
  uint8_t zOut[block];
  int diffCount = min(in->getchar(), limit - 1);
  int window = in->getchar() - MAX_WBITS;
  int index = in->getchar();
  int memLevel = (index % 9) + 1;
  int cLevel = (index / 9) + 1;
  int len = 0;
  int diffPos[limit];
  diffPos[0] = -1;
  for( int i = 0; i <= diffCount; i++ ) {
    int v = in->get32();
    if( i == diffCount ) {
      len = v + diffPos[i];
    } else {
      diffPos[i + 1] = v + diffPos[i] + 1;
    }
  }
  uint8_t diffByte[limit];
  diffByte[0] = 0;
  for( int i = 0; i < diffCount; i++ ) {
    diffByte[i + 1] = in->getchar();
  }
  size -= 7 + 5 * diffCount;

  z_stream recStrm;
  int diffIndex = 1;
  int recPos = 0;
  recStrm.zalloc = Z_NULL;
  recStrm.zfree = Z_NULL;
  recStrm.opaque = Z_NULL;
  recStrm.next_in = Z_NULL;
  recStrm.avail_in = 0;
  int ret = deflateInit2(&recStrm, cLevel, Z_DEFLATED, window, memLevel, Z_DEFAULT_STRATEGY);
  if( ret != Z_OK ) {
    return 0;
  }
  for( uint64_t i = 0; i < size; i += block ) {
    uint32_t blSize = min(uint32_t(size - i), block);
    in->blockRead(&zin[0], blSize);
    recStrm.next_in = &zin[0];
    recStrm.avail_in = blSize;
    do {
      recStrm.next_out = &zOut[0];
      recStrm.avail_out = block;
      ret = deflate(&recStrm, i + blSize == size ? Z_FINISH : Z_NO_FLUSH);
      if( ret != Z_BUF_ERROR && ret != Z_STREAM_END && ret != Z_OK ) {
        break;
      }
      const int have = min(block - recStrm.avail_out, len - recPos);
      while( diffIndex <= diffCount && diffPos[diffIndex] >= recPos && diffPos[diffIndex] < recPos + have ) {
        zOut[diffPos[diffIndex] - recPos] = diffByte[diffIndex];
        diffIndex++;
      }
      if( mode == FMode::FDECOMPRESS ) {
        out->blockWrite(&zOut[0], have);
      } else if( mode == FMode::FCOMPARE ) {
        for( int j = 0; j < have; j++ ) {
          if( zOut[j] != out->getchar() && (diffFound == 0)) {
            diffFound = recPos + j + 1;
          }
        }
      }
      recPos += have;

    } while( recStrm.avail_out == 0 );
  }
  while( diffIndex <= diffCount ) {
    if( mode == FMode::FDECOMPRESS ) {
      out->putChar(diffByte[diffIndex]);
    } else if( mode == FMode::FCOMPARE ) {
      if( diffByte[diffIndex] != out->getchar() && (diffFound == 0)) {
        diffFound = recPos + 1;
      }
    }
    diffIndex++;
    recPos++;
  }
  deflateEnd(&recStrm);
  return recPos == len ? len : 0;
}


// ===== src/filter/bmp.hpp =====

#include <cstdint>

/**
 * 24/32-bit image data transforms:
 *   - simple color transform (b, g, r) -> (g, g-r, g-b)
 * Detects RGB565 to RGB888 conversions.
 */
class BmpFilter : public Filter {
private:
  int stride = 3; //3: RGB or BGR, 4: RGBA or BGRA
  int width = 0;
  bool isPossibleRgb565 = true;
  uint32_t rgb565Run = 0;
  static constexpr int rgb565MinRun = 63;
public:

  void setWidth(int w) {
    this->width = w;
  }
  void setHasApha() {
    this->stride = 4;
    this->isPossibleRgb565 = false; //to fix false positives (and to keep compatibility with previous version)
  }

  void encode(File *in, File *out, uint64_t size, int width, int & /*headerSize*/) override {
    uint32_t r = 0;
    uint32_t g = 0; // green is always the middle channel in both RGB and BGR images, the transform is symmetric in r and b
    uint32_t b = 0;
    for( int i = 0; i < static_cast<int>(size / width); i++ ) {
      for( int j = 0; j < width / stride; j++ ) {
        b = in->getchar();
        g = in->getchar();
        r = in->getchar();
        if( isPossibleRgb565 ) {
          int rgb565RunPrevious = rgb565Run;
          rgb565Run = min(rgb565Run + 1, 0xFFFF) *
                  static_cast<int>((b & 7) == ((b & 8) - ((b >> 3) & 1)) && (g & 3) == ((g & 4) - ((g >> 2) & 1)) &&
                                    (r & 7) == ((r & 8) - ((r >> 3) & 1)));
          if( rgb565Run > rgb565MinRun || rgb565RunPrevious >= rgb565MinRun ) {
            b ^= (b & 8) - ((b >> 3) & 1);
            g ^= (g & 4) - ((g >> 2) & 1);
            r ^= (r & 8) - ((r >> 3) & 1);
          }
          isPossibleRgb565 = rgb565Run > 0;
        }
        r = g - r;
        b = g - b;
        out->putChar(g);
        out->putChar(r);
        out->putChar(b);
        if (stride == 4) {
          out->putChar(in->getchar());
        }
      }
      for( int j = 0; j < width % stride; j++ ) {
        out->putChar(in->getchar());
      }
    }
    for( int i = size % width; i > 0; i-- ) {
      out->putChar(in->getchar());
    }
  }

  uint64_t decode(File * /*in*/, File *out, FMode fMode, uint64_t size, uint64_t &diffFound) override {
    uint32_t r = 0;
    uint32_t g = 0;
    uint32_t b = 0;
    uint32_t a = 0;
    uint32_t p = 0;
    for( int i = 0; i < static_cast<int>(size / width); i++ ) {
      p = i * width;
      for( int j = 0; j < width / stride; j++ ) {
        g = encoder->decompressByte(encoder->predictorMain);
        r = encoder->decompressByte(encoder->predictorMain);
        b = encoder->decompressByte(encoder->predictorMain);
        if (stride == 4) {
          a = encoder->decompressByte(encoder->predictorMain);
        }
        r = g - r;
        b = g - b;
        if( isPossibleRgb565 ) {
          if( rgb565Run >= rgb565MinRun ) {
            b ^= (b & 8) - ((b >> 3) & 1);
            g ^= (g & 4) - ((g >> 2) & 1);
            r ^= (r & 8) - ((r >> 3) & 1);
          }
          rgb565Run = min(rgb565Run + 1, 0xFFFF) *
                  static_cast<uint32_t>((b & 7) == ((b & 8) - ((b >> 3) & 1)) && (g & 3) == ((g & 4) - ((g >> 2) & 1)) &&
                                        (r & 7) == ((r & 8) - ((r >> 3) & 1)));
          isPossibleRgb565 = rgb565Run > 0;
        }
        if( fMode == FMode::FDECOMPRESS ) {
          out->putChar(b);
          out->putChar(g);
          out->putChar(r);
          if (stride == 4) {
            out->putChar(a);
          }
          if((j == 0) && ((i & 0xF) == 0)) {
            encoder->printStatus();
          }
        } else if( fMode == FMode::FCOMPARE ) {
          if((b & 255) != out->getchar() && (diffFound == 0)) {
            diffFound = p + 1;
          }
          if( g != out->getchar() && (diffFound == 0)) {
            diffFound = p + 2;
          }
          if((r & 255) != out->getchar() && (diffFound == 0)) {
            diffFound = p + 3;
          }
          if (stride == 4) {
            if ((a & 255) != out->getchar() && (diffFound == 0)) {
              diffFound = p + 4;
            }
          }
          p += stride;
        }
      }
      for( int j = 0; j < width % stride; j++ ) {
        if( fMode == FMode::FDECOMPRESS ) {
          out->putChar(encoder->decompressByte(encoder->predictorMain));
        } else if( fMode == FMode::FCOMPARE ) {
          if( encoder->decompressByte(encoder->predictorMain) != out->getchar() && (diffFound == 0)) {
            diffFound = p + j + 1;
          }
        }
      }
    }
    for( int i = size % width; i > 0; i-- ) {
      if( fMode == FMode::FDECOMPRESS ) {
        out->putChar(encoder->decompressByte(encoder->predictorMain));
      } else if( fMode == FMode::FCOMPARE ) {
        if( encoder->decompressByte(encoder->predictorMain) != out->getchar() && (diffFound == 0)) {
          diffFound = size - i;
          break;
        }
      }
    }
    return size;
  }
};

// ===== src/VLI.hpp =====

#include <cstdint>

static int VLICost(uint64_t n) {
  int cost = 1;
  while (n > 0x7F) {
    n >>= 7;
    cost++;
  }
  return cost;
}

// ===== src/filter/rle.hpp =====


class RleFilter : Filter {
private:

  int scanLineSize = 0;

  enum class RleState {
      BASE, LITERAL, RUN, LITERAL_RUN
  } state = RleState::BASE;

  void rleOutputRun(uint8_t byte, uint8_t* &outPtr, int &run) {
    while (run > 128) {
        *outPtr++ = 0xFF, *outPtr++ = byte;
        run -= 128;
    }
      *outPtr++ = (uint8_t)(0x80 | (run - 1)), *outPtr++ = byte;
  }


  void handleRun(uint8_t byte, uint8_t *&outPtr, uint8_t *&lastLiteral, int &run) {
    if( run > 1 ) {
      state = RleState::RUN;
      rleOutputRun(byte, outPtr, run);
    } else {
      lastLiteral = outPtr;
      *outPtr++ = 0, *outPtr++ = byte;
      state = RleState::LITERAL;
    }
  }

  void handleLiteral(uint8_t byte, uint8_t *&outPtr, uint8_t *lastLiteral, int &run) {
    if( run > 1 ) {
      state = RleState::LITERAL_RUN;
      rleOutputRun(byte, outPtr, run);
    } else {
      if( ++(*lastLiteral) == 127 ) {
        state = RleState::BASE;
      }
      *outPtr++ = byte;
    }
  }

  uint8_t handleLiteralRun(uint8_t *outPtr, uint8_t *lastLiteral) {
    uint8_t loop = 0;
    if( outPtr[-2] == 0x81 && *lastLiteral < (125)) {
      state = (((*lastLiteral) += 2) == 127) ? RleState::BASE : RleState::LITERAL;
      outPtr[-2] = outPtr[-1];
    } else {
      state = RleState::RUN;
    }
    loop = 1;
    return loop;
  }

public:

  void setScanLineSize(int scanLineSize) {
    this->scanLineSize = scanLineSize; // Run-length Packets should never encode pixels from more than one scan line (important for "decode")
  }

  void encode(File *in, File *out, uint64_t size, int /*info*/, int& headerSize) override {
    uint8_t b = 0;
    uint8_t c = in->getchar();
    int i = 1;
    out->putVLI(scanLineSize);
    headerSize = VLICost(scanLineSize);

    while( i < static_cast<int>(size)) {
      b = in->getchar(), i++;
      if( c == 0x80 ) {
        c = b;
        continue;
      }
      if( c > 0x7F ) {
        for( uint32_t j = 0; j <= (c & 0x7F); j++ ) {
          out->putChar(b);
        }
        c = in->getchar(), i++;
      } else {
        for(uint32_t j = 0; j <= c; j++, i++ ) {
          out->putChar(b), b = in->getchar();
        }
        c = b;
      }
    }
  }

  uint64_t decode(File *in, File *out, FMode fMode, uint64_t  /*size*/, uint64_t &diffFound) override {
    uint8_t inBuffer[0x10000] = {0};
    uint8_t outBuffer[0x10200] = {0};
    uint64_t pos = 0;
    scanLineSize = static_cast<int>(in->getVLI());

    do {
      uint64_t remaining = in->blockRead(&inBuffer[0], scanLineSize);
      uint8_t *inPtr = (uint8_t *) inBuffer;
      uint8_t *outPtr = (uint8_t *) outBuffer;
      uint8_t *lastLiteral = nullptr;
      state = RleState::BASE;
      while( remaining > 0 ) {
        uint8_t byte = *inPtr++;
        uint8_t loop = 0;
        int run = 1;
        for( remaining--; remaining > 0 && byte == *inPtr; remaining--, run++, inPtr++ ) {
        }
        do {
          loop = 0;
          switch( state ) {
            case RleState::BASE:
            case RleState::RUN: {
              handleRun(byte, outPtr, lastLiteral, run);
              break;
            }
            case RleState::LITERAL: {
              handleLiteral(byte, outPtr, lastLiteral, run);
              break;
            }
            case RleState::LITERAL_RUN: {
              loop = handleLiteralRun(outPtr, lastLiteral);
            }
          }
        } while( loop != 0 );
      }

      uint64_t length = outPtr - (&outBuffer[0]);
      if( fMode == FMode::FDECOMPRESS ) {
        out->blockWrite(&outBuffer[0], length);
      } else if( fMode == FMode::FCOMPARE ) {
        for(uint32_t j = 0; j < length; ++j ) {
          if( outBuffer[j] != out->getchar() && (diffFound == 0)) {
            diffFound = pos + j + 1;
            break;
          }
        }
      }
      pos += length;
    } while( !in->eof() && (diffFound == 0));
    return pos;
  }
};

// ===== src/filter/Filters.hpp =====

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
static DetectionInfo detect(File *in, uint64_t blockSize) {

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

static void compressRecursive(File *in, uint64_t blockSize, Encoder &en, String &blstr, float p1, float p2, const bool isTopLevel = true);

static uint64_t decodeFunc(BlockType type, Encoder &en, File *tmp, uint64_t len, int info, File *out, FMode mode, uint64_t &diffFound) {
  if( type == BlockType::IMAGE24 ) {
    auto f = BmpFilter();
    f.setWidth(info);
    f.setEncoder(en);
    return f.decode(tmp, out, mode, len, diffFound);
  }
  if( type == BlockType::IMAGE32 ) {
    auto f = BmpFilter();
    f.setWidth(info);
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

static uint64_t encodeFunc(BlockType type, File *in, File *tmp, uint64_t len, int info, int &hdrsize) {
  if( type == BlockType::IMAGE24 ) {
    auto f= BmpFilter();
    f.encode(in, tmp, len, info, hdrsize);
  } else if( type == BlockType::IMAGE32 ) {
    auto f = BmpFilter();
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
transformEncodeBlock(BlockType type, File *in, uint64_t len, Encoder &en, int info, String &blstr, float p1, float p2, uint64_t begin) {
  if( hasTransform(type, info)) {
    FileTmp tmp;
    int headerSize = 0;
    uint64_t diffFound = encodeFunc(type, in, &tmp, len, info, headerSize);
    const uint64_t tmpSize = tmp.curPos();
    tmp.setpos(tmpSize); //switch to read mode
    if( diffFound == 0 ) {
      tmp.setpos(0);
      en.setFile(&tmp);
      in->setpos(begin);
      decodeFunc(type, en, &tmp, tmpSize, info, in, FMode::FCOMPARE, diffFound);
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
          transformEncodeBlock(type2, &tmp, tmpSize - headerSize, en, info & 0xffffff, blstr, p1, p2, headerSize);
        } else {
          compressRecursive(&tmp, tmpSize, en, blstr, p1, p2, /*isTopLevel: */ false);
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

static void compressBlock(File* in, const uint64_t begin, const uint64_t len, int &blNum, BlockType type, int blockInfo, Encoder& en, String& blstr, float &p1, float &p2, const float pscale) {
  p2 = p1 + pscale * len;
  en.setStatusRange(p1, p2);

  String blstrSub;
  composeSubBlockStringToPrint(blstr, blstrSub, blNum);
  printBlock(begin, len, type, blockInfo, blstrSub);
  transformEncodeBlock(type, in, len, en, blockInfo, blstrSub, p1, p2, begin);
  blNum++;

  p1 = p2;
}

static void compressRecursive(File *in, uint64_t bytesToProcess, Encoder &en, String &blstr, float p1, float p2, const bool isTopLevel) {

  uint64_t begin = in->curPos();

  float pscale = bytesToProcess != 0 ? (p2 - p1) / bytesToProcess : 0;

  int blNum = 0;
  while(bytesToProcess > 0 ) {

    //detect a block 
    DetectionInfo detectionInfo = detect(in, bytesToProcess); // Special blocktypes
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
      compressBlock(in, begin, gapLength, /*ref: */ blNum, BlockType::DEFAULT, 0, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale);
      begin += gapLength;
      bytesToProcess -= gapLength;
    }

    if (detectionInfo.HeaderLength != 0) {
      compressBlock(in, detectionInfo.HeaderStart, detectionInfo.HeaderLength, /*ref: */ blNum, BlockType::HDR, 0, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale);
      begin += detectionInfo.HeaderLength;
      bytesToProcess -= detectionInfo.HeaderLength;
    }

    if (begin != detectionInfo.DataStart)
      quit("Internal error in compressRecursive");
    
    if (detectionInfo.DataLength != 0) {
      compressBlock(in, detectionInfo.DataStart, detectionInfo.DataLength, /*ref: */ blNum, detectionInfo.Type, detectionInfo.DataInfo, en, /*in: */ blstr, /*ref: */ p1, /*ref: */ p2, pscale);
      begin += detectionInfo.DataLength;
      bytesToProcess -= detectionInfo.DataLength;
    }
  }
}


// Compress a file. Split fileSize bytes into blocks by type.
// For each block, output
// <type> <size> and call encode_X to convert to type X.
// Test transform and compress.
static void compressfile(const char *filename, uint64_t fileSize, Encoder &en) {
  assert(en.getMode() == COMPRESS);
  assert(filename && filename[0]);

  Block::EncodeBlockSize(&en, fileSize);

  FileDisk in;
  in.open(filename, true);
  printf("Block segmentation:\n");
  String blstr;
  // detect block types + compress
  compressRecursive(&in, fileSize, en, blstr, 0.0F, 1.0F);
  in.close();
}

static uint64_t decompressRecursive(File *out, uint64_t blockSize, Encoder &en, FMode mode) {
  uint64_t i = 0;
  uint64_t diffFound = 0;
  while( i < blockSize ) {

    uint64_t len = Block::DecodeBlockHeader(&en);
    BlockType type = en.predictorMain->shared->State.blockType;
    int info = en.predictorMain->shared->State.blockInfo;
    if( hasRecursion(type)) {
      FileTmp tmp;
      decompressRecursive(&tmp, len, en, FMode::FDECOMPRESS);
      if( mode != FMode::FDISCARD ) {
        tmp.setpos(0);
        if( hasTransform(type, info)) {
          len = decodeFunc(type, en, &tmp, len, info, out, mode, diffFound);
        }
      }
      tmp.close();
    } else if( hasTransform(type, info)) {
      len = decodeFunc(type, en, nullptr, len, info, out, mode, diffFound);
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
static void decompressFile(const char* filename, FMode fMode, Encoder& en) {
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
  uint64_t r = decompressRecursive(&f, fileSize, en, fMode);
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

// ===== src/Simd.hpp =====

///////////////////////// SIMD Vectorization detection //////////////////////////////////

#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#define HAS_CPUID 1
#endif

#ifdef HAS_CPUID
#if defined(_MSC_VER)
#include <intrin.h>
#define cpuid(info, x) __cpuidex(info, x, 0)
#elif defined(__GNUC__) || defined(__clang__)
#include <x86intrin.h>
#include <cpuid.h>
#define cpuid(info, x) __cpuid_count(x, 0, (info)[0], (info)[1], (info)[2], (info)[3])
#endif
#endif

// Define interface to xgetbv instruction
#ifdef HAS_CPUID
static inline unsigned long long xgetbv(unsigned long ctr) {
#if (defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 160040000) || (defined(__INTEL_COMPILER) && __INTEL_COMPILER >= 1200)
  return _xgetbv(ctr);
#elif (defined(__GNUC__) || defined(__clang__))
  uint32_t a = 0;
  uint32_t d;
  __asm("xgetbv"
  : "=a"(a), "=d"(d)
  : "c"(ctr)
  :);
  return a | ((static_cast<uint64_t>(d)) << 32);
#else
#error Unknown compiler
#endif
}
#endif

/* Returns system's highest supported SIMD instruction set as
0: none (or unknown)
1: MMX
2: SSE
3: SSE2
4: SSE3
5: SSSE3
6: SSE4.1
7: SSE4.2
 : SSE4A //SSE4A is not supported on Intel, so we will exclude it
8: AVX
9: AVX2
10: AVX512 //AVX512F + AVX512BW
11: NEON
*/
static int simdDetect() {
#ifdef HAS_CPUID
  int cpuidResult[4] = {0, 0, 0, 0};
  cpuid(cpuidResult, 0); // call cpuid function 0 ("Get vendor ID and highest basic calling parameter")
  if( cpuidResult[0] == 0 ) {
    return 0; //cpuid is not supported
  }
  cpuid(cpuidResult, 1); // call cpuid function 1 ("Processor Info and Feature Bits")
  if((cpuidResult[3] & (1 << 23)) == 0 ) {
    return 0; //no MMX
  }
  if((cpuidResult[3] & (1 << 25)) == 0 ) {
    return 1; //no SSE
  }
  //SSE: OK
  if((cpuidResult[3] & (1 << 26)) == 0 ) {
    return 2; //no SSE2
  }
  //SSE2: OK
  if((cpuidResult[2] & (1 << 0)) == 0 ) {
    return 3; //no SSE3
  }
  //SSE3: OK
  if((cpuidResult[2] & (1 << 9)) == 0 ) {
    return 4; //no SSSE3
  }
  //SSSE3: OK
  if((cpuidResult[2] & (1 << 19)) == 0 ) {
    return 5; //no SSE4.1
  }
  //SSE4.1: OK
  if((cpuidResult[2] & (1 << 20)) == 0 ) {
    return 6; //no SSE4.2
  }
  //SSE4.2: OK
  if((cpuidResult[2] & (1 << 27)) == 0 ) {
    return 7; //no OSXSAVE (no XGETBV)
  }
  if((xgetbv(0) & 6) != 6 ) {
    return 7; //AVX is not enabled in OS
  }
  if((cpuidResult[2] & (1 << 28)) == 0 ) {
    return 7; //no AVX
  }
  //AVX: OK
  cpuid(cpuidResult, 7); // call cpuid function 7 ("Extended Feature Bits")
  if((cpuidResult[1] & (1 << 5)) == 0 ) {
    return 8; //no AVX2
  }
  //AVX2: OK
  if((cpuidResult[1] & (1 << 16)) == 0 || (cpuidResult[1] & (1 << 30)) == 0 ) {
    return 9; //no AVX512F + AVX512BW
  }
  //AVX512: OK
  return 10;
#elif defined(ARM_NEON_AVAILABLE)
  return 11;
#else
  return 0; //unknown system - use non-accelerated codepaths
#endif
}


// ===== src/paqimg.cpp =====
/*
  paqimg – Specialized Lossless Image Compressor (based on PAQ8PX)
  see README.md for information

  Amalgamated single-translation-unit build: this file pulls in the whole
  source tree through the include graph. Build with CMake.
*/

//////////////////////// Versioning ////////////////////////////////////////

#define PROGNAME     "paqimg"
#define PROGVERSION  "2"  //update version here before publishing your changes
#define PROGYEAR     "2026"



#include <memory>
#include <stdexcept>  //std::exception



typedef enum { DoNone, DoCompress, DoExtract } WHATTODO;

static void printHelp() {
  printf(
    "Free under GPL, http://www.gnu.org/licenses/gpl.txt\n\n"
    "Usage:\n"
    "  to compress     ->  " PROGNAME " INPUT [OUTPUT]\n"
    "  to decompress   ->  " PROGNAME " -d INPUT." PROGNAME PROGVERSION " [OUTPUT]\n"
    "\n"
    "paqimg is a specialized lossless image compressor: it supports\n"
    "BMP, PBM/PGM/PPM/PAM, PGM (RAW), JPEG, HDR/RGBE, GIF, PNG, TGA and TIFF\n"
    "images. Other file types are rejected.\n"
    "\n"
    "The compression level is selected automatically: paqimg checks the\n"
    "installed RAM and picks the strongest level that still fits\n"
    "(level 1 uses ~583 MB, level 12 uses ~24.5 GB). The level actually\n"
    "used is written into the archive and printed on screen.\n"
    "\n"
    "  Example:  " PROGNAME " image.png\n"
    "            " PROGNAME " -d image.png." PROGNAME PROGVERSION "\n"
    "\n"
    "Notes:\n"
    "  INPUT is an image FILE or PATH/FILE\n"
    "  OUTPUT is optional: FILE, PATH, PATH/FILE\n"
    "  The archive is created in the current folder with ." PROGNAME PROGVERSION "\n"
    "  extension if OUTPUT omitted\n"
  );
}

// Approximate total memory use (MB) per compression level.
static const uint64_t LEVEL_MEMORY_MB[13] = {0, 583, 597, 626, 685, 801, 1034, 1501,
                                             2433, 4298, 7772, 14719, 24583};

// Total installed physical memory in MB, or 0 when it cannot be determined.
static uint64_t totalPhysicalMemoryMB() {
#ifdef WINDOWS
  MEMORYSTATUSEX status;
  status.dwLength = sizeof(status);
  if( GlobalMemoryStatusEx(&status) != 0 )
    return status.ullTotalPhys >> 20;
  return 0;
#endif
#ifdef UNIX
  const long pages = sysconf(_SC_PHYS_PAGES);
  const long pageSize = sysconf(_SC_PAGE_SIZE);
  if( pages > 0 && pageSize > 0 )
    return static_cast<uint64_t>(pages) * static_cast<uint64_t>(pageSize) >> 20;
  return 0;
#endif
}

// Select the strongest compression level that fits into the installed RAM
// (with a safety margin for the OS). Falls back to level 5 when the RAM
// size cannot be determined, so that it still runs everywhere.
static int autoLevel() {
  const uint64_t totalMB = totalPhysicalMemoryMB();
  if( totalMB == 0 )
    return 5;
  int level = 1;
  for( int l = 1; l <= 12; l++ ) {
    if( LEVEL_MEMORY_MB[l] <= (totalMB * 9) / 10 )
      level = l;
  }
  return level;
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
      quit();
    }

    if( strcasecmp(argv[1], "-help") == 0 ) {
      printHelp();
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
      if( argv[i][0] == '-' ) {
        if( strcasecmp(argv[i], "-d") == 0 ) {
          if( whattodo != DoNone ) {
            quit("Only one command may be specified.");
          }
          whattodo = DoExtract;
        } else {
          printf("Invalid command: %s\n", argv[i]);
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
    if( whattodo == DoNone ) {
      whattodo = DoCompress; //compress is the default command
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
      // Verify archive header and get the compression level
      const int len = static_cast<int>(strlen(PROGNAME));
      for( int i = 0; i < len; i++ ) {
        if( archive.getchar() != PROGNAME[i] ) {
          printf("%s: not a valid %s file.", archiveName.c_str(), PROGNAME);
          quit();
        }
      }

      const int headerLevel = archive.getchar(); // level
      if( headerLevel == EOF || headerLevel < 1 || headerLevel > 12 ) {
        quit("Unexpected compression level setting in archive");
      }
      level = headerLevel;
    } else {
      // Pick the strongest compression level that fits into the installed RAM
      level = autoLevel();
      printf("Using compression level %d (best fitting the installed RAM)\n", level);
    }

    printf("\n");

    // Write archive header to archive file
    if( mode == COMPRESS ) {
      printf("Creating archive %s...\n", archiveName.c_str());
      archive.create(archiveName.c_str());
      archive.append(PROGNAME);
      archive.putChar(level);
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

    Shared sharedForBlockPredictor;
    sharedForBlockPredictor.init(level, 16);
    sharedForBlockPredictor.chosenSimd = shared.chosenSimd;
    std::unique_ptr<Predictor> predictorBlock = std::make_unique<PredictorBlock>(&sharedForBlockPredictor);
    std::unique_ptr<Predictor> predictorMain = std::make_unique<PredictorMain>(&shared);

    Encoder en(predictorBlock.get(), predictorMain.get(), /*doEncoding: */ true, mode, &archive);

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
      compressfile(fName, fSize, en);
      en.flush();
      printf("-----------------------\n");
      printf("Total input size     : %" PRIu64 "\n", fSize);
      printf("Total archive size   : %" PRIu64 "\n", en.size());
      printf("\n");
    } else { //decompress
      FileName fn;
      fn += outputPath.c_str();
      fn += output.c_str();
      const char *fName = fn.c_str();
      decompressFile(fName, FMode::FDECOMPRESS, en);
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
#include <shellapi.h>
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
  if( (szArglist = CommandLineToArgvW(GetCommandLineW(), &argc_utf8)) == NULL ) {
    printf("CommandLineToArgvW failed\n");
    return 0;
  } else {
    if(argc!=argc_utf8)quit("Error preparing command line arguments.");
    argv_utf8=new char*[argc_utf8+1];
    for(int i=0;i<argc_utf8; i++) {
      wchar_t *s=szArglist[i];
      int buffersize = WideCharToMultiByte(CP_UTF8,0,s,-1,NULL,0,NULL,NULL);
      argv_utf8[i] = new char[buffersize];
      WideCharToMultiByte(CP_UTF8,0,s,-1,argv_utf8[i],buffersize,NULL,NULL);
      //printf("%d: %s\n", i, argv_utf8[i]); //debug: see if conversion is successful
    }
    argv_utf8[argc_utf8]=nullptr;
    int retval=processCommandLine(argc_utf8, argv_utf8);
    for(int i=0;i<argc_utf8; i++) delete[] argv_utf8[i];
    delete[] argv_utf8;
    SetConsoleOutputCP(oldcp);
    return retval;
  }
#else
  return processCommandLine(argc, argv);
#endif
}

// ===== src/AdaptiveMap.cpp =====

AdaptiveMap::AdaptiveMap(const Shared* const sh, const int n) : shared(sh), t(n) {
  dt = DivisionTable::getDT();
}

// sum = n0+n1
// p1 = (n1+0.5) / (n0+n1+1) [Laplace smoothed]
//
// To update p1 given y (new bit seen):
// case: y=0
//  n0++ -> recalculate p1
// case: y=1
//  n1++ -> recalculate p1
// But calculating p1 this way involves a division by an arbitrary denominator which is slow.
// -> We will replace this division by a table-based multiplication by the reciprocal.
// To do so we switch from (n0,n1) to (sum,p1) which encodes the same information just in different form.
// This way the table becomes smaller as it depends on a single operand (sum) not two operands (n0, n1)
// Updating (sum,p1) is not as straightforward as before: sum++, but new_p1 = ?
// 
// We will see below that both update cases (y=0 and y=1) follow the same pattern:
//   new_p1 = old_p1 + delta
// where delta = (y - old_p1) / (sum+2)
// 
// Derivation:
//
// old_p1 = p1 = (n1+0.5) / (n0+n1+1) = (2*n1+1) / (2*(sum+1))      // multiply both numerator and denominator by 2
//
// update case: y=0
//   old_p1 = (2*n1+1) / (2*(sum+1))
//   new_p1 = (2*n1+1) / (2*(sum+2))                                 // n1 unchanged, sum -> sum+1
//          = old_p1 * (sum+1)/(sum+2)                               // multiply and divide by (2*(sum+1))
//          = old_p1 * (1 - 1/(sum+2))                               // rewrite (sum+1)/(sum+2) = 1 - 1/(sum+2)
//          = old_p1 + (0 - old_p1) / (sum+2)
//
// update case: y=1
//   old_p1 = (2*n1+1) / (2*(sum+1))
//   new_p1 = (2*n1+3) / (2*(sum+2))                                 // n1 -> n1+1, sum -> sum+1
//          = (2*n1+1)/(2*(sum+2)) + 1/(sum+2)                       // split numerator: (2*n1+3) = (2*n1+1) + 2
//          = old_p1 * (sum+1)/(sum+2) + 1/(sum+2)                   // multiply and divide first term by (2*(sum+1))
//          = old_p1 * (1 - 1/(sum+2)) + 1/(sum+2)                   // rewrite (sum+1)/(sum+2) = 1 - 1/(sum+2)
//          = old_p1 + (1 - old_p1) / (sum+2)
//
// That's:
//   new_p1 = old_p1 + (y - old_p1) / (sum+2)
// for both cases.
//
// With fixed-point arithmetic:
//
//-> delta_22bit = (target_22bit - p1_22bit) / (sum+2)
//               = (target_22bit - p1_22bit) * dt[sum] / (1<<30)     // where dt[sum] = (1<<30) / (sum+2)

void AdaptiveMap::update(uint32_t* const p, const int limit) {
  assert(limit > 0 && limit < 1024);
  uint32_t cell = p[0];
  const int sum = cell & 1023;      //sum=n0+n1
  const int64_t y = shared->State.y;
  const int64_t target_22bit = y << 22;
  const int64_t p1_22bit = cell >> 10;  //prediction
  const int delta_22bit = int(((target_22bit - p1_22bit) * int64_t(dt[sum])) >> 30); // see derivation above
  cell = (uint32_t(p1_22bit + delta_22bit) << 10) | (sum < limit ? sum + 1 : sum);
  p[0] = cell;
}

// ===== src/Squash.hpp =====

/**
 * return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
 * @param d
 * @return
 */

int squash(int d);


// ===== src/APM.cpp =====

APM::APM(const Shared* const sh, const int n, const int s, const int limit) : AdaptiveMap(sh, n * s), N(n * s), steps(s), cxt(0), limit(limit) {
  assert(s > 4); // number of steps - must be a positive integer bigger than 4
  for( int i = 0; i < N; ++i ) {
    int p = ((i % steps * 2 + 1) * 4096) / (steps * 2) - 2048;
    t[i] = (uint32_t(squash(p)) << 20) + 6; //initial count: 6
  }
}

void APM::update() {
  assert(cxt >= 0 && cxt < N);
  AdaptiveMap::update(&t[cxt], limit);
}

int APM::p(int pr, int cx) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  assert(pr >= 0 && pr < 4096);
  assert(cx >= 0 && cx < N / steps);
  pr = (stretch(pr) + 2048) * (steps - 1);
  int wt = pr & 0x0fff; // interpolation weight (0..4095)
  cx = cx * steps + (pr >> 12);
  assert(cx >= 0 && cx < N - 1);
  cxt = cx + (wt >> 11);
  pr = ((t[cx] >> 13) * (4096 - wt) + (t[cx + 1] >> 13) * wt) >> 19;
  return pr;
}


// ===== src/APM1.cpp =====

APM1::APM1(const Shared* const sh, const int n, const int r) : shared(sh), index(0), n(n), t(n * 33), rate(r) {
  assert(n > 0 && rate > 0 && rate < 32);
  // maps p, cxt -> p initially
  for( int i = 0; i < n; ++i ) {
    for( int j = 0; j < 33; ++j ) {
      if( i == 0 ) {
        t[i * 33 + j] = squash((j - 16) * 128) * 16;
      } else {
        t[i * 33 + j] = t[j];
      }
    }
  }
}

int APM1::p(int pr, const int cxt) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  assert(pr >= 0 && pr < 4096 && cxt >= 0 && cxt < n);
  pr = stretch(pr);
  const int w = pr & 127; // interpolation weight (33 points)
  index = ((pr + 2048) >> 7) + cxt * 33;
  return (t[index] * (128 - w) + t[index + 1] * w) >> 11;
}

void APM1::update() {
  INJECT_SHARED_y
  const int g = (y << 16) + (y << rate) - y - y;
  t[index] += (g - t[index]) >> rate;
  t[index + 1] += (g - t[index + 1]) >> rate;
}


// ===== src/APMPost.cpp =====

APMPost::APMPost(const Shared* const sh, const uint32_t n) : shared(sh), index(0), n(n), t(n* UINT64_C(4096)) {
  assert(n > 0);
  for(size_t i = 0; i < n; ++i ) {
    for(uint64_t j = 0; j < 4096; ++j ) {
      t[i * 4096 + j] = ((4096 - j) >> 1) << 32 | (j >> 1);
    }
  }
}


uint32_t APMPost::p(const uint32_t pr, const uint32_t cxt) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  assert(pr < 4096 && cxt < n);
  index = cxt * 4096 + pr;
  uint64_t n0 = t[index] >> 32;
  uint64_t n1 = t[index] &0xffffffff;
  n0 = n0 * 2 + 1;
  n1 = n1 * 2 + 1;
  constexpr int PRECISION = ArithmeticEncoder::PRECISION;
  return static_cast<uint32_t>((n1 << PRECISION) / (n0+n1));
}

void APMPost::update() {
  INJECT_SHARED_y
  uint64_t n0, n1, value;
  value = t[index];
  n0 = value >> 32;
  n1 = value & 0xffffffff;

  n0 += 1 - y;
  n1 += y;
  int shift = (n0 | n1) >> 32; // shift: 0 or 1
  n0 >>= shift;
  n1 >>= shift;

  t[index] = n0 << 32 | n1;
}


// ===== src/ArithmeticEncoder.cpp =====
#include <cassert>
#include <cstdint>

ArithmeticEncoder::ArithmeticEncoder(File* f)
  : x1(0), x2(UINT32_MAX), x(0), pending_bits(0), bits_in_B(0), B(0), archive(f) {
}

// Read one bit from the archive, buffering a full byte at a time.
int ArithmeticEncoder::bit_read() {
  if (bits_in_B == 0) {
    B = archive->getchar(); // EOF is handled gracefully by the caller
    bits_in_B = 8;
  }
  bits_in_B--; //7..0
  return (B >> bits_in_B) & 1;
}

// Write one bit to the archive, flushing a full byte when the buffer is full.
void ArithmeticEncoder::bit_write(const int bit) {
  B = (B << 1) | bit;
  bits_in_B++;
  if (bits_in_B == 8) {
    archive->putChar(B);
    B = 0;
    bits_in_B = 0;
  }
}

// Write one bit, then emit all pending bits with the opposite value (E3 carry resolution).
void ArithmeticEncoder::bit_write_with_pending(const int bit) {
  bit_write(bit);
  for (; pending_bits > 0; pending_bits--)
    bit_write(bit ^ 1);
}

// Flush the encoder: emit enough bits from x1 to complete the final byte.
void ArithmeticEncoder::flush() {
  do {
    bit_write_with_pending(x1 >> 31);
    x1 <<= 1;
  } while (bits_in_B != 0);
}

// Prime the decode window with the first 32 bits of compressed data.
void ArithmeticEncoder::prefetch() {
  for (int i = 0; i < 32; ++i)
    x = (x << 1) | bit_read();
}

// Encode one bit. p is the probability (scaled by 2^PRECISION) that bit == 1.
void ArithmeticEncoder::encodeBit(uint32_t p, const int bit) {
  if (p == 0) p++;
  assert(p > 0 && p < (1u << PRECISION));

  // Split [x1, x2] at xmid proportional to p; round to nearest for accuracy.
  const uint32_t xmid = x1 + uint32_t((uint64_t(x2 - x1) * p) >> PRECISION);
  assert(xmid >= x1 && xmid < x2);

  bit != 0 ? (x2 = xmid) : (x1 = xmid + 1);

  // Emit leading bits that are identical in x1 and x2.
  while (((x1 ^ x2) >> 31) == 0) {
    bit_write_with_pending(x2 >> 31);
    x1 <<= 1;
    x2 = (x2 << 1) | 1;
  }

  // Rescale when the interval straddles the midpoint; defer the carry bit.
  while (x1 >= 0x40000000 && x2 < 0xC0000000) {
    pending_bits++;
    x1 = (x1 << 1) & 0x7FFFFFFF;
    x2 = (x2 << 1) | 0x80000001;
  }
}

// Decode one bit. p is the probability (scaled by 2^PRECISION) that the bit is 1.
int ArithmeticEncoder::decodeBit(uint32_t p) {
  if (p == 0) p++;
  assert(p > 0 && p < (1u << PRECISION));

  // Mirror the encoder's midpoint split.
  const uint32_t xmid = x1 + uint32_t((uint64_t(x2 - x1) * p) >> PRECISION);
  assert(xmid >= x1 && xmid < x2);

  const int bit = (x <= xmid) ? 1 : 0;
  bit != 0 ? (x2 = xmid) : (x1 = xmid + 1);

  // Consume leading bits and refill x from the bitstream.
  while (((x1 ^ x2) >> 31) == 0) {
    x1 <<= 1;
    x2 = (x2 << 1) | 1;
    x = (x << 1) | bit_read();
  }

  // Undo midpoint rescaling and keep x in sync with the interval.
  while (x1 >= 0x40000000 && x2 < 0xC0000000) {
    x1 = (x1 << 1) & 0x7FFFFFFF;
    x2 = (x2 << 1) | 0x80000001;
    x = (x << 1) ^ 0x80000000;
    x += bit_read();
  }

  return bit;
}

// ===== src/BitCount.hpp =====

#include <cstdint>

uint32_t bitCount(uint32_t v);


// ===== src/BitCount.cpp =====
#include <cstdint>

uint32_t bitCount(uint32_t v) {
  v -= ((v >> 1) & 0x55555555);
  v = ((v >> 2) & 0x33333333) + (v & 0x33333333);
  v = ((v >> 4) + v) & 0x0f0f0f0f;
  v = ((v >> 8) + v) & 0x00ff00ff;
  v = ((v >> 16) + v) & 0x0000ffff;
  return v;
}



// ===== src/Block.hpp =====


namespace Block {

  void EncodeBlockHeader(Encoder* const encoder, BlockType blockType, uint64_t blockSize, int blockInfo);
  uint64_t DecodeBlockHeader(Encoder* const encoder);
  void EncodeBlockSize(Encoder* const encoder, uint64_t blockSize);
  uint64_t DecodeBlockSize(Encoder* const encoder);
  void EncodeBlockType(Encoder* const encoder, BlockType blocktype);
  BlockType DecodeBlockType(Encoder* const encoder);
  void EncodeInfo(Encoder* const encoder, int info);
  int DecodeInfo(Encoder* const encoder);
};


// ===== src/Block.cpp =====

namespace Block {

  void EncodeBlockHeader(Encoder* const encoder, BlockType blockType, uint64_t blockSize, int blockInfo) {
    EncodeBlockType(encoder, blockType);
    EncodeBlockSize(encoder, blockSize);
    if (hasInfo(blockType))
      EncodeInfo(encoder, blockInfo);
    encoder->initContextForBlockModel(blockType, blockInfo);
  }

  uint64_t DecodeBlockHeader(Encoder* const encoder) { //returns blockSize
    BlockType blockType = DecodeBlockType(encoder);
    uint64_t blockSize = DecodeBlockSize(encoder);
    int blockInfo = -1;
    if (hasInfo(blockType))
      blockInfo = DecodeInfo(encoder);

    Shared* shared = encoder->predictorMain->shared;
    shared->State.blockType = blockType;
    shared->State.blockInfo = blockInfo;
    shared->State.blockPos = UINT32_MAX;
    return blockSize;
  }

  void EncodeBlockType(Encoder* const encoder, BlockType blocktype) {
    encoder->setContextForBlockModel(0x10);
    encoder->compressByte(encoder->predictorBlock, uint8_t(blocktype));
    encoder->appendToBlockTypeHistoryForBlockModel(blocktype);
  }

  BlockType DecodeBlockType(Encoder* const encoder) {
    encoder->setContextForBlockModel(0x10);
    BlockType blockType = (BlockType)encoder->decompressByte(encoder->predictorBlock);
    encoder->appendToBlockTypeHistoryForBlockModel(blockType);
    return blockType;
  }

  void EncodeBlockSize(Encoder* const encoder, uint64_t blockSize) {
    encoder->setContextForBlockModel(0x20);
    encoder->compressByte(encoder->predictorBlock, (blockSize >> 24) & 0xFF);
    encoder->setContextForBlockModel(0x21);
    encoder->compressByte(encoder->predictorBlock, (blockSize >> 16) & 0xFF);
    encoder->setContextForBlockModel(0x22);
    encoder->compressByte(encoder->predictorBlock, (blockSize >> 8) & 0xFF);
    encoder->setContextForBlockModel(0x23);
    encoder->compressByte(encoder->predictorBlock, (blockSize) & 0xFF);
  }

  uint64_t DecodeBlockSize(Encoder* const encoder) {
    uint64_t blockSize = 0;
    uint8_t b;
    encoder->setContextForBlockModel(0x20);
    b = encoder->decompressByte(encoder->predictorBlock);
    blockSize = blockSize << 8 | b;
    encoder->setContextForBlockModel(0x21);
    b = encoder->decompressByte(encoder->predictorBlock);
    blockSize = blockSize << 8 | b;
    encoder->setContextForBlockModel(0x22);
    b = encoder->decompressByte(encoder->predictorBlock);
    blockSize = blockSize << 8 | b;
    encoder->setContextForBlockModel(0x23);
    b = encoder->decompressByte(encoder->predictorBlock);
    blockSize = blockSize << 8 | b;
    return blockSize;
  }

  void EncodeInfo(Encoder* const encoder, int info) {
    encoder->setContextForBlockModel(0x30);
    encoder->compressByte(encoder->predictorBlock, (info >> 24) & 0xFF);
    encoder->setContextForBlockModel(0x31);
    encoder->compressByte(encoder->predictorBlock, (info >> 16) & 0xFF);
    encoder->setContextForBlockModel(0x32);
    encoder->compressByte(encoder->predictorBlock, (info >> 8) & 0xFF);
    encoder->setContextForBlockModel(0x33);
    encoder->compressByte(encoder->predictorBlock, (info) & 0xFF);
  }

  int DecodeInfo(Encoder* const encoder) {
    uint32_t info = 0;
    uint8_t b;
    encoder->setContextForBlockModel(0x30);
    b = encoder->decompressByte(encoder->predictorBlock);
    info = info << 8 | b;
    encoder->setContextForBlockModel(0x31);
    b = encoder->decompressByte(encoder->predictorBlock);
    info = info << 8 | b;
    encoder->setContextForBlockModel(0x32);
    b = encoder->decompressByte(encoder->predictorBlock);
    info = info << 8 | b;
    encoder->setContextForBlockModel(0x33);
    b = encoder->decompressByte(encoder->predictorBlock);
    info = info << 8 | b;
    return info;
  }


};

// ===== src/BlockType.cpp =====
#include <cstdint>

bool hasRecursion(BlockType ft) {
  return ft == BlockType::ZLIB || ft == BlockType::GIF || ft == BlockType::RLE || ft == BlockType::LZW || ft == BlockType::PNG8 || ft == BlockType::PNG8GRAY || ft == BlockType::PNG24 || ft == BlockType::PNG32;
}

bool hasInfo(BlockType ft) {
  return ft == BlockType::IMAGE1 || ft == BlockType::IMAGE4 || ft == BlockType::IMAGE8 || ft == BlockType::IMAGE8GRAY || ft == BlockType::IMAGE24 || ft == BlockType::IMAGE32 ||
    ft == BlockType::PNG8 || ft == BlockType::PNG8GRAY || ft == BlockType::PNG24 || ft == BlockType::PNG32;
}

bool hasTransform(BlockType ft, int /*info*/) {
  return ft == BlockType::IMAGE24 || ft == BlockType::IMAGE32 || ft == BlockType::ZLIB || ft == BlockType::GIF ||
    ft == BlockType::RLE || ft == BlockType::LZW || ft == BlockType::PNG8 || ft == BlockType::PNG8GRAY || ft == BlockType::PNG24 || ft == BlockType::PNG32;
}

bool isPNG(BlockType ft) { return ft == BlockType::PNG8 || ft == BlockType::PNG8GRAY || ft == BlockType::PNG24 || ft == BlockType::PNG32; }


// ===== src/Clz.hpp =====

#include <cassert>
#include <cstdint>

#ifdef _MSC_VER
#include <intrin.h>
static inline uint32_t clz(uint32_t x) {
  assert(x != 0);
  unsigned long leading_zero;
  _BitScanReverse(&leading_zero, x);
  return 31u - leading_zero;
}
#elif (defined(__GNUC__) || defined(__clang__))
static inline uint32_t clz(uint32_t x) {
  assert(x != 0);
  return __builtin_clz(x);
}
#else
static inline uint32_t popcnt(uint32_t x) {
  x -= ((x >> 1) & 0x55555555);
  x = (((x >> 2) & 0x33333333) + (x & 0x33333333));
  x = (((x >> 4) + x) & 0x0f0f0f0f);
  x += (x >> 8);
  x += (x >> 16);
  return x & 0x0000003f;
}
static inline uint32_t clz(uint32_t x) {
  assert(x != 0);
  x |= (x >> 1);
  x |= (x >> 2);
  x |= (x >> 4);
  x |= (x >> 8);
  x |= (x >> 16);
  return 32u - popcnt(x);
}
#endif

// ===== src/ContextMap2.cpp =====

ContextMap2::ContextMap2(const Shared* const sh, const uint64_t size, const uint32_t contexts, const int scale) : shared(sh), 
        C(contexts), contextInfoList(contexts), hashTable(size >> 6),
        runMap(sh, contexts, (1 << 12), 127, StateMapType::Run),         /* StateMap : s, n, lim, init */ // 63-255
        stateMap(sh, contexts, (1 << 8), 511, StateMapType::BitHistory), /* StateMap : s, n, lim, init */ // 511-1023
        bhMap8B(sh, contexts, (1 << 8), 511, StateMapType::Generic),     /* StateMap : s, n, lim, init */ // 511-1023
        bhMap12B(sh, contexts, (1 << 12), 511, StateMapType::Generic),   /* StateMap : s, n, lim, init */ // 255-1023
        index(0), mask(uint32_t(hashTable.size() - 1)), hashBits(ilog2(mask + 1)), scale(scale), contextflagsAll(0) {
  assert(size >= 64 && isPowerOf2(size));
  static_assert(sizeof(Bucket16 <HashElementForContextMap, 7>) == (sizeof(HashElementForContextMap) + 2) * 7, "Something is wrong, please pack that struct");
}

void ContextMap2::updatePendingContextsInSlot(HashElementForContextMap* const p, uint32_t c) {
  // in case of a collision updating (mixing) is slightly better (but slightly slower) then resetting, so we update
  StateTable::update(&p->slot1and2.bitState,  (c >> 2) & 1, rnd);
  StateTable::update(&p->slot1and2.bitState0  + ((c >> 2) & 1), (c >> 1) & 1, rnd);
  StateTable::update(&p->slot1and2.bitState00 + ((c >> 1) & 3), (c & 1), rnd);
}

void ContextMap2::updatePendingContexts(uint32_t ctx, uint16_t checksum, uint32_t c) {
  // update pending bit histories for bits 2, 3, 4 (in slot 1)
  HashElementForContextMap* const p1A = hashTable[(ctx + (c >> 6)) & mask].find(checksum, &rnd);
  updatePendingContextsInSlot(p1A, c >> 3);
  // update pending bit histories for bits 5, 6, 7 (in slot 2)
  HashElementForContextMap* const p1B = hashTable[(ctx + (c >> 3)) & mask].find(checksum, &rnd);
  updatePendingContextsInSlot(p1B, c);
}

void ContextMap2::set(uint8_t ctxflags, const uint64_t contexthash) {
  assert(index >= 0 && index < C);
  ContextInfo *contextInfo = &contextInfoList[index];
  const uint32_t ctx = contextInfo->tableIndex = finalize64(contexthash, hashBits);
  const uint16_t chk = contextInfo->tableChecksum = checksum16(contexthash, hashBits);
  HashElementForContextMap* const hashElementInSlot0 =  hashTable[ctx].find(chk, &rnd);
  contextInfo->hashElementInSlot0 = hashElementInSlot0;
  contextInfo->hashElementInSlot012 = hashElementInSlot0;

  if (hashElementInSlot0->slot0.bitState <= 6) { // while constructing statistics for the first 3 bytes (states: 0; 1-2; 3-6) defer updating bit statistics in slot1 and slot2
    ctxflags |= CM_DEFERRED;
  }
  else if (hashElementInSlot0->slot0.bitState <= 14) { // the first 3 bytes in this context are now known, it's time to update pending bit histories
    if (hashElementInSlot0->slot0.runcount == 2) {
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte2 + 256);
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte1 + 256);
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte1 + 256);
    }
    else {
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte3 + 256);
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte2 + 256);
      updatePendingContexts(ctx, chk, hashElementInSlot0->slot0.byte1 + 256);
    }
  }
  
  contextInfo->flags = ctxflags;
  contextflagsAll |= ctxflags;
  index++;
}


void ContextMap2::skip(const uint8_t ctxflags) {
  assert(index >= 0 && index < C);
  contextInfoList[index].flags = ctxflags | CM_SKIPPED_CONTEXT;
  contextflagsAll |= ctxflags;
  index++;
}

void ContextMap2::skipn(const uint8_t ctxflags, const int n) {
  for (int i = 0; i < n; i++)
    skip(ctxflags);
}

ALWAYS_INLINE
size_t ContextMap2::getStateByteLocation(const uint32_t bpos, const uint32_t c0) {
  uint32_t pis = 0; //state byte position in slot
  if (false) {
    // this version is for readability
    switch (bpos) {
    case 0: //slot0
      pis = 0;
      break;
    case 1: //slot0
      pis = 1 + (c0 & 1);
      break;
    case 2: //slot1
      pis = 0;
      break;
    case 3: //slot1
      pis = 1 + (c0 & 1);
      break;
    case 4: //slot1
      pis = 3 + (c0 & 3);
      break;
    case 5: //slot2
      break;
    case 6: //slot2
      pis = 1 + (c0 & 1);
      break;
    case 7: //slot2
      pis = 3 + (c0 & 3);
      break;
    }
  }
  else {
    // this is a speed optimized (branchless) version of the above
    const uint32_t smask = (UINT32_C(0x31031010) >> (bpos << 2)) & 0x0F;
    pis = smask + (c0 & smask);
  }

  return pis;
}


void ContextMap2::update() {
  INJECT_SHARED_y
  INJECT_SHARED_bpos
  INJECT_SHARED_c1
  INJECT_SHARED_c0
  for( uint32_t i = 0; i < index; i++ ) {
    ContextInfo* contextInfo = &contextInfoList[i];
    if((contextInfo->flags & CM_SKIPPED_CONTEXT) == 0) {

      size_t stateIndex = getStateByteLocation((bpos - 1) & 7, (bpos == 0 ? c1 + 256u : c0) >> 1);
      uint8_t* pState = &contextInfo->hashElementInSlot012->states[stateIndex];
      StateTable::update(pState, y, rnd);

      const auto slot0 = &contextInfo->hashElementInSlot0->slot0;

      ASSUME(bpos >= 0);
      ASSUME(bpos <= 7);

      if (bpos == 0) {
        // update byte history and run statistics
        if (slot0->bitState < 3) {
          slot0->byte3 = slot0->byte2 = slot0->byte1 = c1;
          slot0->runcount = 1;
        }
        else {
          const bool isMatch = slot0->byte1 == c1;
          if (isMatch) {
            uint8_t runCount = slot0->runcount;
            if (runCount < 255) {
              slot0->runcount = runCount + 1;
            }
          }
          else {
            // shift byte candidates
            slot0->runcount = 1;
            slot0->byte3 = slot0->byte2;
            slot0->byte2 = slot0->byte1;
            slot0->byte1 = c1; //last byte seen
          }
        }
      }
      else if( bpos==2 || bpos==5 ) {
        if (contextInfo->flags & CM_DEFERRED) { //when in deferred mode...
          // ...reconstruct bit states in temporary location from last seen bytes 

          memset(&contextInfo->bitStateTmp, 0, 7);
          contextInfo->hashElementInSlot012 = &contextInfo->bitStateTmp;
          
          const uint8_t bit0state = slot0->bitState;
          if (bit0state >= 3) { // at least 1 byte was seen
            const uint8_t byte2 = bit0state >= 7 ? slot0->byte2 : slot0->byte1;
            const uint8_t mask = ((1 << bpos) - 1);
            const int shift = 8 - bpos;
            if (((c0 ^ (byte2 >> shift)) & mask) == 0) { // last 2/5 bits must match otherwise it's not the current slot location
              updatePendingContextsInSlot(&contextInfo->bitStateTmp, byte2 >> (shift - 3)); // simulate the current states at the temporary location
            }
            if (bit0state >= 7) { // at least 2 bytes were seen
              const uint8_t byte1 = slot0->byte1;
              if (((c0 ^ (byte1 >> shift)) & mask) == 0) { // last 2/5 bits must match otherwise it's not the current slot location
                updatePendingContextsInSlot(&contextInfo->bitStateTmp, byte1 >> (shift - 3)); // simulate the current states at the temporary location
              }
            }
          }
        }
        else {
          //when pbos==2: switch from slot 0 to slot 1
          //when bpos==5: switch from slot 1 to slot 2
          const uint32_t ctx = contextInfo->tableIndex;
          const uint16_t chk = contextInfo->tableChecksum;
          contextInfo->hashElementInSlot012 = hashTable[(ctx + c0) & mask].find(chk, &rnd);
        }
      }
    }
  }
  if( bpos == 0 ) {
    index = 0;
    contextflagsAll = 0;
  } // start over
}

void ContextMap2::setScale(const int Scale) { scale = Scale; }

void ContextMap2::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  stateMap.subscribe();
  if ((contextflagsAll & CM_USE_RUN_STATS) != 0)
    runMap.subscribe();
  if ((contextflagsAll & CM_USE_BYTE_HISTORY) != 0) {
    bhMap8B.subscribe();
    bhMap12B.subscribe();
  }

  order = 0;
  INJECT_SHARED_bpos
  INJECT_SHARED_c0
  for( uint32_t i = 0; i < index; i++ ) {
    ContextInfo* contextInfo = &contextInfoList[i];
    if((contextInfo->flags & CM_SKIPPED_CONTEXT) == 0 ) {

      const auto slot0   = &contextInfo->hashElementInSlot0->slot0;

      uint8_t* pState = &contextInfo->hashElementInSlot012->states[0] + getStateByteLocation(bpos, c0);
      const int state = *pState;
      const int n0 = StateTable::getNextState(state, 2);
      const int n1 = StateTable::getNextState(state, 3);
      const int bitIsUncertain = int(n0 != 0 && n1 != 0);

      // predict from last byte(s) in context
      uint8_t byteState = slot0->bitState;
      const bool complete1 = (byteState >= 3) || (byteState >= 1 && bpos == 0);
      const bool complete2 = (byteState >= 7) || (byteState >= 3 && bpos == 0);
      const bool complete3 = (byteState >= 15) || (byteState >= 7 && bpos == 0);
      if((contextInfo->flags & CM_USE_RUN_STATS) != 0 ) {
        const int bp = (UINT32_C(0x33322210) >> (bpos << 2)) & 0xF; // {bpos:0}->0  {bpos:1}->1  {bpos:2,3,4}->2  {bpos:5,6,7}->3
        bool skipRunMap = true;
        if( complete1 ) {
          if(((slot0->byte1 + 256u) >> (8 - bpos)) == c0 ) { // 1st candidate (last byte seen) matches
            const int predictedBit = (slot0->byte1 >> (7 - bpos)) & 1;
            const int byte1IsUncertain = static_cast<const int>(slot0->byte2 != slot0->byte1);
            const int runCount = slot0->runcount; // 1..255
            m.add(stretch(runMap.p2(i, runCount << 4 | bp << 2 | byte1IsUncertain << 1 | predictedBit)) >> (1 + byte1IsUncertain));
            skipRunMap = false;
          } else if( complete2 && ((slot0->byte2 + 256u) >> (8 - bpos)) == c0 ) { // 2nd candidate matches
            const int predictedBit = (slot0->byte2 >> (7 - bpos)) & 1;
            const int byte2IsUncertain = static_cast<const int>(slot0->byte3 != slot0->byte2);
            m.add(stretch(runMap.p2(i, bitIsUncertain << 1 | predictedBit)) >> (2 + byte2IsUncertain));
            skipRunMap = false;
          }
          // remark: considering the 3rd byte is not beneficial in most cases, except for some 8bpp images
        }
        if( skipRunMap ) {
          runMap.skip(i);
          m.add(0);
        }
      }
      else if ((contextflagsAll & CM_USE_RUN_STATS) != 0) {
        runMap.skip(i);
      }
      // predict from bit context
      if( state == 0 ) {
        stateMap.skip(i);
        m.add(0);
        m.add(0);
        m.add(0);
        m.add(0);
      } else {
        const int p1 = stateMap.p2(i, state);
        const int st = (stretch(p1) * scale) >> 8;
        const int contextIsYoung = int(state <= 2);
        m.add(st >> contextIsYoung);
        m.add(((p1 - 2048) * scale) >> 9);
        m.add((bitIsUncertain - 1) & st); // when both counts are nonzero add(0) otherwise add(st)
        const int p0 = 4095 - p1;
        m.add((((p1 & (-!n0)) - (p0 & (-!n1))) * scale) >> 10);
        order++;
      }

      if((contextInfo->flags & CM_USE_BYTE_HISTORY) != 0 ) {
        const int bhBits = 
          (((slot0->byte1 >> (7 - bpos)) & 1)) |
          (((slot0->byte2 >> (7 - bpos)) & 1) << 1) |
          (((slot0->byte3 >> (7 - bpos)) & 1) << 2);

        int bhState = 0; // 4 bits
        if( complete3 ) {
          bhState = 8 | (bhBits); //we have seen 3 bytes (at least)
        } else if( complete2 ) {
          bhState = 4 | (bhBits & 3); //we have seen 2 bytes
        } else if( complete1 ) {
          bhState = 2 | (bhBits & 1); //we have seen 1 byte only
        }
        //else new context (bhState=0)

        const uint8_t stateGroup = StateTable::group(state); //0..31
        m.add(stretch(bhMap8B.p2(i, bitIsUncertain << 7 | (bhState << 3) | bpos)) >> 2); // using bitIsUncertain is generally beneficial except for some 8bpp image (noticeable loss)
        m.add(stretch(bhMap12B.p2(i, stateGroup << 7 | (bhState << 3) | bpos)) >> 2);
      }
      else if ((contextflagsAll & CM_USE_BYTE_HISTORY) != 0) {
        bhMap8B.skip(i);
        bhMap12B.skip(i);
      }
    } else { //skipped context

      stateMap.skip(i);
      m.add(0);
      m.add(0);
      m.add(0);
      m.add(0);

      if((contextflagsAll & CM_USE_RUN_STATS) != 0) {
        runMap.skip(i);
        if((contextInfo->flags & CM_USE_RUN_STATS) != 0)
          m.add(0);
      }

      if((contextflagsAll & CM_USE_BYTE_HISTORY) != 0 ) {
        bhMap8B.skip(i);
        bhMap12B.skip(i);
        if ((contextInfo->flags & CM_USE_BYTE_HISTORY) != 0) {
          m.add(0);
          m.add(0);
        }
      }
    }
  }
}

// ===== src/Encoder.cpp =====

Encoder::Encoder(Predictor* predictorBlock, Predictor* predictorMain, bool doEncoding, Mode m, File *f) :
  doEncoding(doEncoding), 
  ari(f), 
  mode(m), 
  archive(f), 
  alt(nullptr),
  predictorBlock(predictorBlock),
  predictorMain(predictorMain)
{
  if( mode == DECOMPRESS ) {
    uint64_t start = size();
    archive->setEnd();
    uint64_t end = size();
    if( end >= (UINT64_C(1) << 31)) {
      quit("Large archives not yet supported.");
    }
    setStatusRange(0.0, static_cast<float>(end));
    archive->setpos(start);
  }
  if( doEncoding && mode == DECOMPRESS ) { // x = first 4 bytes of archive
    ari.prefetch();
  }
}

Mode Encoder::getMode() const { return mode; }

uint64_t Encoder::size() const { return archive->curPos(); }

void Encoder::flush() {
  if(doEncoding && mode == COMPRESS) {
    ari.flush();
  }
}

void Encoder::setFile(File *f) { alt = f; }

void Encoder::compressByte(Predictor *predictor, uint8_t c) {
  assert(mode == COMPRESS);
  if (!doEncoding) {
    archive->putChar(c);
  } else {
    for( int i = 7; i >= 0; --i ) {
      uint32_t p = predictor->p();
      int y = (c >> i) & 1;
      ari.encodeBit(p, y);
      updateModels(predictor, p, y);
    }
    assert(predictor->shared->State.c1 == c);
  }
}

uint8_t Encoder::decompressByte(Predictor *predictor) {
  if( mode == COMPRESS ) {
    assert(alt);
    return alt->getchar();
  }
  if (!doEncoding) {
    return archive->getchar();
  } else {
    for( int i = 0; i < 8; ++i ) {
      int p = predictor->p();
      int y = ari.decodeBit(p);
      updateModels(predictor, p, y);
    }
    return predictor->shared->State.c1;
  }
}

void Encoder::initContextForBlockModel(BlockType blockType, int blockInfo) {
  if (doEncoding) {
    Shared* shared = predictorMain->shared;
    shared->State.blockType = blockType;
    shared->State.blockInfo = blockInfo;
    shared->State.blockPos = UINT32_MAX;
  }
}

void Encoder::setContextForBlockModel(uint8_t context) {
  if (doEncoding)
    predictorBlock->shared->State.blockStateID = context;
}

void Encoder::appendToBlockTypeHistoryForBlockModel(BlockType blockType) {
  if (doEncoding) {
    predictorBlock->shared->State.blockTypeHistory <<= 8;
    predictorBlock->shared->State.blockTypeHistory |= (uint8_t)blockType;
  }
}

void Encoder::updateModels(Predictor* predictor, uint32_t p, int y) {
  bool isMissed = ((p >> (ArithmeticEncoder::PRECISION - 1)) != y);
  predictor->shared->update(y, p, isMissed);
}


void Encoder::setStatusRange(float perc1, float perc2) {
  p1 = perc1;
  p2 = perc2;
}

void Encoder::printStatus(uint64_t n, uint64_t size) const {
  fprintf(stderr, "%6.2f%%\b\b\b\b\b\b\b", (p1 + (p2 - p1) * n / (size + 1)) * 100);
  fflush(stderr);
}

void Encoder::printStatus() const {
  fprintf(stderr, "%6.2f%%\b\b\b\b\b\b\b", float(size()) / (p2 + 1) * 100);
  fflush(stderr);
}

// ===== src/file/File.cpp =====

void File::append(const char *s) {
  for( int i = 0; s[i] != 0; i++ ) {
    putChar(static_cast<uint8_t>(s[i]));
  }
}

uint32_t  File::get32() { return (getchar() << 24) | (getchar() << 16) | (getchar() << 8) | (getchar()); }

void File::put32(uint32_t x) {
  putChar((x >> 24) & 255);
  putChar((x >> 16) & 255);
  putChar((x >> 8) & 255);
  putChar(x & 255);
}

uint64_t File::getVLI() {
  uint64_t i = 0;
  int k = 0;
  uint8_t b = 0;
  do {
    b = getchar();
    i |= static_cast<uint64_t>(b & 0x7F) << k;
    k += 7;
  } while((b >> 7) > 0 );
  return i;
}

void File::putVLI(uint64_t i) {
  while( i > 0x7F ) {
    putChar(0x80 | (i & 0x7F));
    i >>= 7;
  }
  putChar(uint8_t(i));
}


// ===== src/file/FileDisk.cpp =====

FILE* FileDisk::makeTmpFile() {
#if defined(WINDOWS)
  wchar_t szTempFileName[MAX_PATH];
  const UINT uRetVal = GetTempFileNameW(L".", L"tmp", 0, szTempFileName);
  if (uRetVal == 0) return nullptr;
  return fopen(Utf8Str(szTempFileName).utf8_str, "w+bTD");
#else
  return tmpfile();
#endif
}

FileDisk::FileDisk() { file = nullptr; }

FileDisk::~FileDisk() { close(); }

bool FileDisk::open(const char *filename, bool mustSucceed) {
  assert(file == nullptr);
  file = openFile(filename, READ);
  const bool success = (file != nullptr);
  if( !success && mustSucceed ) {
    printf("Unable to open file %s (%s)", filename, strerror(errno));
    quit();
  }
  return success;
}

void FileDisk::create(const char *filename) {
  assert(file == nullptr);
  makeDirectories(filename);
  file = openFile(filename, WRITE);
  if( file == nullptr ) {
    printf("Unable to create file %s (%s)", filename, strerror(errno));
    quit();
  }
}

void FileDisk::createTmp() {
  assert(file == nullptr);
  file = makeTmpFile();
  if( file == nullptr ) {
    printf("Unable to create temporary file (%s)", strerror(errno));
    quit();
  }
}

void FileDisk::close() {
  if( file != nullptr ) {
    fclose(file);
  }
  file = nullptr;
}

int FileDisk::getchar() { return fgetc(file); }

void FileDisk::putChar(uint8_t c) { fputc(c, file); }

uint64_t FileDisk::blockRead(uint8_t *ptr, uint64_t count) { return fread(ptr, 1, count, file); }

void FileDisk::blockWrite(uint8_t *ptr, uint64_t count) { fwrite(ptr, 1, count, file); }

void FileDisk::setpos(uint64_t newPos) { fseeko(file, newPos, SEEK_SET); }

void FileDisk::setEnd() { fseeko(file, 0, SEEK_END); }

uint64_t FileDisk::curPos() { return ftello(file); }

bool FileDisk::eof() { return feof(file) != 0; }

// ===== src/file/FileName.cpp =====

FileName::FileName(const char *s) : String(s) {}

int FileName::lastSlashPos() const {
  int pos = findLast('/');
  if( pos < 0 ) {
    pos = findLast('\\');
  }
  return pos; //has no path when -1
}

void FileName::keepFilename() {
  const int pos = lastSlashPos();
  stripStart(pos + 1); //don't keep last slash
}

void FileName::keepPath() {
  const int pos = lastSlashPos();
  stripEnd(strsize() - pos - 1); //keep last slash
}

void FileName::replaceSlashes() { //
  const uint64_t sSize = strsize();
  for( uint64_t i = 0; i < sSize; i++ ) {
    if((*this)[i] == BADSLASH ) {
      (*this)[i] = GOODSLASH;
    }
  }
  chk_consistency();
}


// ===== src/file/FileTmp.cpp =====

void FileTmp::forgetContentInRam() {
  if( contentInRam != nullptr ) {
    delete contentInRam;
    contentInRam = nullptr;
    filePos = 0;
    fileSize = 0;
  }
}

void FileTmp::forgetFileOnDisk() {
  if( fileOnDisk != nullptr ) {
    fileOnDisk->close();
    delete fileOnDisk;
    fileOnDisk = nullptr;
  }
}

void FileTmp::ramToDisk() {
  assert(fileOnDisk == nullptr);
  fileOnDisk = new FileDisk();
  fileOnDisk->createTmp();
  if( fileSize > 0 ) {
    fileOnDisk->blockWrite(&((*contentInRam)[0]), fileSize);
  }
  fileOnDisk->setpos(filePos);
  forgetContentInRam();
}

FileTmp::FileTmp() {
  contentInRam = new Array<uint8_t>(0);
  filePos = 0;
  fileSize = 0;
  fileOnDisk = nullptr;
}

FileTmp::~FileTmp() { close(); }

bool FileTmp::open(const char * /*filename*/, bool /*mustSucceed*/) {
  assert(false);
  return false;
}

void FileTmp::create(const char * /*filename*/) { assert(false); }

void FileTmp::close() {
  forgetContentInRam();
  forgetFileOnDisk();
}

int FileTmp::getchar() {
  if( contentInRam != nullptr ) {
    if( filePos >= fileSize ) {
      return EOF;
    }
    const uint8_t c = (*contentInRam)[filePos];
    filePos++;
    return c;
  }
  return fileOnDisk->getchar();
}

void FileTmp::putChar(uint8_t c) {
  if( contentInRam != nullptr ) {
    if( filePos < MAX_RAM_FOR_TMP_CONTENT ) {
      if( filePos == fileSize ) {
        contentInRam->pushBack(c);
        fileSize++;
      } else {
        (*contentInRam)[filePos] = c;
      }
      filePos++;
      return;
    }
    ramToDisk();
  }
  fileOnDisk->putChar(c);
}

uint64_t FileTmp::blockRead(uint8_t *ptr, uint64_t count) {
  if( contentInRam != nullptr ) {
    const uint64_t available = fileSize - filePos;
    if( available < count ) {
      count = available;
    }
    if( count > 0 ) {
      memcpy(ptr, &((*contentInRam)[filePos]), count);
    }
    filePos += count;
    return count;
  }
  return fileOnDisk->blockRead(ptr, count);
}

void FileTmp::blockWrite(uint8_t *ptr, uint64_t count) {
  if( contentInRam != nullptr ) {
    if( filePos + count <= MAX_RAM_FOR_TMP_CONTENT ) {
      contentInRam->resize((filePos + count));
      if( count > 0 ) {
        memcpy(&((*contentInRam)[filePos]), ptr, count);
      }
      fileSize += count;
      filePos += count;
      return;
    }
    ramToDisk();
  }
  fileOnDisk->blockWrite(ptr, count);
}

void FileTmp::setpos(uint64_t newPos) {
  if( contentInRam != nullptr ) {
    if( newPos > fileSize ) {
      ramToDisk(); //panic: we don't support seeking past end of file (but stdio does) - let's switch to disk
    } else {
      filePos = newPos;
      return;
    }
  }
  fileOnDisk->setpos(newPos);
}

void FileTmp::setEnd() {
  if( contentInRam != nullptr ) {
    filePos = fileSize;
  } else {
    fileOnDisk->setEnd();
  }
}

uint64_t FileTmp::curPos() {
  if( contentInRam != nullptr ) {
    return filePos;
  }

  return fileOnDisk->curPos();
}

bool FileTmp::eof() {
  if( contentInRam != nullptr ) {
    return filePos >= fileSize;
  }

  return fileOnDisk->eof();
}


// ===== src/filter/LZWDictionary.cpp =====

LZWDictionary::LZWDictionary() : index(0) { reset(); }

void LZWDictionary::reset() {
  memset(&dictionary, 0xFF, sizeof(dictionary));
  memset(&table, 0xFF, sizeof(table));
  for( int i = 0; i < 256; i++ ) {
    table[-findEntry(-1, i) - 1] = static_cast<short>(i);
    dictionary[i].suffix = i;
  }
  index = 258; //2 extra codes, one for resetting the dictionary and one for signaling EOF
}

int LZWDictionary::findEntry(const int prefix, const int suffix) {
  int i = finalize64(hash(prefix, suffix), 13);
  int offset = (i > 0) ? hashSize - i : 1;
  while( true ) {
    if( table[i] < 0 ) { //free slot?
      return -i - 1;
    }
    if( dictionary[table[i]].prefix == prefix && dictionary[table[i]].suffix == suffix ) { // is it the entry we want?
      return table[i];
    }
    i -= offset;
    if( i < 0 ) {
      i += hashSize;
    }
  }
}

void LZWDictionary::addEntry(const int prefix, const int suffix, const int offset) {
  if( prefix == -1 || prefix >= index || index > 4095 || offset >= 0 ) {
    return;
  }
  dictionary[index].prefix = prefix;
  dictionary[index].suffix = suffix;
  table[-offset - 1] = index;
  index += static_cast<int>(index < 4096);
}

int LZWDictionary::dumpEntry(File *f, int code) {
  int n = 4095;
  while( code > 256 && n >= 0 ) {
    buffer[n] = uint8_t(dictionary[code].suffix);
    n--;
    code = dictionary[code].prefix;
  }
  buffer[n] = uint8_t(code);
  f->blockWrite(&buffer[n], 4096 - n);
  return code;
}


// ===== src/filter/TextParserStateInfo.cpp =====

void TextParserStateInfo::reset(uint64_t startPos) {
  Start = startPos;
  End = startPos - 1;
  invalidCount = 0;
  EOLType = 0;
  UTF8State = 0;
}

uint64_t TextParserStateInfo::realLength() const {
  return End - Start + 1;
}

bool TextParserStateInfo::isLargeText() const {
  return realLength() >= TEXT_FRAGMENT_MIN_SIZE;
}

bool TextParserStateInfo::isSmallText() const {
  return realLength() >= TEXT_SMALL_BLOCK_SIZE;
}


// ===== src/Ilog.cpp =====

int Ilog::log(uint16_t x) const { return static_cast<int>(t[x]); }

Ilog& Ilog::getInstance() {
  static Ilog instance;
  return instance;
}

Ilog *ilog = &Ilog::getInstance();

int llog(uint32_t x) {
  if( x >= 0x1000000 ) {
    return 256 + ilog->log(uint16_t(x >> 16));
  }
  if( x >= 0x10000 ) {
    return 128 + ilog->log(uint16_t(x >> 8));
  }

  return ilog->log(uint16_t(x));
}


// ===== src/IndirectMap.cpp =====

IndirectMap::IndirectMap(const Shared* const sh, const int bitsOfContext, const int inputBits, const int scale, const int limit) : 
  shared(sh), data((UINT64_C(1) << bitsOfContext) * ((UINT64_C(1) << inputBits) - 1)),
  sm {sh, 1, 256, 1023, StateMapType::BitHistory}, mask((1 << bitsOfContext) - 1),
  maskBits(bitsOfContext), stride((1 << inputBits) - 1), bTotal(inputBits), scale(scale) {
  assert(inputBits > 0 && inputBits <= 8);
  assert(bitsOfContext + inputBits <= 24);
  cp = nullptr;
  setDirect(0);
}

void IndirectMap::setDirect(const uint32_t ctx) {
  context = (ctx & mask) * stride;
  bCount = b = 0;
}

void IndirectMap::set(const uint64_t ctx) {
  context = (finalize64(ctx, maskBits)) * stride;
  bCount = b = 0;
}

void IndirectMap::update() {
  INJECT_SHARED_y
  StateTable::update(cp, y, rnd);
  b += static_cast<uint32_t>((y != 0) && b > 0);
}

void IndirectMap::setScale(const int Scale) { this->scale = Scale; }

void IndirectMap::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  cp = &data[context + b];
  const uint8_t state = *cp;
  if (state == 0) {
    m.add(0);
    m.add(0);
  }
  else {
    const int p1 = sm.p1(state);
    m.add((stretch(p1) * scale) >> 8);
    m.add(((p1 - 2048) * scale) >> 9);
  }
  bCount++;
  b += b + 1;
  assert(bCount <= bTotal);
}


// ===== src/LargeStationaryMap.cpp =====

LargeStationaryMap::LargeStationaryMap(const Shared* const sh, const int contexts, const int hashBits, const int scale) :
  shared(sh),
  rnd(),
  data((UINT64_C(1) << hashBits)),
  hashBits(hashBits),
  scale(scale),
  numContexts(contexts),
  currentContextIndex(0),
  contextPointers(contexts) {
  assert(hashBits > 0);
  assert(hashBits <= 24); // 24 is just a reasonable limit for memory use 
  reset();
  dt = DivisionTable::getDT(); 
}

void LargeStationaryMap::set(const uint64_t contextHash) {
  assert(currentContextIndex < numContexts);
  uint32_t hashkey = finalize64(contextHash, hashBits);
  uint16_t checksum = checksum16(contextHash, hashBits);
  uint32_t* cp = &data[hashkey].find(checksum, &rnd)->value;
  // Note: the hashtable's MTF logic may physically move elements within a bucket.
  // If two contexts share the same bucket, the second find() shifts the first
  // element down, invalidating the pointer stored for it. Both pointers then
  // refer to the same element: one context's stats are never updated, the other's
  // are updated twice, and mix() reads the wrong stats for the first context.
  // This is rare and behaves like an undetected hash collision
  // -> acceptable, nothing to do.
  contextPointers[currentContextIndex] = cp;
  currentContextIndex++;
}

void LargeStationaryMap::setscale(const int scale) {
  this->scale = scale;
}

void LargeStationaryMap::reset() {
  for (uint32_t i = 0; i < data.size(); i++) {
    data[i].reset();
  }
}

void LargeStationaryMap::update() {
  assert(currentContextIndex <= numContexts);
  while (currentContextIndex > 0) {
    currentContextIndex--;
    uint32_t* const cp = contextPointers[currentContextIndex];
    if (cp == nullptr) {
      continue; // skipped context
    }
    update(cp);
  }
}

void LargeStationaryMap::update(uint32_t* cp) {

  // XOR cells with (2048<<22 = 0x80000000) on read and on write back:
  // The hashtable zero-initialises new slots unconditionally, with no hook to
  // customise it. A raw zero would decode as p1=0 but we need p1 = 0.5
  // XORing with (2048<<22) on read maps zero storage to p1=0.5,
  // and the same XOR in update() maps the probability back before storing.
  const uint32_t cell = *cp ^ 0x80000000;
  const int sum = cell & 1023;      //sum=n0+n1
  const int64_t y = shared->State.y;
  const int64_t target_22bit = y << 22;
  const int64_t p1_22bit = cell >> 10;  //prediction
  const int delta_22bit = int(((target_22bit - p1_22bit) * int64_t(dt[sum])) >> 30); // see derivation above
  (*cp) = 0x80000000 ^ (uint32_t(p1_22bit + delta_22bit) << 10) | (sum < 1023 ? sum + 1 : sum);

}

void LargeStationaryMap::mix(Mixer& m) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  assert(currentContextIndex == numContexts);
  for (size_t i = 0; i < currentContextIndex; i++) {
    const uint32_t* const cp = contextPointers[i];
    const uint32_t cell = *cp ^ 0x80000000;

    int p1 = cell >> 20; // top 12 bits of the probability
    const int st = (stretch(p1) * scale) >> 8;
    m.add(st);
    m.add(((p1 - 2048) * scale) >> 9);

    // recover n1 for yet another mixer input
    const uint32_t p1_22bit = cell >> 10;
    const uint32_t sum = cell & 1023;
    const uint32_t n1 = ((sum + 1) * p1_22bit) >> 22; // 0..1023 // `sum+1` instead of `sum` is to counteract truncation bias in p1
    const int bitIsUncertain = int(sum <= 1 || (n1 < sum - 1 && n1 != 0)); // `n1 < sum-1` instead of `n1 != sum` to counteract truncation bias in p1
    m.add((bitIsUncertain - 1) & st); // when both counts are nonzero add(0) otherwise add(st)
  }
}

void LargeStationaryMap::subscribe() {
  shared->GetUpdateBroadcaster()->subscribe(this);
}

void LargeStationaryMap::skip() {
  assert(currentContextIndex < numContexts);
  contextPointers[currentContextIndex] = nullptr; // mark for skipping
  currentContextIndex++;
}

// ===== src/Mixer_AVX2.cpp =====

#ifdef X64_SIMD_AVAILABLE

static constexpr int SIMD_WIDTH_AVX2 = 32 / sizeof(short); // 16 shorts per 256-bit lane

Mixer_AVX2::Mixer_AVX2(const Shared* const sh, const int n, const int m, const int s, const int promoted)
  : Mixer(sh, n, m, s, promoted, SIMD_WIDTH_AVX2) {
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx2")))
#endif
int Mixer_AVX2::dotProduct(const short* const t, const short* const w, const size_t n) {
  __m256i sum = _mm256_setzero_si256();

  for (size_t i = 0; i < n; i += 16) {
    __m256i tmp = _mm256_madd_epi16(*(const __m256i*) & t[i], *(const __m256i*) & w[i]);
    tmp = _mm256_srai_epi32(tmp, 8);
    sum = _mm256_add_epi32(sum, tmp);
  }

  __m128i lo = _mm256_extractf128_si256(sum, 0);
  __m128i hi = _mm256_extractf128_si256(sum, 1);

  __m128i newSum = _mm_hadd_epi32(lo, hi);
  newSum = _mm_add_epi32(newSum, _mm_srli_si128(newSum, 8));
  newSum = _mm_add_epi32(newSum, _mm_srli_si128(newSum, 4));
  return _mm_cvtsi128_si32(newSum);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx2")))
#endif
int Mixer_AVX2::dotProduct2(const short* const t, const short* const w0, const short* const w1, const size_t n, int& sum1) {
  __m256i s0 = _mm256_setzero_si256();
  __m256i s1 = _mm256_setzero_si256();

  for (size_t i = 0; i < n; i += 16) {
    const __m256i tv = *(const __m256i*) & t[i];
    __m256i tmp0 = _mm256_madd_epi16(tv, *(const __m256i*) & w0[i]);
    __m256i tmp1 = _mm256_madd_epi16(tv, *(const __m256i*) & w1[i]);
    s0 = _mm256_add_epi32(s0, _mm256_srai_epi32(tmp0, 8));
    s1 = _mm256_add_epi32(s1, _mm256_srai_epi32(tmp1, 8));
  }

  __m128i lo0 = _mm256_extractf128_si256(s0, 0);
  __m128i hi0 = _mm256_extractf128_si256(s0, 1);
  __m128i r0 = _mm_hadd_epi32(lo0, hi0);
  r0 = _mm_add_epi32(r0, _mm_srli_si128(r0, 8));
  r0 = _mm_add_epi32(r0, _mm_srli_si128(r0, 4));

  __m128i lo1 = _mm256_extractf128_si256(s1, 0);
  __m128i hi1 = _mm256_extractf128_si256(s1, 1);
  __m128i r1 = _mm_hadd_epi32(lo1, hi1);
  r1 = _mm_add_epi32(r1, _mm_srli_si128(r1, 8));
  r1 = _mm_add_epi32(r1, _mm_srli_si128(r1, 4));

  sum1 = _mm_cvtsi128_si32(r1);
  return _mm_cvtsi128_si32(r0);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx2")))
#endif
void Mixer_AVX2::train(const short* const t, short* const w, const size_t n, const int e) {
  const __m256i one = _mm256_set1_epi16(1);
  const __m256i err = _mm256_set1_epi16(short(e));

  for (size_t i = 0; i < n; i += 16) {
    __m256i tmp = _mm256_adds_epi16(*(const __m256i*) & t[i], *(const __m256i*) & t[i]);
    tmp = _mm256_mulhi_epi16(tmp, err);
    tmp = _mm256_adds_epi16(tmp, one);
    tmp = _mm256_srai_epi16(tmp, 1);
    tmp = _mm256_adds_epi16(tmp, *reinterpret_cast<__m256i*>(&w[i]));
    *reinterpret_cast<__m256i*>(&w[i]) = tmp;
  }
}

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer_AVX512.cpp =====

#ifdef X64_SIMD_AVAILABLE

static constexpr int SIMD_WIDTH_AVX512 = 64 / sizeof(short); // 32 shorts per 512-bit lane

Mixer_AVX512::Mixer_AVX512(const Shared* const sh, const int n, const int m, const int s, const int promoted)
  : Mixer(sh, n, m, s, promoted, SIMD_WIDTH_AVX512) {
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx512bw")))
#endif
int Mixer_AVX512::dotProduct(const short* const t, const short* const w, const size_t n) {
  __m512i sum = _mm512_setzero_si512();

  for (size_t i = 0; i < n; i += 32) {
    __m512i tmp = _mm512_madd_epi16(*(const __m512i*)&t[i], *(const __m512i*)&w[i]);
    tmp = _mm512_srai_epi32(tmp, 8);
    sum = _mm512_add_epi32(sum, tmp);
  }

  __m256i lo = _mm512_extracti64x4_epi64(sum, 0);
  __m256i hi = _mm512_extracti64x4_epi64(sum, 1);

  __m256i newSum1 = _mm256_add_epi32(lo, hi);
  __m128i newSum2 = _mm_add_epi32(_mm256_extractf128_si256(newSum1, 0), _mm256_extractf128_si256(newSum1, 1));
  newSum2 = _mm_add_epi32(newSum2, _mm_srli_si128(newSum2, 8));
  newSum2 = _mm_add_epi32(newSum2, _mm_srli_si128(newSum2, 4));
  return _mm_cvtsi128_si32(newSum2);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx512bw")))
#endif
int Mixer_AVX512::dotProduct2(const short* const t, const short* const w0, const short* const w1, const size_t n, int& sum1) {
  __m512i s0 = _mm512_setzero_si512();
  __m512i s1 = _mm512_setzero_si512();

  for (size_t i = 0; i < n; i += 32) {
    const __m512i tv = *(const __m512i*)&t[i];
    __m512i tmp0 = _mm512_madd_epi16(tv, *(const __m512i*)&w0[i]);
    __m512i tmp1 = _mm512_madd_epi16(tv, *(const __m512i*)&w1[i]);
    s0 = _mm512_add_epi32(s0, _mm512_srai_epi32(tmp0, 8));
    s1 = _mm512_add_epi32(s1, _mm512_srai_epi32(tmp1, 8));
  }

  __m256i lo0 = _mm512_extracti64x4_epi64(s0, 0);
  __m256i hi0 = _mm512_extracti64x4_epi64(s0, 1);
  __m256i sum0 = _mm256_add_epi32(lo0, hi0);
  __m128i r0 = _mm_add_epi32(_mm256_extractf128_si256(sum0, 0), _mm256_extractf128_si256(sum0, 1));
  r0 = _mm_add_epi32(r0, _mm_srli_si128(r0, 8));
  r0 = _mm_add_epi32(r0, _mm_srli_si128(r0, 4));

  __m256i lo1 = _mm512_extracti64x4_epi64(s1, 0);
  __m256i hi1 = _mm512_extracti64x4_epi64(s1, 1);
  __m256i sum1v = _mm256_add_epi32(lo1, hi1);
  __m128i r1 = _mm_add_epi32(_mm256_extractf128_si256(sum1v, 0), _mm256_extractf128_si256(sum1v, 1));
  r1 = _mm_add_epi32(r1, _mm_srli_si128(r1, 8));
  r1 = _mm_add_epi32(r1, _mm_srli_si128(r1, 4));

  sum1 = _mm_cvtsi128_si32(r1);
  return _mm_cvtsi128_si32(r0);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("avx512bw")))
#endif
void Mixer_AVX512::train(const short* const t, short* const w, const size_t n, const int e) {
  const __m512i one = _mm512_set1_epi16(1);
  const __m512i err = _mm512_set1_epi16(short(e));

  for (size_t i = 0; i < n; i += 32) {
    __m512i tmp = _mm512_adds_epi16(*(const __m512i*)&t[i], *(const __m512i*)&t[i]);
    tmp = _mm512_mulhi_epi16(tmp, err);
    tmp = _mm512_adds_epi16(tmp, one);
    tmp = _mm512_srai_epi16(tmp, 1);
    tmp = _mm512_adds_epi16(tmp, *reinterpret_cast<__m512i*>(&w[i]));
    *reinterpret_cast<__m512i*>(&w[i]) = tmp;
  }
}

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer_Neon.cpp =====

#ifdef ARM_NEON_AVAILABLE

static constexpr int SIMD_WIDTH_NEON = 16 / sizeof(short); // 8 shorts per 128-bit lane

// NEON helpers emulating the x86 intrinsics used in the algorithm
#if (defined(__GNUC__) || defined(__clang__))
static inline int32x4_t neon_mulhi_epi16(int32x4_t a, int32x4_t b) {
  int32x4_t rl = vmull_s16(vget_low_s16(vreinterpretq_s16_s32(a)), vget_low_s16(vreinterpretq_s16_s32(b)));
  int32x4_t rh = vmull_s16(vget_high_s16(vreinterpretq_s16_s32(a)), vget_high_s16(vreinterpretq_s16_s32(b)));
  uint16x8x2_t r = vuzpq_u16(vreinterpretq_u16_s32(rl), vreinterpretq_u16_s32(rh));
  return vreinterpretq_s32_u16(r.val[1]);
}

static inline int32x4_t neon_madd_epi16(int32x4_t a, int32x4_t b) {
  int32x4_t pl = vmull_s16(vget_low_s16(vreinterpretq_s16_s32(a)), vget_low_s16(vreinterpretq_s16_s32(b)));
  int32x4_t ph = vmull_s16(vget_high_s16(vreinterpretq_s16_s32(a)), vget_high_s16(vreinterpretq_s16_s32(b)));
  int32x2_t rl = vpadd_s32(vget_low_s32(pl), vget_high_s32(pl));
  int32x2_t rh = vpadd_s32(vget_low_s32(ph), vget_high_s32(ph));
  return vcombine_s32(rl, rh);
}
#endif

Mixer_Neon::Mixer_Neon(const Shared* const sh, const int n, const int m, const int s, const int promoted)
  : Mixer(sh, n, m, s, promoted, SIMD_WIDTH_NEON) {
}

int Mixer_Neon::dotProduct(const short* const t, const short* const w, const size_t n) {
  int32x4_t sum = vdupq_n_s32(0);

  for (size_t i = 0; i < n; i += 8) {
    int32x4_t tmp = neon_madd_epi16(*(int32x4_t*)&t[i], *(int32x4_t*)&w[i]);
    tmp = vshrq_n_s32(tmp, 8);
    sum = vaddq_s32(sum, tmp);
  }

  sum = vaddq_s32(sum, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(sum), vdupq_n_s8(0), 8)));
  sum = vaddq_s32(sum, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(sum), vdupq_n_s8(0), 4)));
  return vgetq_lane_s32(sum, 0);
}

int Mixer_Neon::dotProduct2(const short* const t, const short* const w0, const short* const w1, const size_t n, int& sum1) {
  int32x4_t s0 = vdupq_n_s32(0);
  int32x4_t s1 = vdupq_n_s32(0);

  for (size_t i = 0; i < n; i += 8) {
    const int32x4_t tv = *(int32x4_t*)&t[i];
    int32x4_t tmp0 = neon_madd_epi16(tv, *(int32x4_t*)&w0[i]);
    int32x4_t tmp1 = neon_madd_epi16(tv, *(int32x4_t*)&w1[i]);
    s0 = vaddq_s32(s0, vshrq_n_s32(tmp0, 8));
    s1 = vaddq_s32(s1, vshrq_n_s32(tmp1, 8));
  }

  s0 = vaddq_s32(s0, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(s0), vdupq_n_s8(0), 8)));
  s0 = vaddq_s32(s0, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(s0), vdupq_n_s8(0), 4)));
  s1 = vaddq_s32(s1, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(s1), vdupq_n_s8(0), 8)));
  s1 = vaddq_s32(s1, vreinterpretq_s32_s8(vextq_s8(vreinterpretq_s8_s32(s1), vdupq_n_s8(0), 4)));

  sum1 = vgetq_lane_s32(s1, 0);
  return vgetq_lane_s32(s0, 0);
}

void Mixer_Neon::train(const short* const t, short* const w, const size_t n, const int e) {
  const int32x4_t one = vreinterpretq_s32_s16(vdupq_n_s16(1));
  const int32x4_t err = vreinterpretq_s32_s16(vdupq_n_s16(short(e)));

  for (size_t i = 0; i < n; i += 8) {
    int32x4_t tmp = vreinterpretq_s32_s16(vqaddq_s16(vreinterpretq_s16_s32(*(int32x4_t*)&t[i]), vreinterpretq_s16_s32(*(int32x4_t*)&t[i])));
    tmp = neon_mulhi_epi16(tmp, err);
    tmp = vreinterpretq_s32_s16(vqaddq_s16(vreinterpretq_s16_s32(tmp), vreinterpretq_s16_s32(one)));
    tmp = vreinterpretq_s32_s16(vshrq_n_s16(vreinterpretq_s16_s32(tmp), 1));
    tmp = vreinterpretq_s32_s16(vqaddq_s16(vreinterpretq_s16_s32(tmp), vreinterpretq_s16_s32(*reinterpret_cast<int32x4_t*>(&w[i]))));
    *reinterpret_cast<int32x4_t*>(&w[i]) = tmp;
  }
}

#endif // ARM_NEON_AVAILABLE

// ===== src/Mixer_Scalar.cpp =====

static constexpr int SIMD_WIDTH_SCALAR = 4 / sizeof(short); // processes 2 shorts at once

Mixer_Scalar::Mixer_Scalar(const Shared* const sh, const int n, const int m, const int s, const int promoted)
  : Mixer(sh, n, m, s, promoted, SIMD_WIDTH_SCALAR) {
}

int Mixer_Scalar::dotProduct(const short* const t, const short* const w, const size_t n) {
  int sum = 0;
  for (size_t i = 0; i < n; i += 2) {
    sum += (t[i] * w[i] + t[i + 1] * w[i + 1]) >> 8;
  }
  return sum;
}

int Mixer_Scalar::dotProduct2(const short* const t, const short* const w0, const short* const w1, const size_t n, int& sum1) {
  int s0 = 0;
  int s1 = 0;
  for (size_t i = 0; i < n; i += 2) {
    const int t0 = t[i];
    const int t1 = t[i + 1];
    s0 += (t0 * w0[i] + t1 * w0[i + 1]) >> 8;
    s1 += (t0 * w1[i] + t1 * w1[i + 1]) >> 8;
  }
  sum1 = s1;
  return s0;
}

ALWAYS_INLINE
static int addsSaturate16(const int a, const int b) { // _mm_adds_epi16
  const int sum = a + b;
  if (sum < -32768) { return -32768; }
  if (sum > 32767) { return 32767; }
  return sum;
}

void Mixer_Scalar::train(const short* const t, short* const w, const size_t n, const int e) {
  for (size_t i = 0; i < n; i++) {
    const int t2 = addsSaturate16(t[i], t[i]);        // _mm_adds_epi16(t, t)
    const int prod = (t2 * e) >> 16;                  // _mm_mulhi_epi16
    const int rounded = addsSaturate16(prod, 1) >> 1; // _mm_adds_epi16(.,1), _mm_srai_epi16(.,1)
    w[i] = static_cast<short>(addsSaturate16(rounded, w[i]));
  }
}

// ===== src/Mixer_SSE2.cpp =====

#ifdef X64_SIMD_AVAILABLE

static constexpr int SIMD_WIDTH_SSE2 = 16 / sizeof(short); // 8 shorts per 128-bit lane

Mixer_SSE2::Mixer_SSE2(const Shared* const sh, const int n, const int m, const int s, const int promoted)
  : Mixer(sh, n, m, s, promoted, SIMD_WIDTH_SSE2) {
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("sse2")))
#endif
int Mixer_SSE2::dotProduct(const short* const t, const short* const w, const size_t n) {
  __m128i sum = _mm_setzero_si128();

  for (size_t i = 0; i < n; i += 8) {
    __m128i tmp = _mm_madd_epi16(*(const __m128i*) & t[i], *(const __m128i*) & w[i]);
    tmp = _mm_srai_epi32(tmp, 8);
    sum = _mm_add_epi32(sum, tmp);
  }

  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 8));
  sum = _mm_add_epi32(sum, _mm_srli_si128(sum, 4));
  return _mm_cvtsi128_si32(sum);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("sse2")))
#endif
int Mixer_SSE2::dotProduct2(const short* const t, const short* const w0, const short* const w1, const size_t n, int& sum1) {
  __m128i s0 = _mm_setzero_si128();
  __m128i s1 = _mm_setzero_si128();
  for (size_t i = 0; i < n; i += 8) {
    const __m128i tv = *(const __m128i*) & t[i];
    __m128i tmp0 = _mm_madd_epi16(tv, *(const __m128i*) & w0[i]);
    __m128i tmp1 = _mm_madd_epi16(tv, *(const __m128i*) & w1[i]);
    s0 = _mm_add_epi32(s0, _mm_srai_epi32(tmp0, 8));
    s1 = _mm_add_epi32(s1, _mm_srai_epi32(tmp1, 8));
  }
  s0 = _mm_add_epi32(s0, _mm_srli_si128(s0, 8));
  s0 = _mm_add_epi32(s0, _mm_srli_si128(s0, 4));
  s1 = _mm_add_epi32(s1, _mm_srli_si128(s1, 8));
  s1 = _mm_add_epi32(s1, _mm_srli_si128(s1, 4));
  sum1 = _mm_cvtsi128_si32(s1);
  return _mm_cvtsi128_si32(s0);
}

#if (defined(__GNUC__) || defined(__clang__))
__attribute__((target("sse2")))
#endif
void Mixer_SSE2::train(const short* const t, short* const w, const size_t n, const int e) {
  const __m128i one = _mm_set1_epi16(1);
  const __m128i err = _mm_set1_epi16(short(e));

  for (size_t i = 0; i < n; i += 8) {
    __m128i tmp = _mm_adds_epi16(*(const __m128i*) & t[i], *(const __m128i*) & t[i]);
    tmp = _mm_mulhi_epi16(tmp, err);
    tmp = _mm_adds_epi16(tmp, one);
    tmp = _mm_srai_epi16(tmp, 1);
    tmp = _mm_adds_epi16(tmp, *reinterpret_cast<__m128i*>(&w[i]));
    *reinterpret_cast<__m128i*>(&w[i]) = tmp;
  }
}

#endif // X64_SIMD_AVAILABLE

// ===== src/Mixer.cpp =====


#include <climits>
#include <initializer_list>

// Maximum stretched probability accepted from a model.
static constexpr int STRETCH_LIMIT = 2047;

// Maximum stretched value stored as a layer output. squash() saturates above
// 2047, but later layers can still use the full value through their linear
// dot product.
static constexpr int CLAMP_LIMIT = 4095;

// A skip input is the difference of two layer outputs, so its range is twice
// the normal layer-output range.
static constexpr int SKIP_LIMIT = 2 * CLAMP_LIMIT;
static_assert(SKIP_LIMIT <= 16383, "skip difference would saturate train()");

// Largest error between the target (0 or 4096) and a 12-bit prediction.
static constexpr int MAX_ERROR = 4095;

ALWAYS_INLINE
static int scaleDotProduct(const int dp, const int scaleFactor) {
  // Use 64 bits for the multiplication, then scale and clamp the result.
  const int64_t scaled = (int64_t(dp) * scaleFactor) >> 16;
  if (scaled < -CLAMP_LIMIT) { return -CLAMP_LIMIT; }
  if (scaled > CLAMP_LIMIT) { return CLAMP_LIMIT; }
  return int(scaled);
}

// Rounds @p v up to the next multiple of @p simdWidth.
static size_t padToWidth(const int v, const int simdWidth) {
  return size_t((v + (simdWidth - 1)) & -(simdWidth));
}

Mixer::Mixer(const Shared* const sh, const int n, const int m, const int s, const int promoted, const int simdWidth) :
  shared(sh),
  numInputs(size_t(n)),
  numPromotedInputs(size_t(promoted)),
  numPromoted(0),
  numAdded(0),
  bitContexts(uint32_t(m), uint32_t(s)),
  singleContext(1, 1),
  inputLayer(bitContexts, padToWidth(n, simdWidth),
    /*maxInput*/ STRETCH_LIMIT, /*errorLimit*/ 56, /*initialWeight*/ 128,
    /*lowerLimit*/ learningRate(DEFAULT_END_RATE_INPUT)),
  middleLayer(bitContexts, padToWidth(s + promoted, simdWidth),
    /*maxInput*/ CLAMP_LIMIT, /*errorLimit*/ 56, /*initialWeight*/ 4096,
    /*lowerLimit*/ learningRate(DEFAULT_END_RATE_MIDDLE)),
  // The combining layer has s middle outputs, s skip inputs, and promoted inputs.
  combiningLayer(singleContext, padToWidth(2 * s + promoted, simdWidth),
    /*maxInput*/ SKIP_LIMIT, /*errorLimit*/ 6, /*initialWeight*/ 8192,
    /*lowerLimit*/ learningRate(DEFAULT_END_RATE_COMBINING)) {
  assert(n > 0);
  assert(m > 0);
  assert(s > 0);
  assert(promoted >= 0);

  assert(isWellFormed(inputLayer, simdWidth));
  assert(isWellFormed(middleLayer, simdWidth));
  assert(isWellFormed(combiningLayer, simdWidth));

  // Set the input layout of the middle and combining layers.
  middleLayer.routed = { 0, size_t(s) };
  middleLayer.promoted = { size_t(s), numPromotedInputs };
  combiningLayer.routed = { 0, size_t(s) };
  combiningLayer.skip = { size_t(s), size_t(s) };
  combiningLayer.promoted = { size_t(2 * s), numPromotedInputs };
  assert(middleLayer.promoted.end() <= middleLayer.n);
  assert(combiningLayer.promoted.end() <= combiningLayer.n);

  // Skip weights are deliberately initialized to zero (see the class comment).
  setWeightBlock(combiningLayer, combiningLayer.skip, 0);

  reset();
}

Mixer::~Mixer() = default;

bool Mixer::isWellFormed(const Layer& layer, const int simdWidth) {
  // Inputs must be SIMD-aligned, and the layer must be small enough that its
  // dot products stay within the integer range supported by the SIMD kernels.
  return (layer.n & size_t(simdWidth - 1)) == 0
    && int64_t(layer.n) * layer.maxInput * 128 <= INT_MAX;
}

void Mixer::setWeightBlock(Layer& layer, const Block& block, const short value) {
  assert(block.end() <= layer.n);
  for (uint32_t r = 0; r < layer.contexts.m; ++r) {
    short* const w = &layer.wx[size_t(r) * layer.n];
    for (size_t i = 0; i < block.count; ++i) {
      w[block.offset + i] = value;
    }
  }
}

void Mixer::forward(Layer& layer) {
  assert(layer.scaleFactor > 0);

  const short* const t = &layer.tx[0];
  const ContextSet& ctx = layer.contexts;
  const size_t count = ctx.count;
  const size_t end = count & ~size_t(1);
  size_t i = 0;
  for (; i < end; i += 2) {
    int dp1 = 0;
    int dp0 = dotProduct2(t,
      &layer.wx[ctx.cxt[i + 0] * layer.n],
      &layer.wx[ctx.cxt[i + 1] * layer.n], layer.n, dp1);
    dp0 = scaleDotProduct(dp0, layer.scaleFactor);
    dp1 = scaleDotProduct(dp1, layer.scaleFactor);
    layer.out[i + 0] = static_cast<short>(dp0);
    layer.out[i + 1] = static_cast<short>(dp1);
    layer.pr[i + 0] = static_cast<short>(squash(dp0));
    layer.pr[i + 1] = static_cast<short>(squash(dp1));
  }
  if (i < count) {
    int dp = dotProduct(t, &layer.wx[ctx.cxt[i] * layer.n], layer.n);
    dp = scaleDotProduct(dp, layer.scaleFactor);
    layer.out[i] = static_cast<short>(dp);
    layer.pr[i] = static_cast<short>(squash(dp));
  }
}

void Mixer::routeOutputs(const Layer& from, Layer& to, const Block& block) {
  assert(block.count == from.contexts.count);
  assert(block.end() <= to.n);
  short* const dst = &to.tx[block.offset];
  for (size_t i = 0; i < block.count; ++i) {
    dst[i] = from.out[i];
  }
}

void Mixer::routeSkipConnection() {
  const Block& block = combiningLayer.skip;

  // Input output i and middle output i use the same mixer context
  assert(&inputLayer.contexts == &middleLayer.contexts);
  assert(block.count == inputLayer.contexts.count);

  short* const dst = &combiningLayer.tx[block.offset];
  for (size_t i = 0; i < block.count; ++i) {
    // The difference measures how the middle layer changed that signal.
    const int diff = int(inputLayer.out[i]) - int(middleLayer.out[i]);
    dst[i] = static_cast<short>(diff);
  }
}

void Mixer::setScaleFactor(const int sf0, const int sf1, const int sf2) {
  inputLayer.scaleFactor = sf0;
  middleLayer.scaleFactor = sf1;
  combiningLayer.scaleFactor = sf2;
}

void Mixer::setLowerLimitOfLearningRate(const int lr0, const int lr1, const int lr2) {
  assert(lr0 >= MIN_LEARNING_RATE_UNITS && lr0 <= MAX_LEARNING_RATE_UNITS);
  assert(lr1 >= MIN_LEARNING_RATE_UNITS && lr1 <= MAX_LEARNING_RATE_UNITS);
  assert(lr2 >= MIN_LEARNING_RATE_UNITS && lr2 <= MAX_LEARNING_RATE_UNITS);
  inputLayer.lowerLimitOfLearningRate = learningRate(lr0);
  middleLayer.lowerLimitOfLearningRate = learningRate(lr1);
  combiningLayer.lowerLimitOfLearningRate = learningRate(lr2);
}

void Mixer::promote(const int x) {
  assert(x >= -STRETCH_LIMIT && x <= STRETCH_LIMIT);
  assert(numPromoted < numPromotedInputs);

  const short v = static_cast<short>(x);
  middleLayer.tx[middleLayer.promoted.offset + numPromoted] = v;
  combiningLayer.tx[combiningLayer.promoted.offset + numPromoted] = v;
  ++numPromoted;
}

void Mixer::update() {
  // Keep the fixed-point intermediate values within the integer and short
  // ranges used by train().
  static_assert(int64_t(MAX_ERROR) * (MAX_LEARNING_RATE >> 2) <= INT_MAX,
    "error * rate overflows int");
  static_assert((int64_t(MAX_ERROR) * (MAX_LEARNING_RATE >> 2)) >> 16 <= SHRT_MAX,
    "scaled error does not fit a short");

  INJECT_SHARED_y
    const int target = y << 12;

  // Each layer trains its weight vectors selected for the last prediction.
  // The input and middle layers use the same context selections; the combining
  // layer uses its single weight vector.
  for (Layer* const layerPtr : { &inputLayer, & middleLayer, & combiningLayer }) {
    Layer& layer = *layerPtr;
    const ContextSet& ctx = layer.contexts;
    const int rate = layer.rate >> 2;

    for (size_t i = 0; i < ctx.count; ++i) {
      const int err = target - layer.pr[i];
      if (err < -layer.errorLimit || err > layer.errorLimit) {
        train(&layer.tx[0], &layer.wx[ctx.cxt[i] * layer.n], layer.n, (err * rate) >> 16);
      }
    }

    // Decay the learning rate one step per bit until it reaches its floor.
    if (layer.rate > layer.lowerLimitOfLearningRate) {
      layer.rate--;
    }
  }

  reset();
}

int Mixer::p() {
  shared->GetUpdateBroadcaster()->subscribe(this);

  // All per-bit context selections and promoted inputs must be present before
  // computing the final prediction.
  //assert(numAdded == numInputs);
  assert(bitContexts.count == bitContexts.s);
  assert(numPromoted == numPromotedInputs);

  // Input layer -> middle layer -> combining layer, with the input layer's
  // outputs also reaching the combining layer through the skip connection.
  // Promoted inputs were written directly into both later layers.
  forward(inputLayer);
  routeOutputs(inputLayer, middleLayer, middleLayer.routed);

  forward(middleLayer);
  routeOutputs(middleLayer, combiningLayer, combiningLayer.routed);
  routeSkipConnection();

  // The combining layer has one weight vector, selected by its single context.
  singleContext.select(0, 1);
  forward(combiningLayer);

  return combiningLayer.pr[0];
}

void Mixer::add(const int x) {
  assert(x >= -STRETCH_LIMIT && x <= STRETCH_LIMIT);
  assert(numAdded < numInputs);
  inputLayer.tx[numAdded++] = static_cast<short>(x);
}

void Mixer::set(const uint32_t cx, const uint32_t range) {
  // Select one weight vector from the next range belonging to this context.
  bitContexts.select(cx, range);
}

void Mixer::reset() {
  // Start the next bit with fresh input, promotion, and context-selection state.
  numAdded = 0;
  numPromoted = 0;
  bitContexts.reset();
  singleContext.reset();
}

// ===== src/MixerFactory.cpp =====

MixerFactory::MixerFactory(const Shared* const sh) : shared(sh) {}

Mixer* MixerFactory::createMixer(const int n, const int m, const int s, const int promoted) const {
  const SIMDType chosenSimd = shared->chosenSimd;
  if (chosenSimd == SIMDType::SIMD_NONE) {
    return new Mixer_Scalar(shared, n, m, s, promoted);
  }
#ifdef X64_SIMD_AVAILABLE
  else if (chosenSimd >= SIMDType::SIMD_AVX512) {
    return new Mixer_AVX512(shared, n, m, s, promoted);
  }
  else if (chosenSimd >= SIMDType::SIMD_AVX2) {
    return new Mixer_AVX2(shared, n, m, s, promoted);
  }
  else if (chosenSimd >= SIMDType::SIMD_SSE2) {
    return new Mixer_SSE2(shared, n, m, s, promoted);
  }
#endif
#ifdef ARM_NEON_AVAILABLE
  else if (chosenSimd == SIMDType::SIMD_NEON) {
    return new Mixer_Neon(shared, n, m, s, promoted);
  }
#endif
  assert(false);
  return nullptr;
}

// ===== src/model/BlockModel.hpp =====


class BlockModel {
private:
  static constexpr int nCM = 5;
  Shared* const shared;
  ContextMap2 cm;
public:
  static constexpr int MIXERINPUTS =
    nCM * (ContextMap2::MIXERINPUTS + ContextMap2::MIXERINPUTS_BYTE_HISTORY); //30
  static constexpr int MIXERCONTEXTS = ((int)BlockType::Count + 1) + (9 * 2 * 8) + (256); //428
  static constexpr int MIXERCONTEXTSETS = 3;
  BlockModel(Shared* const sh, const uint64_t cmSize);

  void mix(Mixer& m);

};


// ===== src/model/BlockModel.cpp =====

/**
 * Model for metadata: blocktype, blockinfo and blocksize
**/

BlockModel::BlockModel(Shared* const sh, const uint64_t cmSize) :
  shared(sh), cm(sh, cmSize, nCM, 64)
{
  assert(isPowerOf2(cmSize));
}

void BlockModel::mix(Mixer& m) {
  const uint32_t blockTypeHistory = shared->State.blockTypeHistory; // xx xx xx xx
  const uint8_t blockStateID = shared->State.blockStateID; //0x10, 0x20..0x23, 0x30..0x33
  uint8_t bytePos = (blockStateID & 0x0f); //0,1,2,3
  INJECT_SHARED_c4
  uint32_t extrabytes = c4 & (0xffffff >> ((3 - bytePos) * 8)); //last 0-1-2-3 bytes (depending on bytePos) from c4 
  bytePos = (blockStateID == 0x10) ? 0 : (bytePos + 1); //0,1,2,3,4
  const uint8_t pos9 = bytePos + (blockStateID >= 0x30 ? 4 : 0); //0,1,2,3,4,5,6,7,8
  INJECT_SHARED_bpos
  if (bpos == 0) {
    const uint8_t _H = CM_USE_BYTE_HISTORY;
    uint64_t i = 0;
    if (blockStateID == 0x10) { //blocktype
      cm.set(_H, hash(++i));
      cm.set(_H, hash(++i, blockTypeHistory & 0xff)); //1 history
      cm.set(_H, hash(++i, blockTypeHistory & 0xffff)); //2 history
      cm.set(_H, hash(++i, blockTypeHistory & 0xff00)); //1 sparse history
      cm.set(0, hash(++i, blockTypeHistory & 0xffffff)); //3 history
    }
    else { //blockSize and blockInfo
      if (extrabytes == 0) {
        cm.set(_H, hash(++i, blockStateID));
      }
      else {
        cm.skip(_H); ++i;
      }
      cm.set(_H, hash(++i, blockStateID, extrabytes));
      cm.set(_H, hash(++i, blockStateID, extrabytes, blockTypeHistory & 0xff)); //1 history
      cm.set(_H, hash(++i, blockStateID, extrabytes, blockTypeHistory & 0xffff)); //2 history
      cm.set(0, hash(++i, blockStateID, extrabytes, blockTypeHistory & 0xffffff)); //3 history
    }
  }
  cm.mix(m);

  m.set(pos9 == 0 ? 0 : 1 + (blockTypeHistory & 0xff), 1 + (int)BlockType::Count);
  m.set(pos9 << 4 | (extrabytes == 0) << 3 | bpos, 9 * 2 * 8);
  m.set(finalize64(hash(pos9, blockTypeHistory & 0xffff), 8), 256);
}



// ===== src/model/ContextModelGeneric.cpp =====

class ContextModelGeneric: public IContextModel {

private:
  Shared* const shared;
  Models* const models;
  Mixer* m;

public:
  ContextModelGeneric(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    m = mf->createMixer(
      1 +  //bias
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS
      ,
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE + NormalModel::MIXERCONTEXTS_POST
      ,
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE + NormalModel::MIXERCONTEXTSETS_POST
      ,
      0
    );
    m->setScaleFactor(980, 130, 90);
    m->setLowerLimitOfLearningRate(7, 3, 2);
  }

  int p() {

    m->add(256); //network bias

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);
    normalModel.mixPost(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    return m->p();
  }

  ~ContextModelGeneric() {
    delete m;
  }

};


// ===== src/model/ContextModelImage1.cpp =====

class ContextModelImage1 : public IContextModel {

private:
  Shared* const shared;
  Models* const models;
  Mixer* m;

public:
  ContextModelImage1(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    m = mf->createMixer (
      1 +  //bias
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS + 
      Image1BitModel::MIXERINPUTS
      ,
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE + NormalModel::MIXERCONTEXTS_POST +
      Image1BitModel::MIXERCONTEXTS
      ,
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE + NormalModel::MIXERCONTEXTSETS_POST +
      Image1BitModel::MIXERCONTEXTSETS
      ,
      0
    );
    m->setScaleFactor(800, 360, 80);
  }

  void setParam(int width) {
    Image1BitModel& image1BitModel = models->image1BitModel();
    image1BitModel.setParam(width);
  }

  int p() {

    m->add(256); //network bias

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);
    normalModel.mixPost(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    Image1BitModel& image1BitModel = models->image1BitModel();
    image1BitModel.mix(*m);

    return m->p();
  }

  ~ContextModelImage1() {
    delete m;
  }

};

// ===== src/model/ContextModelImage4.cpp =====

class ContextModelImage4 : public IContextModel {

private:
  Shared* const shared;
  Models* const models;
  Mixer* m;

public:
  ContextModelImage4(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    m = mf->createMixer (
      1 +  //bias
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS + 
      Image4BitModel::MIXERINPUTS
      ,
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE +
      Image4BitModel::MIXERCONTEXTS
      ,
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE + 
      Image4BitModel::MIXERCONTEXTSETS
      ,
      0
    );
    m->setScaleFactor(2048, 320, 256);
  }

  void setParam(int imageWidthInBytes) {
    Image4BitModel& image4BitModel = models->image4BitModel();
    image4BitModel.setParam(imageWidthInBytes);
  }

  int p() {

    m->add(256); //network bias

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    Image4BitModel& image4BitModel = models->image4BitModel();
    image4BitModel.mix(*m);

    return m->p();
  }

  ~ContextModelImage4() {
    delete m;
  }

};

// ===== src/model/ContextModelImage8.cpp =====

class ContextModelImage8 : public IContextModel {

private:
  Shared* const shared;
  Models* const models;
  Mixer* m;

public:
  ContextModelImage8(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    m = mf->createMixer (
      1 +  //bias
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS + 
      Image8BitModel::MIXERINPUTS
      ,
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE +
      Image8BitModel::MIXERCONTEXTS
      ,
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE + 
      Image8BitModel::MIXERCONTEXTSETS
      ,
      0
    );
  }

  void setParam(int width, uint32_t isGray) {
    Image8BitModel& image8BitModel = models->image8BitModel();
    image8BitModel.setParam(width, isGray);
    if (isGray)
      m->setScaleFactor(1300, 150, 100); // 1100-1400, 90-110
    else
      m->setScaleFactor(1600, 160, 110); // 1500-1800, 100-128
  }

  int p() {

    m->add(256); //network bias

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    Image8BitModel& image8BitModel = models->image8BitModel();
    image8BitModel.mix(*m);

    return m->p();
  }

  ~ContextModelImage8() {
    delete m;
  }

};

// ===== src/model/ContextModelImage24.cpp =====

class ContextModelImage24 : public IContextModel
{

private:
  Shared* const shared;
  Models* const models;
  Mixer* m0;
  Mixer* m1;
  Mixer* m2;

public:
  ContextModelImage24(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    int mixerinputs =
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS +
      Image24BitModel::MIXERINPUTS;
    int mixerContexts =
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE +
      Image24BitModel::MIXERCONTEXTS;
    int mixerContextSets =
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE +
      Image24BitModel::MIXERCONTEXTSETS;

    m0 = mf->createMixer(mixerinputs, mixerContexts, mixerContextSets, 0);
    m1 = mf->createMixer(mixerinputs, mixerContexts, mixerContextSets, 0);
    m2 = mf->createMixer(mixerinputs, mixerContexts, mixerContextSets, 0);

    m0->setScaleFactor(490, 180, 130);
    m1->setScaleFactor(620, 170, 135);
    m2->setScaleFactor(770, 160, 140);

    m0->setLowerLimitOfLearningRate(7, 2, 1);
    m1->setLowerLimitOfLearningRate(6, 2, 1);
    m2->setLowerLimitOfLearningRate(8, 2, 2);
  }

  void setParam(int width, int isAlpha) {
    Image24BitModel& image24BitModel = models->image24BitModel();
    image24BitModel.setParam(width, isAlpha);
  }

  int p() {

    Image24BitModel& image24BitModel = models->image24BitModel();
    image24BitModel.update();

    int color = image24BitModel.color;
    Mixer* m =
      color == 0 ? m0 :
      color == 1 ? m1 :
      color == 2 ? m2 : m2;

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    image24BitModel.mix(*m);

    return m->p();
  }

  ~ContextModelImage24() {
    delete m0;
    delete m1;
    delete m2;
  }

};

// ===== src/model/ContextModelJpeg.cpp =====

class ContextModelJpeg : public IContextModel
{

private:
  const Shared* const shared;
  Models* const models;
  Mixer* mEntropyCoded; // for the entropy coded image data, where JpegModel makes predictions
  Mixer* mSpecial;      // for bits whose value follows from the JPEG format (stuffing bytes, markers, restart intervals)
  Mixer* mNoJpeg;       // for all other bits

public:
  ContextModelJpeg(Shared* const sh, Models* const models, const MixerFactory* const mf) :
    shared(sh), models(models)
  {

    mEntropyCoded = mf->createMixer(
      1 + //bias
      NormalModel::MIXERINPUTS + MatchModel::MIXERINPUTS +
      JpegModel::MIXERINPUTS
      ,
      NormalModel::MIXERCONTEXTS_PRE + MatchModel::MIXERCONTEXTS +
      JpegModel::MIXERCONTEXTS
      ,
      NormalModel::MIXERCONTEXTSETS_PRE + MatchModel::MIXERCONTEXTSETS +
      JpegModel::MIXERCONTEXTSETS
      ,
      0
    );
    mEntropyCoded->setScaleFactor(1024, 160, 128);

    mSpecial = mf->createMixer(
      1 + //bias
      NormalModel::MIXERINPUTS + MatchModel::MIXERINPUTS +
      JpegModel::SPECIALMIXERINPUTS // the fixed prediction from JpegModel
      ,
      NormalModel::MIXERCONTEXTS_PRE + MatchModel::MIXERCONTEXTS +
      JpegModel::SPECIALMIXERCONTEXTS
      ,
      NormalModel::MIXERCONTEXTSETS_PRE + MatchModel::MIXERCONTEXTSETS +
      JpegModel::SPECIALMIXERCONTEXTSETS
      ,
      0
    );
    mSpecial->setScaleFactor(4096, 1024, 512); // this mixer only sees stuffing and marker bits

    mNoJpeg = mf->createMixer(
      1 + //bias
      NormalModel::MIXERINPUTS + MatchModel::MIXERINPUTS
      ,
      NormalModel::MIXERCONTEXTS_PRE + MatchModel::MIXERCONTEXTS
      ,
      NormalModel::MIXERCONTEXTSETS_PRE + MatchModel::MIXERCONTEXTSETS
      ,
      0
    );
    mNoJpeg->setScaleFactor(1500, 150, 120);
  }

  int p() {
    // jpegModel runs first, before any model adds an input:
    // - it sets shared->State.JPEG.state, which NormalModel::updateHashes() and
    //   the SSE stage read;
    // - its result tells which mixer to use for this bit.
    JpegModel& jpegModel = models->jpegModel();
    const JpegResult jpegResult = jpegModel.update();

    Mixer* const m =
      jpegResult == JpegResult::EntropyCoded ? mEntropyCoded :
      jpegResult == JpegResult::Special ? mSpecial :
      mNoJpeg;

    m->add(256); // bias

    NormalModel& normalModel = models->normalModel();
    normalModel.mix(*m);

    MatchModel& matchModel = models->matchModel();
    matchModel.mix(*m);

    switch (jpegResult) {
      case JpegResult::EntropyCoded: {
        jpegModel.mix(*m);
        break;
      }
      case JpegResult::Special: {
        m->add(jpegModel.specialInput());
        m->set(jpegModel.specialContext(), JpegModel::SPECIALMIXERCONTEXTS);
        break;
      }
      case JpegResult::NoJpeg: {
        // nothing extra to mix: the generic model set is not part of this image compressor
        break;
      }
    }

    return m->p();
  }

  ~ContextModelJpeg() {
    delete mEntropyCoded;
    delete mSpecial;
    delete mNoJpeg;
  }

};


// ===== src/model/ContextModel.cpp =====

ContextModel::ContextModel(Shared* const sh, Models* const models, const MixerFactory* const mf) :
  shared(sh), 
  models(models), 
  mixerFactory(mf)
{}

int ContextModel::p() {
  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    uint32_t& blpos = shared->State.blockPos;
    blpos++;
    if (blpos == 0) {

      BlockType blockType = shared->State.blockType;
      int blockInfo = shared->State.blockInfo;

      switch (blockType) {

        case BlockType::IMAGE1: {
          static ContextModelImage1 contextModelImage1{ shared, models, mixerFactory};
          int width = blockInfo;
          contextModelImage1.setParam(width);
          selectedContextModel = &contextModelImage1;
          break;
        }

        case BlockType::IMAGE4: {
          static ContextModelImage4 contextModelImage4{ shared, models, mixerFactory };
          int width = blockInfo;
          contextModelImage4.setParam(width);
          selectedContextModel = &contextModelImage4;
          break;
        }

        case BlockType::IMAGE8:
        case BlockType::IMAGE8GRAY: {
          static ContextModelImage8 contextModelImage8{ shared, models, mixerFactory };
          int isGray = blockType == BlockType::IMAGE8GRAY;
          int width = blockInfo & 0xffffff;
          contextModelImage8.setParam(width, isGray);
          selectedContextModel = &contextModelImage8;
          break;
        }

        case BlockType::IMAGE24:
        case BlockType::IMAGE32: {
          static ContextModelImage24 contextModelImage24{ shared, models, mixerFactory };
          int isAlpha = blockType == BlockType::IMAGE32;
          int width = blockInfo & 0xffffff;
          contextModelImage24.setParam(width, isAlpha);
          selectedContextModel = &contextModelImage24;
          break;
        }

        case BlockType::JPEG: {
          static ContextModelJpeg contextModelJpeg{ shared, models, mixerFactory };
          selectedContextModel = &contextModelJpeg;
          break;
        }

        default: {
          static ContextModelGeneric contextModelGeneric{ shared, models, mixerFactory };
          selectedContextModel = &contextModelGeneric;
          break;
        }
      }
    }
  }

  return selectedContextModel->p();

}


// ===== src/model/ContextModelBlock.cpp =====

ContextModelBlock::ContextModelBlock(Shared* const sh, const MixerFactory* const mf) : shared(sh) {
  m = mf->createMixer(
    1 +  //bias
    BlockModel::MIXERINPUTS
    ,
    BlockModel::MIXERCONTEXTS
    ,
    BlockModel::MIXERCONTEXTSETS
    ,
    0
  );
  m->setScaleFactor(10240, 4096, 3072);
}

int ContextModelBlock::p() {

  m->add(256); //network bias

  static BlockModel blockModel{ shared, 32768 };
  blockModel.mix(*m);

  return m->p();
}

ContextModelBlock::~ContextModelBlock() {
  delete m;
}

// ===== src/model/Image1BitModel.cpp =====
#include <algorithm>

//////////////////////////////////////////////////////////////////////////////////////////////////
// Small helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

static inline uint32_t bitCount64(uint64_t v) {
  return (uint32_t)(bitCount((uint32_t)v) + bitCount((uint32_t)(v >> 32)));
}

/**
 * A run length in 16 steps: exact for short runs, coarser for long ones. 0 = start of a row.
 */
static uint32_t runBucket(uint32_t runLength) {
  return runLength < 8 ? runLength :
    runLength < 10 ? 8 :
    runLength < 12 ? 9 :
    runLength < 16 ? 10 :
    runLength < 24 ? 11 :
    runLength < 32 ? 12 :
    runLength < 48 ? 13 :
    runLength < 64 ? 14 : 15;
}

/**
 * A match length in 8 steps. 0 = no match.
 */
static uint32_t matchBucket(uint32_t matchLength) {
  return matchLength == 0 ? 0 :
    matchLength < 4 ? 1 :
    matchLength < 8 ? 2 :
    matchLength < 16 ? 3 :
    matchLength < 32 ? 4 :
    matchLength < 64 ? 5 :
    matchLength < 256 ? 6 : 7;
}

/**
 * The length of an active match (1 or more) in 16 steps: exact up to 8, coarser above. The first
 * few pixels of a match are where its reliability changes the most.
 */
static uint32_t matchBucketFine(uint32_t matchLength) {
  return matchLength <= 8 ? matchLength - 1 :
    matchLength < 12 ? 8 :
    matchLength < 16 ? 9 :
    matchLength < 24 ? 10 :
    matchLength < 32 ? 11 :
    matchLength < 64 ? 12 :
    matchLength < 128 ? 13 :
    matchLength < 512 ? 14 : 15;
}

/**
 * How many pixels a match source differed in when it was taken, in 4 steps: none, 1-2, 3-6, more.
 */
static uint32_t residBucket(uint32_t r) {
  return r == 0 ? 0 : r <= 2 ? 1 : r <= 6 ? 2 : 3;
}

/**
 * The signed distance from the current pixel to a colour change on the row above, in 32 steps:
 * one step per pixel up to 8 either way, coarser beyond. 31 = no colour change left on that row.
 */
static uint32_t transBucket(int d) {
  if (d == INT32_MAX) return 31;
  if (d >= -8 && d <= 8) return static_cast<uint32_t>(d + 12); //4..20
  if (d < 0) {
    return d >= -16 ? 3 : d >= -32 ? 2 : d >= -64 ? 1 : 0;
  }
  return d <= 16 ? 21 : d <= 32 ? 22 : d <= 64 ? 23 : 24;
}

/**
 * How many pixels of a line of pixels (bit 0 = nearest) have the same colour as the nearest one.
 * The sentinel bit, one above the last pixel, stands for "all of them".
 */
static uint32_t runUp(const uint32_t col, const uint32_t sentinel) {
  const uint32_t d = ((col & 1) != 0 ? ~col : col) | sentinel;
  return static_cast<uint32_t>(bitCount((d & (0u - d)) - 1));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Sparse templates
//
// These three were not designed by hand but found by a search: pixels were added one at a time,
// each time the one that improved the model's predictions the most. The result is not a block but
// a scatter: the nearest pixels at about a stroke's width apart, and single pixels spread over the
// height of a letter, up to 14 rows up. Read together they give a rough picture of the letter
// around the current pixel. {dy, dx} as in at().
//////////////////////////////////////////////////////////////////////////////////////////////////

//the 15 pixels that predict the best on their own
//  6  ........x....
//  5  .............
//  4  .............
//  3  .......xxx...
//  2  ......x.x...x
//  1  ...x...xxxx..
//  0  x...x..x?
static const int8_t T15[15][2] = { {1,0},{0,-1},{1,1},{2,-2},{0,-4},{2,4},{1,2},{0,-8},{1,-5},{2,0},{3,1},{3,0},{6,0},{3,-1},{1,-1} };

//grown from the first 12 pixels of T15, by what they add to the rest of the model
// 11  x...............
// 10  ...x.....x....x.
//  9  ................
//  8  ............x...
//  7  ................
//  6  ......x.x....x.x
//  5  ................
//  4  ................
//  3  ........xx......
//  2  ......x.x...x.x.
//  1  ...x....xxx.....
//  0  x...x..x?
static const int8_t G22[22][2] = { {1,0},{0,-1},{1,1},{2,-2},{0,-4},{2,4},{1,2},{0,-8},{1,-5},{2,0},{3,1},{3,0},
  {11,-8},{6,0},{6,5},{10,1},{10,-5},{2,6},{6,-2},{6,7},{10,6},{8,4} };

//grown from the first 8 pixels of T15, by what they add to the rest of the model including G22
// 14  ..........x.....
// 13  ................
// 12  .....x.......x..
// 11  ................
// 10  .x....x.x.......
//  9  ................
//  8  ................
//  7  ................
//  6  ..x.....x.x...x.
//  5  ................
//  4  ......x.x...x...
//  3  ................
//  2  ......x..x..x..x
//  1  ........xxx.....
//  0  x...x..x?
static const int8_t H23[23][2] = { {1,0},{0,-1},{1,1},{2,-2},{0,-4},{2,4},{1,2},{0,-8},
  {10,-7},{2,1},{12,-3},{6,6},{10,0},{6,-6},{12,5},{2,7},{4,4},{4,0},{10,-2},{4,-2},{6,0},{6,2},{14,2} };

//////////////////////////////////////////////////////////////////////////////////////////////////
// Construction
//////////////////////////////////////////////////////////////////////////////////////////////////

Image1BitModel::Image1BitModel(Shared* const sh) :
  shared(sh),
  mapL(sh, nLSM, 20, 128), //contexts, hash bits, input scale
  stateMap(sh, N, 256, 1023, StateMapType::BitHistory),
  matchStateMap(sh, nMatch, 1 << 7, 1023, StateMapType::Generic) {
}

void Image1BitModel::setParam(int widthInBytes) {
  if (w != static_cast<uint32_t>(widthInBytes)) { //the width changes: a new image begins
    w = widthInBytes;
    xByte = yRow = 0;
    runLength = runValue = 0;
    hPeriod = vPeriod = qualH = qualV = 0;
    for (uint32_t d = 0; d <= maxHPeriod; ++d) hAcc[d] = 0;
    for (uint32_t k = 0; k <= maxVPeriod; ++k) vAcc[k] = 0;
    for (int k = 0; k < nMatch; ++k) { matchLen[k] = 0; predictedBit[k] = -1; }
    refValid = false;
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Reading pixels
//////////////////////////////////////////////////////////////////////////////////////////////////

uint32_t Image1BitModel::at(const int dy, const int dx) const {
  const uint64_t row =
    dy == 0 ? r0 : dy == 1 ? r1 : dy == 2 ? r2 : dy == 3 ? r3 : dy == 4 ? r4 : dy == 5 ? r5 : dy == 6 ? r6 :
    dy == 7 ? r7 : dy == 8 ? r8 : dy == 9 ? r9 : dy == 10 ? r10 : dy == 11 ? r11 : dy == 12 ? r12 :
    dy == 13 ? r13 : dy == 14 ? r14 : dy == 15 ? r15 : r16;
  return static_cast<uint32_t>(dy == 0 ? row >> (-dx - 1) : row >> (8 - dx)) & 1;
}

uint32_t Image1BitModel::sparse(const int8_t(*tpl)[2], const int n) const {
  uint32_t v = 0;
  for (int k = 0; k < n; ++k) v = v << 1 | at(tpl[k][0], tpl[k][1]);
  return v;
}

template<typename B>
int Image1BitModel::px(const B& buf, int bpos, int dy, int dx) const {
  const int off = bpos + dx + 64; //offset by 64 so that >> and & work for dx down to -64
  const int dist = (int)(dy * w) - (off >> 3) + 8;
  if (dist < 1) return 0;
  return (buf(dist) >> (7 - (off & 7))) & 1;
}

//Three bytes are read: the one pixel q is in, and the two after it.
template<typename B>
uint32_t Image1BitModel::pixelsAt(const B& buf, const int64_t q, const int n, const uint32_t curByteIdx) const {
  const int64_t b0 = q >= 0 ? q >> 3 : -((-q + 7) >> 3); //floor(q / 8)
  uint32_t v = 0;
  for (int j = 0; j < 3; ++j) {
    const int64_t b = b0 + j;
    uint32_t byte = 0;
    if (b >= 0 && b < static_cast<int64_t>(curByteIdx)) byte = buf(static_cast<uint32_t>(static_cast<int64_t>(curByteIdx) - b));
    v = v << 8 | byte;
  }
  const int shift = 24 - static_cast<int>(q - b0 * 8) - n;
  return (v >> shift) & ((1u << n) - 1);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// At the start of each row: analyse the row just finished
//////////////////////////////////////////////////////////////////////////////////////////////////

//Index the row just finished by where it changes colour, so that the next colour change after any
//position is a single lookup (see trackRuns, which uses it).
//
//A row is a sequence of white and black runs, and what matters is where a new run starts. For
//every position x:
//
//    nc1[x] = where the row next turns black, at or after x
//    nc0[x] = where the row next turns white, at or after x
//
//    x:        0  1  2  3  4  5  6  7  8  9
//    row:      0  0  1  1  1  0  0  1  0  0
//    nc1[x]:   2  2  2  7  7  7  7  7  -  -
//    nc0[x]:   5  5  5  5  5  5  8  8  8  -        ( - = none left, stored as the row length )
//
//Both are built in one pass from right to left. The row is taken to start on white, so if its first
//pixel is black, that is a colour change too.
template<typename B>
void Image1BitModel::buildRefLine(const B& buf) {
  if (w == 0 || w > maxRowBytes) { refValid = false; return; }
  for (uint32_t j = 0; j < w; ++j) refBytes[j] = buf(w - j);
  const uint32_t pixels = w * 8;
  nc0[pixels] = nc1[pixels] = pixels; //no colour change from here on
  uint32_t next0 = pixels, next1 = pixels;
  uint32_t cur = refBytes[(pixels - 1) >> 3] & 1; //the last pixel of the row
  for (uint32_t x = pixels; x-- > 0;) {
    const uint32_t left = x > 0 ? ((refBytes[(x - 1) >> 3] >> (7 - ((x - 1) & 7))) & 1) : 0;
    if (cur != left) {
      if (cur != 0) next1 = x; else next0 = x;
    }
    nc0[x] = next0;
    nc1[x] = next1;
    cur = left;
  }
  refValid = true;
}

//Find the distances at which the image repeats itself, horizontally (along the row) and vertically
//(from row to row). These give the adaptive template pixels (see readAdaptiveTemplate): a dither
//or halftone screen repeats every few pixels, and lines of text repeat every few rows.
//
//The row just finished is compared with itself shifted by d pixels, and with the rows v rows above
//it. The score of a comparison counts only the ink, not the paper:
//
//     score = 1024 * (pixels set in both) / (pixels set in either)     (1024 = identical)
//
//Simply counting the pixels that agree would not work: a mostly white page agrees with itself at
//any distance. And the distance chosen is not the one with the best score - nearby pixels are
//always similar, so that would always be the smallest distance - but the one whose score stands out
//the most from its neighbours' (its prominence).
template<typename B>
void Image1BitModel::updateRepeatPeriods(const B& buf) {
  if (w < minRowBytes || w > maxRowBytes) { hPeriod = vPeriod = qualH = qualV = 0; return; }

  //unpack the row just finished: buf(w) is its first byte, buf(1) its last
  for (uint32_t j = 0; j < w; ++j) rowBytes[j] = buf(w - j);
  const uint32_t nWords = (w + 7) / 8;
  for (uint32_t k = 0; k < nWords; ++k) {
    uint64_t v = 0;
    for (uint32_t b = 0; b < 8; ++b) {
      const uint32_t j = k * 8 + b;
      v = v << 8 | (j < w ? rowBytes[j] : 0);
    }
    rowWords[k] = v; //bit 63 of word 0 is the leftmost pixel of the row
  }

  const auto prominence = [](const uint32_t* acc, const uint32_t k) {
    return 2 * static_cast<int>(acc[k]) - static_cast<int>(acc[k - 1]) - static_cast<int>(acc[k + 1]);
    };

  //horizontal: the row against itself shifted by d pixels. The first word is skipped, as the bits
  //shifted into it would come from outside the row.
  uint32_t scoreAtH[maxHPeriod + 2] = {};
  for (uint32_t d = 2; d <= maxHPeriod; ++d) {
    uint32_t inter = 0, uni = 0;
    for (uint32_t k = 1; k < nWords; ++k) {
      const uint64_t shifted = (rowWords[k] >> d) | (rowWords[k - 1] << (64 - d));
      inter += bitCount64(rowWords[k] & shifted);
      uni += bitCount64(rowWords[k] | shifted);
    }
    scoreAtH[d] = uni == 0 ? 0 : (inter << 10) / uni;
    hAcc[d] = hAcc[d] - (hAcc[d] >> 3) + scoreAtH[d]; //smoothed over about 8 rows, so the choice is stable
  }
  uint32_t bestD = 0;
  int bestPromH = minProminence;
  for (uint32_t d = 3; d < maxHPeriod; ++d) {
    const int prom = prominence(&hAcc[0], d);
    if (prom > bestPromH) { bestPromH = prom; bestD = d; }
  }
  hPeriod = bestD;
  //how well it repeats, from this row alone, so it follows changes in the image at once
  const uint32_t scoreH = bestD == 0 ? 0 : scoreAtH[bestD];
  qualH = bestD == 0 ? 0 : scoreH < 400 ? 0 : scoreH < 600 ? 1 : scoreH < 800 ? 2 : 3;

  //vertical: the row against the rows minVPeriod..maxVPeriod above it
  uint32_t scoreAtV[maxVPeriod + 2] = {};
  uint32_t lastK = 0;
  for (uint32_t k = minVPeriod; k <= maxVPeriod; ++k) {
    if (yRow < k + 1) break;
    uint32_t inter = 0, uni = 0;
    const uint32_t base = (k + 1) * w;
    for (uint32_t j = 0; j < w; ++j) {
      const uint32_t a = rowBytes[j], b = buf(base - j);
      inter += bitCount(a & b);
      uni += bitCount(a | b);
    }
    scoreAtV[k] = uni == 0 ? 0 : (inter << 10) / uni;
    vAcc[k] = vAcc[k] - (vAcc[k] >> 3) + scoreAtV[k];
    lastK = k;
  }
  uint32_t bestV = 0;
  int bestPromV = minProminence;
  for (uint32_t k = minVPeriod + 1; k + 1 <= lastK; ++k) {
    const int prom = prominence(&vAcc[0], k);
    if (prom > bestPromV) { bestPromV = prom; bestV = k; }
  }
  vPeriod = bestV;
  const uint32_t scoreV = bestV == 0 ? 0 : scoreAtV[bestV];
  qualV = bestV == 0 ? 0 : scoreV < 400 ? 0 : scoreV < 600 ? 1 : scoreV < 800 ? 2 : 3;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// Match model helpers
//////////////////////////////////////////////////////////////////////////////////////////////////

//The 40 pixels compared: the 8 to the left on the current row, and 16 on each of the two rows
//above (7 to the left of the column and 8 to the right). The candidate's pixels are read from the
//buffer in the same layout as r0..r2, so they can be compared bit by bit.
//  2  .xxxxxxxxxxxxxxxx
//  1  .xxxxxxxxxxxxxxxx
//  0  xxxxxxxx?
template<typename B>
uint32_t Image1BitModel::residual(const B& buf, const uint32_t byteIdx, const uint32_t bp, const uint32_t curByteIdx) const {
  const int64_t s = static_cast<int64_t>(byteIdx) * 8 + bp;
  const int64_t rowPixels = static_cast<int64_t>(w) * 8;
  const uint32_t s0 = pixelsAt(buf, s - 8, 8, curByteIdx);
  const uint32_t s1 = pixelsAt(buf, s - rowPixels - 7, 16, curByteIdx);
  const uint32_t s2 = pixelsAt(buf, s - 2 * rowPixels - 7, 16, curByteIdx);
  return static_cast<uint32_t>(
    bitCount((s0 ^ static_cast<uint32_t>(r0)) & 0xff) +
    bitCount((s1 ^ static_cast<uint32_t>(r1)) & 0xffff) +
    bitCount((s2 ^ static_cast<uint32_t>(r2)) & 0xffff));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// update(): learn from the pixel just coded
//////////////////////////////////////////////////////////////////////////////////////////////////

void Image1BitModel::update() {
  INJECT_SHARED_y

  //the indexed contexts: their bit histories and their counts
  for (int i = 0; i < N; ++i) {
    StateTable::update(&t[cxt[i]], y, rnd);

    uint32_t c = counts[cxt[i]];
    int n0 = (c >> 8) & 255;
    int n1 = c & 255;

    if (y == 0) n0++; else n1++;
    if (n0 + n1 >= 64) { //halve both, so the counts follow recent pixels
      n0 >>= 1;
      n1 >>= 1;
    }

    counts[cxt[i]] = n0 << 8 | n1;
  }

  //the match models: if the prediction was right, the match moves on to the next pixel, otherwise
  //it ends (the next pixel looks the neighbourhood up again)
  for (int k = 0; k < nMatch; ++k) {
    if (matchLen[k] > 0) {
      if (predictedBit[k] == y) {
        if (matchLen[k] < 65535) matchLen[k]++;
        if (++matchBpos[k] == 8) {
          matchBpos[k] = 0;
          matchByteIdx[k]++;
          //the source has reached the end of its row: what follows it is another row
          if (++matchSrcX[k] >= w) matchLen[k] = 0;
        }
      }
      else matchLen[k] = 0;
    }
    //remember where this key was seen: here
    MatchBucket& bucket = matchTable[(static_cast<size_t>(k) << bucketBits) + matchHashIdx[k]];
    for (int s = matchSlots - 1; s > 0; --s) {
      bucket.pos[s] = bucket.pos[s - 1];
      bucket.check[s] = bucket.check[s - 1];
    }
    bucket.pos[0] = lastBpos << 29 | (lastByteIdx & 0x1fffffff);
    bucket.check[0] = matchCheck[k];
  }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
// mix(): predict the next pixel
//////////////////////////////////////////////////////////////////////////////////////////////////

void Image1BitModel::mix(Mixer& m) {
  update();

  INJECT_SHARED_y
  INJECT_SHARED_buf
  INJECT_SHARED_bpos

  shiftRows(buf, y, bpos);

  //at the start of a row, the row above is complete: analyse it
  const bool rowStart = (bpos == 0 && xByte == 0);
  if (rowStart) {
    if (yRow > 0) { updateRepeatPeriods(buf); buildRefLine(buf); }
    else refValid = false;
  }

  //read the neighbourhood
  trackRuns(y, rowStart, bpos);
  readColumns();
  readDirections();
  readAdaptiveTemplate(buf, bpos);
  runMatchModels(buf, bpos, rowStart);
  readNeighbourhood(y);

  //and turn it into predictions
  setIndexedContexts(y, bpos);
  setHashedContexts(y, bpos);
  mapL.mix(m);
  addInputs(m);
  setMixerContexts(m, y, bpos);
  publishToSSE();

  //move on to the next pixel
  if (bpos == 7 && w != 0) {
    if (++xByte >= w) { xByte = 0; yRow++; }
  }
}

//Shift the pixel just coded into r0, and the pixel above the next one into each of the rows above
//(see the row registers in the header).
template<typename B>
void Image1BitModel::shiftRows(const B& buf, const int y, const int bpos) {
  r0 += r0 + y;
  r1 += r1 + ((buf(1 * w - 1) >> (7 - bpos)) & 1);
  r2 += r2 + ((buf(2 * w - 1) >> (7 - bpos)) & 1);
  r3 += r3 + ((buf(3 * w - 1) >> (7 - bpos)) & 1);
  r4 += r4 + ((buf(4 * w - 1) >> (7 - bpos)) & 1);
  r5 += r5 + ((buf(5 * w - 1) >> (7 - bpos)) & 1);
  r6 += r6 + ((buf(6 * w - 1) >> (7 - bpos)) & 1);
  r7 += r7 + ((buf(7 * w - 1) >> (7 - bpos)) & 1);
  r8 += r8 + ((buf(8 * w - 1) >> (7 - bpos)) & 1);
  r9 += r9 + ((buf(9 * w - 1) >> (7 - bpos)) & 1);
  r10 += r10 + ((buf(10 * w - 1) >> (7 - bpos)) & 1);
  r11 += r11 + ((buf(11 * w - 1) >> (7 - bpos)) & 1);
  r12 += r12 + ((buf(12 * w - 1) >> (7 - bpos)) & 1);
  r13 += r13 + ((buf(13 * w - 1) >> (7 - bpos)) & 1);
  r14 += r14 + ((buf(14 * w - 1) >> (7 - bpos)) & 1);
  r15 += r15 + ((buf(15 * w - 1) >> (7 - bpos)) & 1);
  r16 += r16 + ((buf(16 * w - 1) >> (7 - bpos)) & 1);

  //the same rows one byte further ahead; a row narrower than three bytes has no such byte
  if (w >= 3) {
    r1b += r1b + ((buf(1 * w - 2) >> (7 - bpos)) & 1);
    r2b += r2b + ((buf(2 * w - 2) >> (7 - bpos)) & 1);
    r3b += r3b + ((buf(3 * w - 2) >> (7 - bpos)) & 1);
    r4b += r4b + ((buf(4 * w - 2) >> (7 - bpos)) & 1);
    r5b += r5b + ((buf(5 * w - 2) >> (7 - bpos)) & 1);
    r6b += r6b + ((buf(6 * w - 2) >> (7 - bpos)) & 1);
    r7b += r7b + ((buf(7 * w - 2) >> (7 - bpos)) & 1);
    r8b += r8b + ((buf(8 * w - 2) >> (7 - bpos)) & 1);
  }
}

//The run of identical pixels that ends just left of the current pixel, and where the row above (the
//reference line) changes colour next.
//
//A bilevel image is made of runs, and a run usually ends where the run above it ended: the edge of
//a letter or a line continues from one row to the next. This is the idea behind the "vertical
//mode" of CCITT fax coding (T.4/T.6), and the names follow that standard:
//
//  a0        x                        <- the current run starts at a0; x is the pixel being coded
//  |---------?
//  ....|..........|.....              <- the row above: its next two colour changes, b1 and b2
//      b1         b2
//
//b1 is the first colour change on the row above, right of a0, to the colour the current run would
//change to; b2 is the change after it. When x reaches b1 the current run is likely to end. When x
//is past b2, the run above has already ended inside the current one.
void Image1BitModel::trackRuns(const int y, const bool rowStart, const int bpos) {
  //runs do not continue from one row to the next
  const uint32_t uy = static_cast<uint32_t>(y);
  if (rowStart) { runLength = 0; runValue = uy; prevRun1 = prevRun2 = 0; }
  else if (uy == runValue) { if (runLength < 65535) runLength++; }
  else { prevRun2 = prevRun1; prevRun1 = runLength; runValue = uy; runLength = 1; }
  f.runIdx = runBucket(runLength); //0..15

  f.curCol = runLength == 0 ? 0 : runValue; //the colour of the current run; a row starts on white
  int dB1 = INT32_MAX, dB2 = INT32_MAX;
  if (refValid) {
    const uint32_t pixels = w * 8;
    const uint32_t x = xByte * 8 + static_cast<uint32_t>(bpos);
    const uint32_t from = runLength == 0 ? 0 : x - runLength + 1; //just right of a0
    const uint32_t b1 = f.curCol == 0 ? nc1[from] : nc0[from];
    if (b1 < pixels) {
      dB1 = static_cast<int>(x) - static_cast<int>(b1);
      const uint32_t b2 = f.curCol == 0 ? nc0[b1 + 1] : nc1[b1 + 1];
      if (b2 < pixels) dB2 = static_cast<int>(x) - static_cast<int>(b2);
    }
  }
  f.vb1 = transBucket(dB1); //x - b1, 0..31
  f.vb2 = transBucket(dB2); //x - b2
}

//The columns above the current pixel, and how far the ink (or the paper) runs up each of them.
//
//The edge of a vertical stroke - the stem of a letter, a table rule, the side of a frame - is where
//the vertical runs next to each other end, just as the reference line does for horizontal runs.
//Seven columns side by side give the profile of a stroke across its width.
//
// 16  ..LcR..
// 15  ..LcR..
// 14  ..LcR..
// 13  ..LcR..
// 12  ..LcR..
// 11  ..LcR..
// 10  ..LcR..
//  9  ..LcR..
//  8  32LcR23
//  7  32LcR23
//  6  32LcR23
//  5  32LcR23
//  4  32LcR23
//  3  32LcR23
//  2  32LcR23
//  1  32LcR23
//  0  ...?
//  c: col8 (rows 1..8) and col16 (rows 9..16), bit 0 nearest
//  L, R: col8L and col8R, the columns either side (rows 9..16 are used for their runs only)
//  2, 3: the columns two and three away, used for their runs only
void Image1BitModel::readColumns() {
  const auto column8 = [&](const int shift) -> uint32_t {
    return static_cast<uint32_t>((r1 >> shift & 1) | (r2 >> shift & 1) << 1 | (r3 >> shift & 1) << 2 | (r4 >> shift & 1) << 3 |
      (r5 >> shift & 1) << 4 | (r6 >> shift & 1) << 5 | (r7 >> shift & 1) << 6 | (r8 >> shift & 1) << 7);
  };
  const auto column16 = [&](const int shift) -> uint32_t {
    return static_cast<uint32_t>((r9 >> shift & 1) | (r10 >> shift & 1) << 1 | (r11 >> shift & 1) << 2 | (r12 >> shift & 1) << 3 |
      (r13 >> shift & 1) << 4 | (r14 >> shift & 1) << 5 | (r15 >> shift & 1) << 6 | (r16 >> shift & 1) << 7);
  };
  f.col8 = column8(8);
  f.col16 = column16(8);
  f.vrun = static_cast<int>(runUp(f.col8, 0x100u)); //1..8
  f.colCount = bitCount(f.col8); //0..8: how much of the column is ink

  f.col8L = column8(9);
  f.col8R = column8(7);
  f.vrunL = runUp(f.col8L, 0x100u);                        //1..8
  f.vrunR = runUp(f.col8R, 0x100u);                        //1..8
  f.vrun16 = runUp(f.col8 | f.col16 << 8, 0x10000u);       //1..16
  f.vrun16L = runUp(f.col8L | column16(9) << 8, 0x10000u); //1..16
  f.vrun16R = runUp(f.col8R | column16(7) << 8, 0x10000u); //1..16
  f.vrunLL = runUp(column8(10), 0x100u);   //1..8
  f.vrunRR = runUp(column8(6), 0x100u);    //1..8
  f.vrunLLL = runUp(column8(11), 0x100u);  //1..8
  f.vrunRRR = runUp(column8(5), 0x100u);   //1..8
}

//The pixels above, read along slanted lines: the two diagonals and the slopes between them and the
//column. Each line is kept as 8 bits (bit 0 nearest), and as how far the colour of its nearest
//pixel runs along it.
//
//A rectangular template sees a slanted stroke - the leg of a 'v', a sloping line in a drawing, an
//italic letter - as a staircase, and has to learn every step of it separately. Read along its own
//direction, the stroke is just a run.
//
//The diagonals diagNW and diagNE (the digits are the order of the pixels, 1 = nearest):
//  8  8...............8
//  7  .7.............7.
//  6  ..6...........6..
//  5  ...5.........5...
//  4  ....4.......4....
//  3  .....3.....3.....
//  2  ......2...2......
//  1  .......1.1.......
//  0  ........?
//diagNW2 and diagNE2 are the same lines one column further out. diagNW16 continues diagNW from
//row 9 to row 16.
//
//The steep slopes steepNW and steepNE (one column per two rows):
//  8  8.......8
//  7  7.......7
//  6  .6.....6.
//  5  .5.....5.
//  4  ..4...4..
//  3  ..3...3..
//  2  ...2.2...
//  1  ...1.1...
//  0  ....?
//steepNW16 continues steepNW from row 9 to row 16.
//
//The flat slopes flatNW and flatNE (two columns per row):
//  8  8...............................8
//  7  ..7...........................7..
//  6  ....6.......................6....
//  5  ......5...................5......
//  4  ........4...............4........
//  3  ..........3...........3..........
//  2  ............2.......2............
//  1  ..............1...1..............
//  0  ................?
//flatNW16 continues flatNW from row 9 to row 16.
//
//The very steep slopes vsteepNW and vsteepNE (one column per three rows):
//  8  8.....8
//  7  7.....7
//  6  .6...6.
//  5  .5...5.
//  4  .4...4.
//  3  ..3.3..
//  2  ..2.2..
//  1  ..1.1..
//  0  ...?
//
//And the very flat slope vflatNW (three columns per row):
//  8  8........................
//  7  ...7.....................
//  6  ......6..................
//  5  .........5...............
//  4  ............4............
//  3  ...............3.........
//  2  ..................2......
//  1  .....................1...
//  0  ........................?
void Image1BitModel::readDirections() {
  f.diagNW = (r1 >> 9 & 1) | (r2 >> 10 & 1) << 1 | (r3 >> 11 & 1) << 2 | (r4 >> 12 & 1) << 3 |
    (r5 >> 13 & 1) << 4 | (r6 >> 14 & 1) << 5 | (r7 >> 15 & 1) << 6 | (r8 >> 16 & 1) << 7;
  f.diagNE = (r1 >> 7 & 1) | (r2 >> 6 & 1) << 1 | (r3 >> 5 & 1) << 2 | (r4 >> 4 & 1) << 3 |
    (r5 >> 3 & 1) << 4 | (r6 >> 2 & 1) << 5 | (r7 >> 1 & 1) << 6 | (r8 & 1) << 7;
  f.drunNW = runUp(f.diagNW, 0x100u); //1..8
  f.drunNE = runUp(f.diagNE, 0x100u); //1..8
  f.diagNW2 = (r1 >> 10 & 1) | (r2 >> 11 & 1) << 1 | (r3 >> 12 & 1) << 2 | (r4 >> 13 & 1) << 3 |
    (r5 >> 14 & 1) << 4 | (r6 >> 15 & 1) << 5 | (r7 >> 16 & 1) << 6 | (r8 >> 17 & 1) << 7;
  f.diagNE2 = (r1 >> 6 & 1) | (r2 >> 5 & 1) << 1 | (r3 >> 4 & 1) << 2 | (r4 >> 3 & 1) << 3 |
    (r5 >> 2 & 1) << 4 | (r6 >> 1 & 1) << 5 | (r7 & 1) << 6 | (r8b >> 7 & 1) << 7;
  f.diagNW16 = (r9 >> 17 & 1) | (r10 >> 18 & 1) << 1 | (r11 >> 19 & 1) << 2 | (r12 >> 20 & 1) << 3 |
    (r13 >> 21 & 1) << 4 | (r14 >> 22 & 1) << 5 | (r15 >> 23 & 1) << 6 | (r16 >> 24 & 1) << 7;
  f.drunNW16 = runUp(f.diagNW | f.diagNW16 << 8, 0x10000u); //1..16

  f.steepNW = (r1 >> 9 & 1) | (r2 >> 9 & 1) << 1 | (r3 >> 10 & 1) << 2 | (r4 >> 10 & 1) << 3 |
    (r5 >> 11 & 1) << 4 | (r6 >> 11 & 1) << 5 | (r7 >> 12 & 1) << 6 | (r8 >> 12 & 1) << 7;
  f.steepNE = (r1 >> 7 & 1) | (r2 >> 7 & 1) << 1 | (r3 >> 6 & 1) << 2 | (r4 >> 6 & 1) << 3 |
    (r5 >> 5 & 1) << 4 | (r6 >> 5 & 1) << 5 | (r7 >> 4 & 1) << 6 | (r8 >> 4 & 1) << 7;
  f.flatNW = (r1 >> 10 & 1) | (r2 >> 12 & 1) << 1 | (r3 >> 14 & 1) << 2 | (r4 >> 16 & 1) << 3 |
    (r5 >> 18 & 1) << 4 | (r6 >> 20 & 1) << 5 | (r7 >> 22 & 1) << 6 | (r8 >> 24 & 1) << 7;
  f.flatNE = (r1 >> 6 & 1) | (r2 >> 4 & 1) << 1 | (r3 >> 2 & 1) << 2 | (r4 & 1) << 3 |
    (r5b >> 6 & 1) << 4 | (r6b >> 4 & 1) << 5 | (r7b >> 2 & 1) << 6 | (r8b & 1) << 7;
  f.srunNW = runUp(f.steepNW, 0x100u);
  f.srunNE = runUp(f.steepNE, 0x100u);
  f.frunNW = runUp(f.flatNW, 0x100u);

  f.vsteepNW = (r1 >> 9 & 1) | (r2 >> 9 & 1) << 1 | (r3 >> 9 & 1) << 2 | (r4 >> 10 & 1) << 3 |
    (r5 >> 10 & 1) << 4 | (r6 >> 10 & 1) << 5 | (r7 >> 11 & 1) << 6 | (r8 >> 11 & 1) << 7;
  f.vsteepNE = (r1 >> 7 & 1) | (r2 >> 7 & 1) << 1 | (r3 >> 7 & 1) << 2 | (r4 >> 6 & 1) << 3 |
    (r5 >> 6 & 1) << 4 | (r6 >> 6 & 1) << 5 | (r7 >> 5 & 1) << 6 | (r8 >> 5 & 1) << 7;
  f.vflatNW = (r1 >> 11 & 1) | (r2 >> 14 & 1) << 1 | (r3 >> 17 & 1) << 2 | (r4 >> 20 & 1) << 3 |
    (r5 >> 23 & 1) << 4 | (r6 >> 26 & 1) << 5 | (r7 >> 29 & 1) << 6 | (r8 >> 32 & 1) << 7;
  f.vsrunNW = runUp(f.vsteepNW, 0x100u);
  f.vfrunNW = runUp(f.vflatNW, 0x100u);

  f.steepNW16 = (r9 >> 13 & 1) | (r10 >> 13 & 1) << 1 | (r11 >> 14 & 1) << 2 | (r12 >> 14 & 1) << 3 |
    (r13 >> 15 & 1) << 4 | (r14 >> 15 & 1) << 5 | (r15 >> 16 & 1) << 6 | (r16 >> 16 & 1) << 7;
  f.flatNW16 = (r9 >> 26 & 1) | (r10 >> 28 & 1) << 1 | (r11 >> 30 & 1) << 2 | (r12 >> 32 & 1) << 3 |
    (r13 >> 34 & 1) << 4 | (r14 >> 36 & 1) << 5 | (r15 >> 38 & 1) << 6 | (r16 >> 40 & 1) << 7;
  f.srunNW16 = runUp(f.steepNW | f.steepNW16 << 8, 0x10000u); //1..16
  f.frunNW16 = runUp(f.flatNW | f.flatNW16 << 8, 0x10000u);   //1..16
}

//The adaptive template pixels: the pixels one repeat period away (see updateRepeatPeriods), d pixels
//along the row and v rows up.
//
//        V1 V0 V2         <- v rows up: columns -1, 0, +1
//           V3            <- v+1 rows up
//              ...
//  H1.....x.....H2        <- the row above: columns -d and +d
//  H0.....?               <- the current row: column -d
template<typename B>
void Image1BitModel::readAdaptiveTemplate(const B& buf, const int bpos) {
  f.atH0 = f.atH1 = f.atH2 = f.atV0 = f.atV1 = f.atV2 = f.atV3 = 0;
  if (hPeriod >= 2) {
    f.atH0 = static_cast<uint32_t>(r0 >> (hPeriod - 1)) & 1;
    f.atH1 = static_cast<uint32_t>(r1 >> (8 + hPeriod)) & 1;
    f.atH2 = static_cast<uint32_t>(px(buf, bpos, 1, static_cast<int>(hPeriod)));
  }
  if (vPeriod >= minVPeriod && yRow > vPeriod) {
    f.atV0 = static_cast<uint32_t>(px(buf, bpos, static_cast<int>(vPeriod), 0));
    f.atV1 = static_cast<uint32_t>(px(buf, bpos, static_cast<int>(vPeriod), -1));
    f.atV2 = static_cast<uint32_t>(px(buf, bpos, static_cast<int>(vPeriod), 1));
    f.atV3 = static_cast<uint32_t>(px(buf, bpos, static_cast<int>(vPeriod) + 1, 0));
  }
}

//The match models: where was this neighbourhood seen before, and what came next there?
//
//Each model hashes the neighbourhood into a key of its own shape, and looks the key up in its
//table, which remembers the last three places the key was seen. Of those, the one whose 40
//surrounding pixels look the most like the current ones is taken (see residual), unless even that
//one differs in too many. From then on the match predicts that the pixel after the source is the
//same as the pixel after the current one, until it is wrong, or until either of them reaches the
//end of its row.
//
//Four keys, because they fail at different moments, and because several of them agreeing is worth
//more than any one of them.
template<typename B>
void Image1BitModel::runMatchModels(const B& buf, const int bpos, const bool rowStart) {
  const uint32_t curByteIdx = yRow * w + xByte;

  //A: a diamond, wide and shallow. The rows above, read wide, say more about which letter this is
  //than a narrow block reaching the same number of pixels further up.
  //  4  ....xxxxxxxxxxxx....
  //  3  ...xxxxxxxxxxxxxx...
  //  2  ..xxxxxxxxxxxxxxxx..
  //  1  xxxxxxxxxxxxxxxxxxxx
  //  0  .xxxxxxxxxxxx?
  const uint64_t keyA = hash(r0 & 0xfff, (r1 >> 2) & 0xfffff, (r2 >> 4) & 0xffff, (r3 >> 5) & 0x3fff, (r4 >> 6) & 0xfff);

  //B: a column narrowing over twelve rows: the same letter, wherever it is on the line
  // 12  .....xxx..
  // 11  .....xxx..
  // 10  .....xxx..
  //  9  .....xxx..
  //  8  ....xxxxx.
  //  7  ....xxxxx.
  //  6  ....xxxxx.
  //  5  ....xxxxx.
  //  4  ...xxxxxxx
  //  3  ...xxxxxxx
  //  2  ...xxxxxxx
  //  1  ...xxxxxxx
  //  0  xxxxxx?
  const uint64_t keyB = hash(r0 & 0x3f,
    (r1 >> 5 & 0x7f) | (r2 >> 5 & 0x7f) << 7 | (r3 >> 5 & 0x7full) << 14 | (r4 >> 5 & 0x7full) << 21,
    (r5 >> 6 & 0x1f) | (r6 >> 6 & 0x1f) << 5 | (r7 >> 6 & 0x1full) << 10 | (r8 >> 6 & 0x1full) << 15,
    (r9 >> 7 & 7) | (r10 >> 7 & 7) << 3 | (r11 >> 7 & 7ull) << 6 | (r12 >> 7 & 7ull) << 9);

  //C and D: the same letter, blurred. Two prints of the same letter differ in a pixel here and
  //there, and an exact key would treat them as different. These keys merge the rows above in pairs,
  //so they only ask for the same shape: C keeps a pixel where both rows of the pair have ink (AND),
  //D where either has (OR). One forgives a missing pixel, the other an extra one.
  //In rows 1..8, 'a' and 'b' are the two rows of a pair:
  //  8  ..bbbbbbbbbbbb
  //  7  ..aaaaaaaaaaaa
  //  6  ..bbbbbbbbbbbb
  //  5  ..aaaaaaaaaaaa
  //  4  ..bbbbbbbbbbbb
  //  3  ..aaaaaaaaaaaa
  //  2  ..bbbbbbbbbbbb
  //  1  ..aaaaaaaaaaaa
  //  0  xxxxxxxx?
  const uint64_t a12 = r1 & r2, a34 = r3 & r4, a56 = r5 & r6, a78 = r7 & r8;
  const uint64_t keyC = hash(r0 & 0xff,
    (a12 >> 3 & 0xfff) | (a34 >> 3 & 0xfffull) << 12,
    (a56 >> 3 & 0xfff) | (a78 >> 3 & 0xfffull) << 12);
  const uint64_t keyD = hash(r0 & 0xff,
    ((r1 | r2) >> 3 & 0xfff) | ((r3 | r4) >> 3 & 0xfffull) << 12,
    ((r5 | r6) >> 3 & 0xfff) | ((r7 | r8) >> 3 & 0xfffull) << 12);

  //the high bits of a key select its bucket, the byte below them is its check byte
  const uint64_t keys[nMatch] = { keyA, keyB, keyC, keyD };
  for (int k = 0; k < nMatch; ++k) {
    matchHashIdx[k] = finalize64(keys[k], bucketBits);
    matchCheck[k] = static_cast<uint8_t>(keys[k] >> (56 - bucketBits));
  }
  //a match does not continue into the next row
  if (rowStart) {
    for (int k = 0; k < nMatch; ++k) matchLen[k] = 0;
  }
  for (int k = 0; k < nMatch; ++k) {
    if (matchLen[k] == 0 && w != 0) {
      //of the places this key was seen, take the one that looks the most like here
      const MatchBucket& bucket = matchTable[(static_cast<size_t>(k) << bucketBits) + matchHashIdx[k]];
      uint32_t bestRes = maxResidual + 1;
      for (int s = 0; s < matchSlots; ++s) {
        const uint32_t e = bucket.pos[s];
        if (e == 0) break;
        if (bucket.check[s] != matchCheck[k]) continue;
        const uint32_t cand = e & 0x1fffffff;
        const uint32_t candBpos = e >> 29;
        if (cand >= curByteIdx) continue;
        const uint32_t res = residual(buf, cand, candBpos, curByteIdx);
        if (res < bestRes) { bestRes = res; matchByteIdx[k] = cand; matchBpos[k] = candBpos; }
      }
      if (bestRes <= maxResidual) {
        matchLen[k] = 1;
        matchSrcX[k] = matchByteIdx[k] % w;
        matchResid[k] = residBucket(bestRes);
      }
    }
    predictedBit[k] = -1;
    if (matchLen[k] > 0) {
      const uint32_t dist = curByteIdx - matchByteIdx[k];
      if (dist >= 1) predictedBit[k] = (buf(dist) >> (7 - matchBpos[k])) & 1;
      else matchLen[k] = 0;
    }
  }
  lastByteIdx = curByteIdx;
  lastBpos = static_cast<uint32_t>(bpos);

  //what they say together: for each, 0 = no match, 1 = expects paper, 2 = expects ink
  f.lenBucketA = matchBucket(matchLen[0]);
  f.lenBucketB = matchBucket(matchLen[1]);
  f.matchVote = (predictedBit[0] + 1) * 3 + (predictedBit[1] + 1); //A and B: 0..8
  f.matchVote4 = (f.matchVote * 3 + (predictedBit[2] + 1)) * 3 + (predictedBit[3] + 1); //all four: 0..80

  //The best of the active matches is the longest one, and of equally long ones the one that
  //resembled the current neighbourhood the most. The matches disagree when any two of them expect
  //different pixels.
  bool expects0 = false, expects1 = false;
  int bestMatch = -1;
  for (int k = 0; k < nMatch; ++k) {
    if (predictedBit[k] < 0) continue;
    if (predictedBit[k] == 0) expects0 = true; else expects1 = true;
    if (bestMatch < 0 || matchLen[k] > matchLen[bestMatch] ||
      (matchLen[k] == matchLen[bestMatch] && matchResid[k] < matchResid[bestMatch]))
      bestMatch = k;
  }
  const uint32_t matchUncertain = static_cast<uint32_t>(expects0 && expects1);
  //the verdict, for the mixer and the SSE stage: 0 = no match, otherwise 1 + these bits:
  //  bits 3-4: the length of the best match (1-3, 4-15, 16-63, 64 or more)
  //  bit 2:    the pixel it expects
  //  bit 1:    the matches disagree
  //  bit 0:    the best match's source was identical to the current neighbourhood
  f.matchMode = 0;
  if (bestMatch >= 0) {
    const uint32_t bl = matchLen[bestMatch];
    const uint32_t lenQ = bl < 4 ? 0 : bl < 16 ? 1 : bl < 64 ? 2 : 3;
    f.matchMode = 1 + (lenQ << 3 | static_cast<uint32_t>(predictedBit[bestMatch]) << 2 | matchUncertain << 1 |
      static_cast<uint32_t>(matchResid[bestMatch] == 0));
  }
}

//The pixels right around the current one, used by several contexts, the direct predictors, the
//mixer and the SSE stage.
void Image1BitModel::readNeighbourhood(const int y) {
  //surrounding4
  //  1  xxx
  //  0  x?
  f.surrounding4 = y | (r1 >> 7 & 7) << 1;

  //mCtx6
  //  3  ...x
  //  2  ...x
  //  1  ...x
  //  0  xxx?
  f.mCtx6 = ((r0 & 7) | (r1 >> 8 & 1) << 3 | (r2 >> 8 & 1) << 4 | (r3 >> 8 & 1) << 5);

  //surrounding12
  //  2  xxxxx
  //  1  xxxxx
  //  0  xx?
  f.surrounding12 = (r0 & 3) | (r1 >> 6 & 0x1f) << 2 | (r2 >> 6 & 0x1f) << 7;

  //surrounding24
  //  3  xxxxxxx
  //  2  xxxxxxx
  //  1  xxxxxxx
  //  0  xxx?
  f.surrounding24 = (r0 & 7) | (r1 >> 5 & 0x7f) << 3 | (r2 >> 5 & 0x7f) << 10 | (r3 >> 5 & 0x7f) << 17;

  //frame16: a frame around surrounding24; the two together cover 40 pixels
  //  4  xxxxxxxxx
  //  3  x.......x
  //  2  x.......x
  //  1  x.......x
  //  0  x...?
  f.frame16 = (r0 >> 3 & 1) | (r1 >> 12 & 1) << 1 | (r2 >> 12 & 1) << 2 | (r3 >> 12 & 1) << 3 | (r4 >> 4 & 0x1ff) << 4 | (r3 >> 4 & 1) << 13 | (r2 >> 4 & 1) << 14 | (r1 >> 4 & 1) << 15;

  //how many of the 4, 12, 24 and 40 pixels around are ink: the grey level, for dithered images
  f.bitcount04 = bitCount(f.surrounding4);
  f.bitcount12 = bitCount(f.surrounding12);
  f.bitcount24 = bitCount(f.surrounding24);
  f.bitcount40 = bitCount(f.frame16) + f.bitcount24;

  //the shape of the ink just above, blurred: the two rows above are merged, and each pixel with
  //the one to its left, so each of the 6 bits covers 2x2 pixels. dil6 is set where any of the 4 is
  //ink (the ink dilated), ero6 where all 4 are (the ink eroded). The 6 cells cover:
  //  2  xxxxxxx
  //  1  xxxxxxx
  //  0  ....?
  const uint64_t mo12 = (r1 | r2), ma12 = (r1 & r2);
  f.dil6 = static_cast<uint32_t>((mo12 | mo12 >> 1) >> 6) & 0x3f;
  f.ero6 = static_cast<uint32_t>((ma12 & ma12 >> 1) >> 6) & 0x3f;
}

//The indexed contexts: small enough to address their slots directly. Each context uses its own part
//of the table t (see C in the header).
void Image1BitModel::setIndexedContexts(const int y, const int bpos) {
  int c = 0; //where the current context's slots start
  size_t i = 0; //context number

  //////////////////////////////////////////////////////////////////////////////
  // Small templates: the pixels right around the current one, from all sides
  //////////////////////////////////////////////////////////////////////////////

  //  1  .x
  //  0  x?
  cxt[i++] = c + (y | (r1 >> 8 & 1) << 1);
  c += 1 << 2;

  //surrounding4
  //  1  xxx
  //  0  x?
  cxt[i++] = c + f.surrounding4;
  c += 1 << 4;

  //mCtx6
  //  3  ...x
  //  2  ...x
  //  1  ...x
  //  0  xxx?
  cxt[i++] = c + f.mCtx6;
  c += 1 << 6;

  //  2  .xx..
  //  1  ..xxx
  //  0  xxx?
  cxt[i++] = c + ((r0 & 7) | (r1 >> 7 & 7) << 3 | (r2 >> 9 & 3) << 6);
  c += 1 << 8;

  //  3  .x...
  //  2  .x...
  //  1  xxxxx
  //  0  x?
  cxt[i++] = c + (y | (r1 >> 5 & 0x1f) << 1 | (r2 >> 8 & 1) << 6 | (r3 >> 8 & 1) << 7);
  c += 1 << 8;

  //  3  .xx.
  //  2  .xx.
  //  1  .xxx
  //  0  x?
  cxt[i++] = c + (y | (r1 >> 6 & 7) << 1 | (r2 >> 7 & 3) << 4 | (r3 >> 7 & 3) << 6);
  c += 1 << 8;

  //the column above, 8 rows
  //  8  x
  //  7  x
  //  6  x
  //  5  x
  //  4  x
  //  3  x
  //  2  x
  //  1  x
  //  0  ?
  cxt[i++] = c + ((r1 >> 8 & 1) << 7 | (r2 >> 8 & 1) << 6 | (r3 >> 8 & 1) << 5 | (r4 >> 8 & 1) << 4 | (r5 >> 8 & 1) << 3 | (r6 >> 8 & 1) << 2 | (r7 >> 8 & 1) << 1 | (r8 >> 8 & 1));
  c += 1 << 8;

  //  4  xx
  //  3  xx
  //  2  xx
  //  1  xx
  //  0  x?
  cxt[i++] = c + (y | (r1 >> 8 & 3) << 1 | (r2 >> 8 & 3) << 3 | (r3 >> 8 & 3) << 5 | (r4 >> 8 & 3) << 7);
  c += 1 << 9;

  //  1  xxxxxx
  //  0  .xxxx?
  cxt[i++] = c + ((r0 & 0x0f) | (r1 >> 8 & 0x3f) << 4);
  c += 1 << 10;

  //  2  .....xx.
  //  1  .xxxxxxx
  //  0  xxx?
  cxt[i++] = c + ((r0 & 7) | (r1 >> 4 & 0x7f) << 3 | ((r2 >> 5) & 3) << 10);
  c += 1 << 12;

  //surrounding12
  //  2  xxxxx
  //  1  xxxxx
  //  0  xx?
  cxt[i++] = c + f.surrounding12;
  c += 1 << 12;

  //how many of the 4, the 12 and the 40 pixels around are ink
  cxt[i++] = c + f.bitcount04;
  c += 5;

  cxt[i++] = c + f.bitcount12;
  c += 13;

  cxt[i++] = c + f.bitcount40;
  c += 41;

  //the dither phase (x mod 4, y mod 4), and surrounding4. An ordered dither repeats every few
  //pixels, so where we are within its cell tells a lot about the next pixel.
  cxt[i++] = c + ((bpos & 3) | (yRow & 3) << 2 | f.surrounding4 << 4);
  c += 1 << 8;

  //  1  .xxxxx
  //  0  xxxxx?
  cxt[i++] = c + ((r0 & 0x1f) | (r1 >> 8 & 0x1f) << 5);
  c += 1 << 10;

  //  4  xxx
  //  3  xxx
  //  2  xxx
  //  1  xxx
  //  0  x?
  cxt[i++] = c + (y | (r1 >> 7 & 7) << 1 | (r2 >> 7 & 7) << 4 | (r3 >> 7 & 7) << 7 | (r4 >> 7 & 7) << 10);
  c += 1 << 13;

  //the horizontal adaptive template (see readAdaptiveTemplate): W, N, and the pixels one period
  //along the row away, with how well the image repeats at that period
  cxt[i++] = c + (y | (r1 >> 8 & 1) << 1 | f.atH0 << 2 | f.atH1 << 3 | f.atH2 << 4 | qualH << 5);
  c += 1 << 7;

  //the vertical adaptive template: W, N, and the pixels one period up, with how well the image
  //repeats at that period
  cxt[i++] = c + (y | (r1 >> 8 & 1) << 1 | f.atV0 << 2 | f.atV1 << 3 | f.atV2 << 4 | f.atV3 << 5 | qualV << 6);
  c += 1 << 8;

  //surrounding4, and the pixel two rows up
  //  2  .x.
  //  1  xxx
  //  0  x?
  cxt[i++] = c + (f.surrounding4 | (r2 >> 8 & 1) << 4);
  c += 32;

  //////////////////////////////////////////////////////////////////////////////
  // Wide, shallow templates, and the column.
  //
  // Beyond the nearest pixels, the templates below read one or two rows as wide
  // as possible, at every depth down to 13 rows up, instead of compact blocks.
  // A row of a letter, of a line or of a dither pattern is a horizontal feature,
  // so a wide reading of a row says more about the current pixel than a block of
  // the same number of pixels. Each depth is a separate look at the same letter,
  // and the mixer learns how far up it is still worth looking.
  //////////////////////////////////////////////////////////////////////////////

  //the column above, 16 rows: vertical lines, frame borders, and the spacing of lines of text
  // 16  x
  // 15  x
  // 14  x
  // 13  x
  // 12  x
  // 11  x
  // 10  x
  //  9  x
  //  8  x
  //  7  x
  //  6  x
  //  5  x
  //  4  x
  //  3  x
  //  2  x
  //  1  x
  //  0  ?
  cxt[i++] = c + (f.col8 | f.col16 << 8);
  c += 1 << 16;

  //the sparse template T15
  //  6  ........x....
  //  5  .............
  //  4  .............
  //  3  .......xxx...
  //  2  ......x.x...x
  //  1  ...x...xxxx..
  //  0  x...x..x?
  cxt[i++] = c + sparse(T15, 15);
  c += 1 << 16;

  //the dither phase over 16 rows (x mod 8, y mod 16), and surrounding4
  cxt[i++] = c + (bpos | (yRow & 15) << 3 | f.surrounding4 << 7);
  c += 1 << 11;

  //  1  xxxxxxxxxxxxx
  //  0  .....x?
  cxt[i++] = c + (y | (r1 >> 2 & 0x1fff) << 1);
  c += 1 << 14;

  //  2  xxxxxxxxxxxxx
  //  1  .............
  //  0  .....x?
  cxt[i++] = c + (y | (r2 >> 2 & 0x1fff) << 1);
  c += 1 << 14;

  //  1  xxxxxxxxxxxxxxx
  //  0  .......?
  cxt[i++] = c + (r1 >> 1 & 0x7fff);
  c += 1 << 15;

  //  4  xxxxxxx
  //  3  xxxxxxx
  //  2  .......
  //  1  .......
  //  0  ..x?
  cxt[i++] = c + (y | (r3 >> 5 & 0x7f) << 1 | (r4 >> 5 & 0x7f) << 8);
  c += 1 << 15;

  //  2  xxxxxxxxxxxxxxx
  //  1  ...............
  //  0  .......?
  cxt[i++] = c + (r2 >> 1 & 0x7fff);
  c += 1 << 15;

  //  6  xxxxxxx
  //  5  xxxxxxx
  //   (rows 4..1 up: none)
  //  0  ..x?
  cxt[i++] = c + (y | (r5 >> 5 & 0x7f) << 1 | (r6 >> 5 & 0x7f) << 8);
  c += 1 << 15;

  //  4  xxxxxxxxxxxxx
  //  3  .............
  //  2  .............
  //  1  .............
  //  0  .....x?
  cxt[i++] = c + (y | (r4 >> 2 & 0x1fff) << 1);
  c += 1 << 14;

  //  3  xxxxxxxxxxxxx
  //  2  .............
  //  1  .............
  //  0  .....x?
  cxt[i++] = c + (y | (r3 >> 2 & 0x1fff) << 1);
  c += 1 << 14;

  // 10  xxxxxxx
  //  9  xxxxxxx
  //   (rows 8..1 up: none)
  //  0  ..x?
  cxt[i++] = c + (y | (r9 >> 5 & 0x7f) << 1 | (r10 >> 5 & 0x7f) << 8);
  c += 1 << 15;

  // 13  xxxxx
  // 12  xxxxx
  // 11  xxxxx
  //   (rows 10..1 up: none)
  //  0  .x?
  cxt[i++] = c + (y | (r11 >> 6 & 0x1f) << 1 | (r12 >> 6 & 0x1f) << 6 | (r13 >> 6 & 0x1f) << 11);
  c += 1 << 16;

  //  2  ..xxxxxx..
  //  1  xxxxxxxxxx
  //  0  ....?
  cxt[i++] = c + ((r1 >> 3 & 0x3ff) | (r2 >> 5 & 0x3f) << 10);
  c += 1 << 16;

  //  2  xxxxxxxxxxxxxxxxx
  //  1  .................
  //  0  ........?
  cxt[i++] = c + (r2 & 0x1ffff);
  c += 1 << 17;

  //////////////////////////////////////////////////////////////////////////////
  // Runs and directions
  //////////////////////////////////////////////////////////////////////////////

  //how far the ink runs up the column and the two either side, over 16 rows (the columns 'L',
  //'c' and 'R' of readColumns): how a vertical stroke is ending across its width. And N and W.
  cxt[i++] = c + ((f.vrun16 - 1) | (f.vrun16L - 1) << 4 | (f.vrun16R - 1) << 8 | (f.col8 & 1) << 12 | y << 13);
  c += 1 << 14;

  //how far the ink runs up the five columns -2..+2, over 8 rows: the profile of a stroke
  cxt[i++] = c + ((f.vrunLL - 1) | (f.vrunL - 1) << 3 | (f.vrun - 1) << 6 | (f.vrunR - 1) << 9 | (f.vrunRR - 1) << 12);
  c += 1 << 15;

  //how far the ink runs along the two steep slopes and the flat one to the left (see
  //readDirections), the nearest pixel of each steep slope, and W
  cxt[i++] = c + ((f.srunNW - 1) | (f.srunNE - 1) << 3 | (f.frunNW - 1) << 6 | (f.steepNW & 1) << 9 | (f.steepNE & 1) << 10 | y << 11);
  c += 1 << 12;

  //the two diagonals (see readDirections)
  //  8  x...............x
  //  7  .x.............x.
  //  6  ..x...........x..
  //  5  ...x.........x...
  //  4  ....x.......x....
  //  3  .....x.....x.....
  //  2  ......x...x......
  //  1  .......x.x.......
  //  0  ........?
  cxt[i++] = c + (f.diagNW | f.diagNE << 8);
  c += 1 << 16;

  //the blurred shape of the ink just above (dil6 and ero6, see readNeighbourhood), W and N
  cxt[i++] = c + (f.dil6 | f.ero6 << 6 | y << 12 | (r1 >> 8 & 1) << 13);
  c += 1 << 14;

  //the next two colour changes on the row above (b1 and b2, see trackRuns), the colour of the
  //current run, and N
  cxt[i++] = c + (f.vb1 | f.vb2 << 5 | f.curCol << 10 | (r1 >> 8 & 1) << 11);
  c += 1 << 12;

  assert(i == N);
  assert(c == C);
}

//The hashed contexts: templates too wide, too deep or too blurred to index directly.
void Image1BitModel::setHashedContexts(const int y, const int bpos) {
  //most indexed contexts are read here a second time (see noMirror in the header)
  size_t i;
  for (i = 0; i < N; ++i) {
    if ((noMirror >> i & 1) == 0) mapL.set(hash(i, cxt[i]));
  }

  //////////////////////////////////////////////////////////////////////////////
  // Blocks around the current pixel
  //////////////////////////////////////////////////////////////////////////////

  //  3  ....xxxxx.....
  //  2  ...xxxxxxx....
  //  1  .xxxxxxxxxxxxx
  //  0  xxxxxx?
  mapL.set(hash(i++, (r0 & 0x3f), (r1 >> 1 & 0x1fff), (r2 >> 5 & 0x7f), (r3 >> 6 & 0x1f)));

  //  3  ........xx
  //  2  ........xx
  //  1  ..xxxxxxxx
  //  0  xxxxxxxx?
  mapL.set(hash(i++, (r0 & 0xff), (r1 >> 7 & 0xff), (r2 >> 7 & 3) << 2 | (r3 >> 7 & 3)));

  //  8  .xx
  //  7  .xx
  //  6  .xx
  //  5  .xx
  //  4  .xx
  //  3  .xx
  //  2  .xx
  //  1  .xx
  //  0  x?
  mapL.set(hash(i++, y | (r1 >> 7 & 3) << 1, (r2 >> 7 & 3) | (r3 >> 7 & 3) << 2, (r4 >> 7 & 3) | (r5 >> 7 & 3) << 2, (r6 >> 7 & 3) | (r7 >> 7 & 3) << 2, (r8 >> 7 & 3)));

  //surrounding24
  //  3  xxxxxxx
  //  2  xxxxxxx
  //  1  xxxxxxx
  //  0  xxx?
  mapL.set(hash(i++, f.surrounding24));

  //surrounding24 and frame16
  //  4  xxxxxxxxx
  //  3  xxxxxxxxx
  //  2  xxxxxxxxx
  //  1  xxxxxxxxx
  //  0  xxxx?
  mapL.set(hash(i++, f.surrounding24, f.frame16));

  //  1  .........xxxxxxxxxxxxxxxx
  //  0  xxxxxxxxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0xffff, r1 & 0xffff));

  //  8  xxx
  //  7  xxx
  //  6  xxx
  //  5  xxx
  //  4  xxx
  //  3  xxx
  //  2  xxx
  //  1  xxx
  //  0  x?
  mapL.set(hash(i++, y | (r1 >> 7 & 7) << 1,
    (r2 >> 7 & 7) | (r3 >> 7 & 7) << 3 | (r4 >> 7 & 7) << 6,
    (r5 >> 7 & 7) | (r6 >> 7 & 7) << 3 | (r7 >> 7 & 7) << 6 | (r8 >> 7 & 7) << 9));

  //surrounding12, and the dither phase (x mod 8, y mod 8)
  mapL.set(hash(i++, f.surrounding12, bpos | (yRow & 7) << 3));

  //  4  .........................xxxxxxxx
  //  3  .................xxxxxxxxxxxxxxxx
  //  2  .................xxxxxxxxxxxxxxxx
  //  1  .........xxxxxxxxxxxxxxxxxxxxxxxx
  //  0  xxxxxxxxxxxxxxxxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0xffffff, r1 & 0xffffff, r2 & 0xffff, r3 & 0xffff, r4 & 0xff));

  //a diamond: narrower further away, where each pixel says less
  //  4  ..........xxxxx....
  //  3  .........xxxxxxx...
  //  2  ........xxxxxxxxx..
  //  1  ........xxxxxxxxxxx
  //  0  xxxxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0xfff, (r1 >> 2) & 0x7ff, (r2 >> 4) & 0x1ff, (r3 >> 5) & 0x7f, (r4 >> 6) & 0x1f));

  //the rows above, and of the current row only W: what is left at the start of a row, and what
  //carries a vertical stroke across a gap in the current row
  //  5  xxxxx
  //  4  xxxxx
  //  3  xxxxx
  //  2  xxxxx
  //  1  xxxxx
  //  0  .x?
  mapL.set(hash(i++, y | (r1 >> 6 & 0x1f) << 1, (r2 >> 6 & 0x1f) | (r3 >> 6 & 0x1f) << 5, (r4 >> 6 & 0x1f) | (r5 >> 6 & 0x1f) << 5));

  //the column above (col8), and the nearest pixels
  //  8  ......x..
  //  7  ......x..
  //  6  ......x..
  //  5  ......x..
  //  4  ......x..
  //  3  ......x..
  //  2  .....xxx.
  //  1  ....xxxxx
  //  0  xxxxxx?
  mapL.set(hash(i++, f.col8, r0 & 0x3f, (r1 >> 6 & 0x1f) | (r2 >> 7 & 7) << 5));

  //  8  xxxx
  //  7  xxxx
  //  6  xxxx
  //  5  xxxx
  //  4  xxxx
  //  3  xxxx
  //  2  xxxx
  //  1  xxxx
  //  0  .x?
  mapL.set(hash(i++, y | (r1 >> 7 & 0xf) << 1,
    (r2 >> 7 & 0xf) | (r3 >> 7 & 0xf) << 4 | (r4 >> 7 & 0xf) << 8,
    (r5 >> 7 & 0xf) | (r6 >> 7 & 0xf) << 4 | (r7 >> 7 & 0xf) << 8 | (r8 >> 7 & 0xf) << 12));

  //  3  ......xxxxxxxxx....
  //  2  ....xxxxxxxxxxxxx..
  //  1  ..xxxxxxxxxxxxxxxxx
  //  0  xxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0x3ff, r1 & 0x1ffff, (r2 >> 2) & 0x1fff, (r3 >> 4) & 0x1ff));

  //  4  xxxxxxx
  //  3  xxxxxxx
  //  2  xxxxxxx
  //  1  xxxxxxx
  //  0  ..x?
  mapL.set(hash(i++, y | (r1 >> 5 & 0x7f) << 1, (r2 >> 5 & 0x7f) | (r3 >> 5 & 0x7f) << 7, (r4 >> 5 & 0x7f)));

  //how much ink is around, over 12, 24 and 40 pixels, with surrounding4 and the ink in the column above
  mapL.set(hash(i++, static_cast<uint32_t>(f.bitcount40) | static_cast<uint32_t>(f.bitcount24) << 6 |
    static_cast<uint32_t>(f.bitcount12) << 11, f.surrounding4, static_cast<uint32_t>(f.colCount)));

  //surrounding24, and the dither phase (x mod 8, y mod 8)
  mapL.set(hash(i++, f.surrounding24, bpos | (yRow & 7) << 3));

  //////////////////////////////////////////////////////////////////////////////
  // Deep and wide: whole rows, and rows up to 16 above
  //////////////////////////////////////////////////////////////////////////////

  //the column above, 16 rows, with W and WW
  // 16  ..x
  // 15  ..x
  // 14  ..x
  // 13  ..x
  // 12  ..x
  // 11  ..x
  // 10  ..x
  //  9  ..x
  //  8  ..x
  //  7  ..x
  //  6  ..x
  //  5  ..x
  //  4  ..x
  //  3  ..x
  //  2  ..x
  //  1  ..x
  //  0  xx?
  mapL.set(hash(i++, y | (r0 & 3) << 1, f.col8 | f.col16 << 8));

  // 16  xxx
  // 15  xxx
  // 14  xxx
  // 13  xxx
  // 12  xxx
  // 11  xxx
  // 10  xxx
  //  9  xxx
  //   (rows 8..3 up: none)
  //  2  xxx
  //  1  xxx
  //  0  x?
  mapL.set(hash(i++, y | (r1 >> 7 & 7) << 1 | (r2 >> 7 & 7) << 4,
    (r9 >> 7 & 7) | (r10 >> 7 & 7) << 3 | (r11 >> 7 & 7) << 6 | (r12 >> 7 & 7) << 9,
    (r13 >> 7 & 7) | (r14 >> 7 & 7) << 3 | (r15 >> 7 & 7) << 6 | (r16 >> 7 & 7) << 9));

  // 16  ..................x....
  // 15  ..................x....
  // 14  ..................x....
  // 13  ..................x....
  // 12  ..................x....
  // 11  ..................x....
  // 10  ..................x....
  //  9  ..................x....
  //   (rows 8..2 up: none)
  //  1  .............xxxxxxxxxx
  //  0  xxxxxxxxxxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0x3ffff, r1 >> 4 & 0x3ff, f.col16));

  //surrounding24, the current run (length and colour) and the run up the column
  mapL.set(hash(i++, f.surrounding24, f.runIdx | runValue << 4, static_cast<uint32_t>(f.vrun)));

  //  3  ........xxxxxxxxxxxxxxxx........
  //  2  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  1  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  0  ...............?
  mapL.set(hash(i++, r1 & 0xffffff, r2 & 0xffffff, r3 & 0xffff, (r1b & 0xff) | (r2b & 0xffull) << 8));

  //  3  xxxxxxxxx
  //  2  xxxxxxxxx
  //  1  xxxxxxxxx
  //  0  ...x?
  mapL.set(hash(i++, y | (r1 >> 4 & 0x1ff) << 1, (r2 >> 4 & 0x1ff) | (r3 >> 4 & 0x1ff) << 9));

  //  8  xxxxxxxxxxxxxxxx
  //  7  xxxxxxxxxxxxxxxx
  //  6  xxxxxxxxxxxxxxxx
  //  5  xxxxxxxxxxxxxxxx
  //   (rows 4..1 up: none)
  //  0  ......x?
  mapL.set(hash(i++, r5 & 0xffff, r6 & 0xffff, r7 & 0xffff, r8 & 0xffff, y));

  // 12  xxxxxxxxxxxxxxxx
  // 11  xxxxxxxxxxxxxxxx
  // 10  xxxxxxxxxxxxxxxx
  //  9  xxxxxxxxxxxxxxxx
  //   (rows 8..1 up: none)
  //  0  ......x?
  mapL.set(hash(i++, r9 & 0xffff, r10 & 0xffff, r11 & 0xffff, r12 & 0xffff, y));

  //  3  xxxxxxxxxxxxxxx
  //  2  xxxxxxxxxxxxxxx
  //  1  xxxxxxxxxxxxxxx
  //  0  ......x?
  mapL.set(hash(i++, r1 >> 1 & 0x7fff, r2 >> 1 & 0x7fff, r3 >> 1 & 0x7fff, y));

  //  1  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  0  ......................x?
  mapL.set(hash(i++, r1 & 0xffffffffull, r1b & 0xff, y));

  //  3  ................xxxxxxxxxxxxxxxxxxxxxxxx
  //  2  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  1  ........................................
  //  0  ......................x?
  mapL.set(hash(i++, r2 & 0xffffffffull, (r2b & 0xff) | (r3b & 0xffull) << 8, r3 & 0xffff, y));

  // 16  ........x........
  // 15  ........x........
  // 14  ........x........
  // 13  ........x........
  // 12  ........x........
  // 11  ........x........
  // 10  ........x........
  //  9  ........x........
  //  8  ........x........
  //  7  ........x........
  //  6  ........x........
  //  5  ........x........
  //  4  ........x........
  //  3  ........x........
  //  2  ........x........
  //  1  ........xxxxxxxxx
  //  0  xxxxxxxx?
  mapL.set(hash(i++, f.col8 | f.col16 << 8, r0 & 0xff, r1 & 0xff));

  //  0  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx?
  mapL.set(hash(i++, r0 & 0xffffffffull, y));

  // 16  ..x..
  // 15  ..x..
  // 14  ..x..
  // 13  ..x..
  // 12  ..x..
  // 11  ..x..
  // 10  ..x..
  //  9  ..x..
  //  8  ..x..
  //  7  ..x..
  //  6  ..x..
  //  5  ..x..
  //  4  ..x..
  //  3  ..x..
  //  2  xxxxx
  //  1  xxxxx
  //  0  xx?
  mapL.set(hash(i++, f.col8 | f.col16 << 8, f.surrounding12));

  // 16  .x.
  // 15  .x.
  // 14  .x.
  // 13  .x.
  // 12  .x.
  // 11  .x.
  // 10  .xx
  //  9  .xx
  //  8  .x.
  //  7  .x.
  //  6  .x.
  //  5  .x.
  //  4  .x.
  //  3  .x.
  //  2  .xx
  //  1  .xx
  //  0  x?
  mapL.set(hash(i++, (r1 >> 7 & 3) | (r2 >> 7 & 3) << 2 | (r9 >> 7 & 3) << 4 | (r10 >> 7 & 3) << 6, f.col8 | f.col16 << 8, y));

  //  4  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  3  xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
  //  2  ................................
  //  1  ................................
  //  0  ......................x?
  mapL.set(hash(i++, r3 & 0xffffffffull, r4 & 0xffffffffull, y));

  //the row above, read further ahead: from 7 behind to 16 ahead
  //  1  xxxxxxxxxxxxxxx.xxxxxxxx
  //  0  ......x?
  mapL.set(hash(i++, y | (r1 >> 1 & 0x7fff) << 1, r1b & 0xff));

  //  2  ......xxxxx....xxxxxxxx
  //  1  ......xxxxx....xxxxxxxx
  //  0  xxxxxx?
  mapL.set(hash(i++, y | (r0 & 0x3f) << 1, (r1 >> 4 & 0x1f) | (r1b & 0xff) << 5, (r2 >> 4 & 0x1f) | (r2b & 0xff) << 5));

  //what is coming on the three rows above, beyond the reach of the other templates
  //  3  ..........xxxxxxxx
  //  2  ..........xxxxxxxx
  //  1  xxx.......xxxxxxxx
  //  0  x?
  mapL.set(hash(i++, y | (r1 >> 7 & 7) << 1, (r1b & 0xff) | (r2b & 0xff) << 8 | (r3b & 0xffull) << 16));

  //////////////////////////////////////////////////////////////////////////////
  // Match models, runs and directions
  //////////////////////////////////////////////////////////////////////////////

  //the pixels the four match models expect, and surrounding12
  mapL.set(hash(i++, f.matchVote4, f.surrounding12));

  //how far the ink runs up the seven columns -3..+3 (see readColumns), with W and WW
  mapL.set(hash(i++, (f.vrunLLL - 1) | (f.vrunLL - 1) << 3 | (f.vrunL - 1) << 6 | (f.vrun - 1) << 9 |
    (f.vrunR - 1) << 12 | (f.vrunRR - 1) << 15 | (f.vrunRRR - 1) << 18, y | (r0 & 3) << 1));

  //the same for the three middle columns over 16 rows and the two beside them over 8, with
  //surrounding12 and the ink in the column above
  mapL.set(hash(i++, (f.vrun16 - 1) | (f.vrun16L - 1) << 4 | (f.vrun16R - 1) << 8 | (f.vrunLL - 1) << 12 | (f.vrunRR - 1) << 15,
    f.surrounding12, f.colCount));

  //the lengths of the current run and the two before it, the colour of the current run,
  //surrounding12, and the next colour change on the row above (b1, see trackRuns)
  mapL.set(hash(i++, f.runIdx | runBucket(prevRun1) << 4 | runBucket(prevRun2) << 8 | runValue << 12, f.surrounding12, f.vb1));

  //the diagonals and surrounding12: a slanted stroke, or a corner?
  //  8  x...............x
  //  7  .x.............x.
  //  6  ..x...........x..
  //  5  ...x.........x...
  //  4  ....x.......x....
  //  3  .....x.....x.....
  //  2  ......xxxxx......
  //  1  ......xxxxx......
  //  0  ......xx?
  mapL.set(hash(i++, f.diagNW | f.diagNE << 8, f.surrounding12));

  //each diagonal two pixels wide, which gives the width of a slanted stroke, not only its edge;
  //with W, WW and WWW
  //  8  xx...............xx
  //  7  .xx.............xx.
  //  6  ..xx...........xx..
  //  5  ...xx.........xx...
  //  4  ....xx.......xx....
  //  3  .....xx.....xx.....
  //  2  ......xx...xx......
  //  1  .......xx.xx.......
  //  0  .......xx?
  mapL.set(hash(i++, f.diagNW | f.diagNW2 << 8 | f.diagNE << 16 | f.diagNE2 << 24, y | (r0 & 3) << 1));

  //the very steep and very flat slopes (see readDirections), with W, WW, WWW and the three pixels above
  mapL.set(hash(i++, f.vsteepNW | f.vsteepNE << 8 | f.vflatNW << 16, y | (r0 & 3) << 1 | (r1 >> 7 & 7) << 3));

  //the steep and flat slopes, with W, WW and WWW
  mapL.set(hash(i++, f.steepNW | f.steepNE << 8, f.flatNW | f.flatNE << 8, y | (r0 & 3) << 1));

  //how far the ink runs in every direction: the diagonals, the slopes, the columns -1..+1 and the
  //current row; with surrounding4 and W
  mapL.set(hash(i++,
    (f.drunNW - 1) | (f.drunNE - 1) << 3 | (f.srunNW - 1) << 6 | (f.srunNE - 1) << 9 |
    (f.frunNW - 1) << 12 | (f.vrun - 1) << 15 | f.runIdx << 18,
    (f.vsrunNW - 1) | (f.vfrunNW - 1) << 3 | (f.vrunL - 1) << 6 | (f.vrunR - 1) << 9, f.surrounding4, y));

  //////////////////////////////////////////////////////////////////////////////
  // The shape of the ink, blurred
  //
  // Two prints of the same letter differ in a pixel here and there, and an exact
  // template treats them as two different contexts. These templates merge
  // neighbouring pixels first, so both prints give the same context: rows are
  // merged in pairs, and each pixel with the one to its left. A merged cell is
  // ink if any of its pixels is (the ink dilated), or only if all of them are
  // (the ink eroded).
  //
  // The diagrams show all the pixels that go into the cells.
  //////////////////////////////////////////////////////////////////////////////
  {
    const uint64_t o12 = (r1 | r2), o34 = (r3 | r4), o56 = (r5 | r6);
    const uint64_t a12 = (r1 & r2), a34 = (r3 & r4), a56 = (r5 & r6);
    //rows 1..6, ten cells across, dilated; then the same eroded; with W, WW and WWW
    //  6  xxxxxxxxxxx
    //  5  xxxxxxxxxxx
    //  4  xxxxxxxxxxx
    //  3  xxxxxxxxxxx
    //  2  xxxxxxxxxxx
    //  1  xxxxxxxxxxx
    //  0  ...xxx?
    mapL.set(hash(i++, ((o12 | o12 >> 1) >> 4 & 0x3ff) | ((o34 | o34 >> 1) >> 4 & 0x3ffull) << 10 | ((o56 | o56 >> 1) >> 4 & 0x3ffull) << 20,
      y | (r0 & 7) << 1));
    mapL.set(hash(i++, ((a12 & a12 >> 1) >> 4 & 0x3ff) | ((a34 & a34 >> 1) >> 4 & 0x3ffull) << 10 | ((a56 & a56 >> 1) >> 4 & 0x3ffull) << 20,
      y | (r0 & 7) << 1));

    //rows 1 and 2, thirteen cells across, both dilated and eroded: where there certainly is ink,
    //where there certainly is not, and the uncertain edge between them; with surrounding4
    //  2  xxxxxxxxxxxxxx
    //  1  xxxxxxxxxxxxxx
    //  0  .......x?
    mapL.set(hash(i++, (o12 | o12 >> 1) >> 3 & 0x1fff, (a12 & a12 >> 1) >> 3 & 0x1fff, f.surrounding4, y));

    //rows 1..12, eight cells across, dilated: tall enough for a whole letter; with W and WW
    // 12  xxxxxxxxx
    // 11  xxxxxxxxx
    // 10  xxxxxxxxx
    //  9  xxxxxxxxx
    //  8  xxxxxxxxx
    //  7  xxxxxxxxx
    //  6  xxxxxxxxx
    //  5  xxxxxxxxx
    //  4  xxxxxxxxx
    //  3  xxxxxxxxx
    //  2  xxxxxxxxx
    //  1  xxxxxxxxx
    //  0  ...xx?
    const uint64_t o78 = (r7 | r8), o9a = (r9 | r10), obc = (r11 | r12);
    mapL.set(hash(i++, ((o12 | o12 >> 1) >> 5 & 0xff) | ((o34 | o34 >> 1) >> 5 & 0xffull) << 8 | ((o56 | o56 >> 1) >> 5 & 0xffull) << 16,
      ((o78 | o78 >> 1) >> 5 & 0xff) | ((o9a | o9a >> 1) >> 5 & 0xffull) << 8 | ((obc | obc >> 1) >> 5 & 0xffull) << 16,
      y | (r0 & 3) << 1));

    //rows 1..6, ten cells across, each cell three pixels wide, dilated: the coarsest shape that
    //still tells letters apart; with surrounding4
    //  6  xxxxxxxxxxxx
    //  5  xxxxxxxxxxxx
    //  4  xxxxxxxxxxxx
    //  3  xxxxxxxxxxxx
    //  2  xxxxxxxxxxxx
    //  1  xxxxxxxxxxxx
    //  0  ......x?
    const uint64_t t12 = o12 | o12 >> 1 | o12 >> 2, t34 = o34 | o34 >> 1 | o34 >> 2, t56 = o56 | o56 >> 1 | o56 >> 2;
    mapL.set(hash(i++, (t12 >> 4 & 0x3ff) | (t34 >> 4 & 0x3ffull) << 10 | (t56 >> 4 & 0x3ffull) << 20, f.surrounding4, y));
  }

  //////////////////////////////////////////////////////////////////////////////
  // Long lines, the reference line, and the sparse templates
  //////////////////////////////////////////////////////////////////////////////

  //the steep and the flat slope to the left over 16 rows (see readDirections), how far the ink
  //runs along each, and W
  mapL.set(hash(i++, f.steepNW | f.steepNW16 << 8 | f.flatNW << 16 | f.flatNW16 << 24, (f.srunNW16 - 1) | (f.frunNW16 - 1) << 4, y));

  //the diagonal to the left over 16 rows, how far the ink runs along it, and surrounding4
  // 16  x.................
  // 15  .x................
  // 14  ..x...............
  // 13  ...x..............
  // 12  ....x.............
  // 11  .....x............
  // 10  ......x...........
  //  9  .......x..........
  //  8  ........x.........
  //  7  .........x........
  //  6  ..........x.......
  //  5  ...........x......
  //  4  ............x.....
  //  3  .............x....
  //  2  ..............x...
  //  1  ...............xxx
  //  0  ...............x?
  mapL.set(hash(i++, f.diagNW | f.diagNW16 << 8, (f.drunNW16 - 1) | f.surrounding4 << 4));

  //how far the ink runs up the columns -1..+1 (and the middle one over 16 rows), the columns
  //either side, and surrounding4
  mapL.set(hash(i++, (f.vrun - 1) | (f.vrunL - 1) << 3 | (f.vrunR - 1) << 6 | (f.vrun16 - 1) << 9,
    f.col8L | f.col8R << 8, f.surrounding4));

  //the next two colour changes on the row above (b1 and b2, see trackRuns), the current run and
  //surrounding12: where the run above ends, and what the pixels look like as we get there
  mapL.set(hash(i++, f.vb1 | f.vb2 << 5 | f.curCol << 10, f.runIdx, f.surrounding12));

  //b1 and the current run, with the two rows above, read wide
  //  2  .xxxxxxx.
  //  1  xxxxxxxxx
  //  0  ....?
  mapL.set(hash(i++, f.vb1 | f.curCol << 5 | f.runIdx << 6, r1 >> 4 & 0x1ff, r2 >> 5 & 0x7f));

  //the sparse templates G22 and H23
  // 11  x...............
  // 10  ...x.....x....x.
  //  9  ................
  //  8  ............x...
  //  7  ................
  //  6  ......x.x....x.x
  //  5  ................
  //  4  ................
  //  3  ........xx......
  //  2  ......x.x...x.x.
  //  1  ...x....xxx.....
  //  0  x...x..x?
  mapL.set(hash(i++, sparse(G22, 22)));
  // 14  ..........x.....
  // 13  ................
  // 12  .....x.......x..
  // 11  ................
  // 10  .x....x.x.......
  //  9  ................
  //  8  ................
  //  7  ................
  //  6  ..x.....x.x...x.
  //  5  ................
  //  4  ......x.x...x...
  //  3  ................
  //  2  ......x..x..x..x
  //  1  ........xxx.....
  //  0  x...x..x?
  mapL.set(hash(i++, sparse(H23, 23)));
}

//The mixer inputs that do not come from the hashed contexts.
void Image1BitModel::addInputs(Mixer& m) {
  //the indexed contexts: a prediction from the bit history, and one from the counts
  stateMap.subscribe();
  for (int i = 0; i < N; ++i) {
    const uint8_t state = t[cxt[i]];
    if (state == 0) {
      stateMap.skip(i);
      m.add(0);
      m.add(0);
    }
    else {
      int p1 = stateMap.p2(i, state);
      int st = stretch(p1) >> 1;
      m.add(st);
      int n0 = counts[cxt[i]] >> 8;
      int n1 = counts[cxt[i]] & 255;
      n0++;
      n1++;

      p1 = (n1 << 12) / (n0 + n1);
      st = stretch(p1) >> 1;
      m.add(st);
    }
  }

  //how much ink is around. The counts are passed the other way round (ink as 0s): in a dithered
  //image, much ink around predicts paper next, and the mixer learns the sign it needs.
  add(m, f.bitcount04, 04 - f.bitcount04);
  add(m, f.bitcount12, 12 - f.bitcount12);
  add(m, f.bitcount24, 24 - f.bitcount24);
  add(m, f.bitcount40, 40 - f.bitcount40);
  add(m, f.colCount, 8 - f.colCount);

  //a long run predicts more of the same. At the start of a row runLength is 0: no prediction.
  const uint32_t rl = runLength < 24 ? runLength : 24;
  add(m, runValue != 0 ? 0 : rl, runValue != 0 ? rl : 0);

  //the same for the runs up the column and along the two diagonals
  {
    const uint32_t vl = std::min(f.vrun16, 16u);
    add(m, (f.col8 & 1) != 0 ? 0 : vl, (f.col8 & 1) != 0 ? vl : 0);
    const uint32_t dl = std::min(f.drunNW16, 16u);
    add(m, (f.diagNW & 1) != 0 ? 0 : dl, (f.diagNW & 1) != 0 ? dl : 0);
    const uint32_t el = std::min(f.drunNE, 8u);
    add(m, (f.diagNE & 1) != 0 ? 0 : el, (f.diagNE & 1) != 0 ? el : 0);
  }

  //the match models: how far each one can be trusted is learned from its length, the pixel it
  //expects, and how closely its source resembled the current neighbourhood
  matchStateMap.subscribe();
  for (int k = 0; k < nMatch; ++k) {
    if (predictedBit[k] < 0) {
      matchStateMap.skip(k);
      m.add(0);
      continue;
    }
    const uint32_t exp = static_cast<uint32_t>(predictedBit[k]);
    const uint32_t ctx = matchBucketFine(matchLen[k]) | exp << 4 | matchResid[k] << 5; //7 bits
    m.add(stretch(matchStateMap.p2(k, ctx)) >> 1);
  }
}

//A direct predictor: the probability that the next pixel is 1, from how many 0s (n0) and 1s (n1)
//speak for each, given to the mixer in two forms.
void Image1BitModel::add(Mixer& m, uint32_t n0, uint32_t n1) {
  int p1 = ((n1 + 1) << 12) / ((n0 + n1) + 2);
  const int a = stretch(p1) >> 1;
  const int b = (p1 - 2048) >> 2;
  m.add(a); m.add(b);
}

//The mixer weight sets: each line selects one set of weights by what it describes.
void Image1BitModel::setMixerContexts(Mixer& m, const int y, const int bpos) {
  m.set(f.surrounding4, 16);
  m.set(f.mCtx6, 64);
  m.set(f.bitcount40, 41);                                                      //how much ink is around
  m.set(f.runIdx | runValue << 4, 32);                                          //the current run
  m.set((f.vrun - 1) | (f.col8 & 1) << 3 | y << 4 | (r1 >> 7 & 1) << 5, 64);    //the run up the column, N, W and NE
  m.set((bpos & 3) | (yRow & 3) << 2, 16);                                      //the dither phase
  m.set(f.matchVote | (f.lenBucketA >> 2) << 4 | (f.lenBucketB >> 2) << 5, 64); //match models A and B: the pixel they expect, and whether their match is long
  m.set((f.vrun - 1) | (f.vrunL - 1) << 3 | (f.vrunR - 1) << 6, 512);           //the runs up the columns -1..+1
  m.set((f.drunNW - 1) | (f.drunNE - 1) << 3 | (f.diagNW & 1) << 6 | (f.diagNE & 1) << 7, 256); //the runs along the diagonals
  m.set(f.runIdx | runBucket(prevRun1) << 4, 256);                              //the current run and the one before it
  m.set(bitCount(f.dil6) * 7 + bitCount(f.ero6), 49);                           //how much of the blurred shape above is ink
  //the nearest pixels:
  //  1  ......xx
  //  0  xxxxxx?
  m.set(static_cast<uint32_t>((r0 & 0x3f) | (r1 >> 7 & 3) << 6), 256);
  m.set(f.matchMode, 33);                                                       //the match models' verdict
}

//Hand the SSE stage a summary of the neighbourhood (see Shared::State.Image1).
void Image1BitModel::publishToSSE() {
  auto& im = shared->State.Image1;
  im.ctx12 = f.surrounding12;
  im.shape = (f.drunNW - 1) | (f.drunNE - 1) << 3 | (f.vrun - 1) << 6 | (f.srunNW - 1) << 9 |
    (f.srunNE - 1) << 12 | (f.frunNW - 1) << 15 | (f.vrun16 - 1) << 18;
  im.column = f.col8 | f.col16 << 8;
  im.rowAbove = static_cast<uint32_t>(r1 >> 1 & 0x7fff);
  im.run = static_cast<uint8_t>(f.runIdx | runValue << 4);
  im.ref = static_cast<uint8_t>(f.vb1 | f.curCol << 5);
  im.ink = static_cast<uint8_t>(f.bitcount40);
  im.match = static_cast<uint8_t>(f.matchMode);
}


// ===== src/model/Image24BitModel.cpp =====

Image24BitModel::Image24BitModel(Shared* const sh, const uint64_t size) :
  shared(sh),
  cm(sh, size, nCM, 64),
  mapL{ sh, nLSM, 23, 74 },     /* LargeStationaryMap : Contexts, HashBits, Scale=64, Rate=16 */
  mapR1{ sh, nRM, 1 << 7, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapR2{ sh, nRM, 1 << 5, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapR3{ sh, nRM, 1 << 7, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapOLS1{ sh, nOLS, 1 << 7, 74 },  /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapOLS2{ sh, nOLS, 1 << 5, 74 }   /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
{
  for (int i = 0; i < nOLS; i++) {
    for (int j = 0; j < 4; j++) { // RGBA color components
      ols[i][j] = create_OLS_float(sh->chosenSimd, num[i], 1, lambda[i], nu);
    }
  }
}

// Get pixel value
// Clamp to nearest valid pixel in the same color plane, when none exists, return a fixed fallback
ALWAYS_INLINE uint8_t Image24BitModel::Px(int relX, int relY, int colorShift) const {
  relX *= stride;
  relX += colorShift;
  int x0 = x - relX;
  while (x0 < 0) { //we are over the left side, we need to go right until we arrive to the pixel area
    relX -= stride;
    x0 += stride;
  }
  while (x0 >= w) { //we are over the right side, we need to go left until we arrive to the pixel area
    relX += stride;
    x0 -= stride;
  }
  if (line - relY < 0) //we are over the top, let's navigate to the first row
    relY = line;
  int offset = relY * w + relX;
  if (offset <= 0) {
    if (relY < line) //no left (or right) pixel, but there is still room above
      offset += w; //go above
    else
      return 127; //no valid neighbor pixel found
  }
  INJECT_SHARED_buf
  return buf(offset);
}

ALWAYS_INLINE uint8_t Image24BitModel::Ls(int relX, int relY) const {
  if (line - relY < 0)  //the lossBuf buffer is pre-filled with 255 initially, so this check is not really necessary
    return 255;
  relX *= stride;
  if (x - relX < 0)
    return 255;
  if (x - relX >= w)
    return 255;
  int offset = relY * w + relX;
  const uint32_t valuesPerByte = 1;
  return lossBuf((offset - 1) * valuesPerByte + 1);
}

ALWAYS_INLINE uint8_t Image24BitModel::GetPredErr(const uint32_t ctxIndex, int relX, int relY) const {
  if (line - relY < 0)  //the predErrBuf buffer is pre-filled with 255 initially, so this check is not really necessary
    return 255;
  relX *= stride;
  if (x - relX < 0)
    return 255;
  if (x - relX >= w)
    return 255;
  int offset = relY * w + relX;
  const uint32_t valuesPerByte = (nRM + nOLS);
  return predErrBuf((offset - 1) * valuesPerByte + ctxIndex + 1);
}

ALWAYS_INLINE uint32_t Image24BitModel::GetPredErrAvg(const uint32_t predictorIndex) const {
  // Current pixel's prediction confidence is based on the already known error at neighboring pixel predictions
  uint32_t predErrW = GetPredErr(predictorIndex, 1, 0);
  uint32_t predErrN = GetPredErr(predictorIndex, 0, 1);
  uint32_t predErrNW = GetPredErr(predictorIndex, 1, 1);
  uint32_t predErrNE = GetPredErr(predictorIndex, -1, 1);
  uint32_t predErrWW = GetPredErr(predictorIndex, 2, 0);
  uint32_t predErrNN = GetPredErr(predictorIndex, 0, 2);
  uint32_t predErrAvg = (2 * predErrW + 2 * predErrN + predErrNE + predErrNW + predErrWW + predErrNN) >> 3; // 0..255
  return predErrAvg;
}

ALWAYS_INLINE static int avg(int x, int y) {
  return (x + y + 1) >> 1;  //note: rounding here works properly only when x+y is non-negative, but we don't really need the function to be aware of negative values as they are rare
}

// Stores a prediction with a spread signal based on the distance between two reference pixels.
// ref1 and ref2 are spatial reference pixels; their difference measures local variation -
// higher difference = less confident = used as a context component in the probability map.
// Use when the prediction value is computed externally (e.g. a complex algebraic expression)
// but a natural pair of reference pixels still exists to supply the spread signal.
ALWAYS_INLINE void Image24BitModel::MakePrediction(int i, uint8_t ref1, uint8_t ref2, int prediction) {
  uint32_t absdiff = rabs(ref1, ref2);
  predictions[i] = absdiff << 16 | ((prediction) & 65535);
}

// Trend extrapolation: continues the gradient observed from pxFar to px,
// starting from the spatial origin.
// prediction = origin + (px - pxFar)
// spread = rabs(px - pxFar): larger gradient = more uncertain prediction.
// Use when pixels are expected to follow a consistent directional trend
// (e.g. continuing a horizontal, vertical, or diagonal gradient).
ALWAYS_INLINE void Image24BitModel::MakePredictionTrend(int i, int px, int pxFar, int origin) {
  uint32_t absdiff = rabs(px, pxFar);
  int prediction = origin + px - pxFar;
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

// Smoothed trend: applies the gradient at half strength instead of full extrapolation.
// prediction = avg(origin, origin + (px - pxFar))
// spread = rabs(px - pxFar): same signal as Trend.
// Use in smoother regions where a full trend extrapolation would overshoot,
// or when the gradient is expected to taper rather than continue linearly.
ALWAYS_INLINE void Image24BitModel::MakePredictionSmooth(int i, int px, int pxFar, int origin) {
  uint32_t absdiff = rabs(px, pxFar);
  int prediction = avg(origin, origin + px - pxFar);
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

ALWAYS_INLINE void Image24BitModel::MakePredictionAvg(int i, int px1, int px2) {
  uint32_t absdiff = rabs(px1, px2);
  int prediction = avg(px1, px2);
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

// Stores only a prediction value; the spread signal is absent (upper 16 bits are zero).
// Use for complex algebraic combinations of multiple pixels where no single
// natural pair of reference pixels exists to derive a meaningful spread signal.
ALWAYS_INLINE void Image24BitModel::MakePredictionC(int i, int prediction) {
  predictions[i] = prediction & 65535;
}

void Image24BitModel::update() {
  INJECT_SHARED_bpos
  INJECT_SHARED_c1

  if (color < 4) // no need to accumulate loss from the padding zone
    loss += shared->State.loss; // += 0..1023

  // for every byte
  if (bpos == 0) {

    INJECT_SHARED_pos
    if (pos - lastPos != 1) {
      init();
    }
    else {
      x++;
      if (x >= w) {
        x = 0;
        line++;
      }
    }
    lastPos = pos;

    INJECT_SHARED_buf
    if (x == 0) {
      color = 0;
    }
    else if( x < w - padding ) {
      color++;
      if( color >= stride ) {
        color = 0;
      }
    } else {
      color = 4; // flag for padding zone
    }

    if (color < 4) { // we are in the pixel area

      lossBuf.add(static_cast<uint8_t>(min(loss >> 2, 255)));  // 0..255
      loss = 0;

      WWWWWW = Px(6, 0, 0); //buf(6*stride)
      WWWWW = Px(5, 0, 0); //buf(5*stride)
      WWWW = Px(4, 0, 0); //buf(4*stride)
      WWW = Px(3, 0, 0); //buf(3*stride)
      WW = Px(2, 0, 0); //buf(2*stride)
      W = Px(1, 0, 0); //buf(1*stride)

      NWWWW = Px(4, 1, 0); //buf(4*stride + 1*w)
      NWWW = Px(3, 1, 0); //buf(3*stride + 1*w)
      NWW = Px(2, 1, 0); //buf(2*stride + 1*w)
      NW = Px(1, 1, 0); //buf(1*stride + 1*w)
      N = Px(0, 1, 0); //buf(1*w)

      NE = Px(-1, 1, 0); //buf(-1*stride + 1*w)
      NEE = Px(-2, 1, 0); //buf(-2*stride + 1*w)
      NEEE = Px(-3, 1, 0); //buf(-3*stride + 1*w)
      NEEEE = Px(-4, 1, 0); //buf(-4*stride + 1*w)

      NNNWWW = Px(3, 3, 0); //buf(3*stride + 3*w)
      NNWWW = Px(3, 2, 0); //buf(3*stride + 2*w)
      NNWW = Px(2, 2, 0); //buf(2*stride + 2*w)
      NNW = Px(1, 2, 0); //buf(1*stride + 2*w)
      NN = Px(0, 2, 0); //buf(2*w)

      NNE = Px(-1, 2, 0); //buf(-1*stride + 2*w)
      NNEE = Px(-2, 2, 0); //buf(-2*stride + 2*w)
      NNEEE = Px(-3, 2, 0); //buf(-3*stride + 2*w)

      NNNWW = Px(2, 3, 0); //buf(2*stride + 3*w)
      NNNW = Px(1, 3, 0); //buf(1*stride + 3*w)
      NNN = Px(0, 3, 0); //buf(3*w)

      NNNE = Px(-1, 3, 0); //buf(-1*stride + 3*w)
      NNNEE = Px(-2, 3, 0); //buf(-2*stride + 3*w)
      NNNEEE = Px(-3, 3, 0); //buf(-3*stride + 3*w)

      NNNNW = Px(1, 4, 0); //buf(1*stride + 4*w)
      NNNN = Px(0, 4, 0); //buf(4*w)
      NNNNE = Px(-1, 4, 0); //buf(-1*stride + 4*w)
      NNNNN = Px(0, 5, 0); //buf(5*w)
      NNNNNN = Px(0, 6, 0); //buf(6*w)

      WWp1 = Px(2, 0, 1); //buf(2*stride + 1)
      Wp1 = Px(1, 0, 1); //buf(1*stride + 1)
      p1 = Px(0, 0, 1); //buf(1)
      NWp1 = Px(1, 1, 1); //buf(1*stride + 1*w + 1)
      Np1 = Px(0, 1, 1); //buf(1*w + 1)
      NEp1 = Px(-1, 1, 1); //buf(-1*stride + 1*w + 1)
      NNp1 = Px(0, 2, 1); //buf(2*w + 1)

      uint8_t NNNp1 = Px(0, 3, 1); //buf(3*w + 1)
      uint8_t WWWp1 = Px(3, 0, 1); //buf(3*stride + 1)
      uint8_t NNWp1 = Px(1, 2, 1); //buf(1*stride + 2*w + 1)
      uint8_t NNEp1 = Px(-1, 2, 1); //buf(-1*stride + 2*w + 1)
      uint8_t NWWp1 = Px(2, 1, 1); //buf(2*stride + 1*w + 1)
      uint8_t NEEp1 = Px(-2, 1, 1); //buf(-2*stride + 1*w + 1)
      uint8_t NNWWp1 = Px(2, 2, 1); //buf(2*stride + 2*w + 1)
      uint8_t NNEEp1 = Px(-2, 2, 1); //buf(-2*stride + 2*w + 1)
      uint8_t NNNNp1 = Px(0, 4, 1); //buf(4*w + 1)
      uint8_t NNNNNNp1 = Px(0, 6, 1); //buf(6*w + 1)
      uint8_t WWWWp1 = Px(4, 0, 1); //buf(4*stride + 1)
      uint8_t WWWWWWp1 = Px(6, 0, 1); //buf(6*stride + 1)
      uint8_t NNNWp1 = Px(1, 3, 1); //buf(1*stride + 3*w + 1)
      uint8_t NNNEp1 = Px(-1, 3, 1); //buf(-1*stride + 3*w + 1)

      WWp2 = Px(2, 0, 2); //buf(2*stride + 2)
      Wp2 = Px(1, 0, 2); //buf(1*stride + 2)
      p2 = Px(0, 0, 2); //buf(2)
      NWp2 = Px(1, 1, 2); //buf(1*stride + 1*w + 2)
      Np2 = Px(0, 1, 2); //buf(1*w + 2)
      NEp2 = Px(-1, 1, 2); //buf(-1*stride + 1*w + 2)
      NNp2 = Px(0, 2, 2); //buf(2*w + 2)

      uint8_t NNNp2 = Px(0, 3, 2); //buf(3*w + 2)
      uint8_t WWWp2 = Px(3, 0, 2); //buf(3*stride + 2)
      uint8_t NNWp2 = Px(1, 2, 2); //buf(1*stride + 2*w + 2)
      uint8_t NNEp2 = Px(-1, 2, 2); //buf(-1*stride + 2*w + 2)
      uint8_t NWWp2 = Px(2, 1, 2); //buf(2*stride + 1*w + 2)
      uint8_t NEEp2 = Px(-2, 1, 2); //buf(-2*stride + 1*w + 2)
      uint8_t NNWWp2 = Px(2, 2, 2); //buf(2*stride + 2*w + 2)
      uint8_t NNEEp2 = Px(-2, 2, 2); //buf(-2*stride + 2*w + 2)
      uint8_t NNNNp2 = Px(0, 4, 2); //buf(4*w + 2)
      uint8_t NNNNNNp2 = Px(0, 6, 2); //buf(6*w + 2)
      uint8_t WWWWp2 = Px(4, 0, 2); //buf(4*stride + 2)
      uint8_t WWWWWWp2 = Px(6, 0, 2); //buf(6*stride + 2)
      uint8_t NNNWp2 = Px(1, 3, 2); //buf(1*stride + 3*w + 2)
      uint8_t NNNEp2 = Px(-1, 3, 2); //buf(-1*stride + 3*w + 2)

      uint8_t NNNNWW = Px(2, 4, 0); //buf(2*stride + 4*w)
      uint8_t NNWWWW = Px(4, 2, 0); //buf(4*stride + 2*w)
      uint8_t NNNNEE = Px(-2, 4, 0); //buf(-2*stride + 4*w)
      uint8_t NNEEEE = Px(-4, 2, 0); //buf(-4*stride + 2*w)
      uint8_t NNNNNNWW = Px(2, 6, 0); //buf(2*stride + 6*w)
      uint8_t NNNNNNEE = Px(-2, 6, 0); //buf(-2*stride + 6*w)
      uint8_t NNNNWWWW = Px(4, 4, 0); //buf(4*stride + 4*w)
      uint8_t NNNNEEEE = Px(-4, 4, 0); //buf(-4*stride + 4*w)
      uint8_t NEEEEEE = Px(-6, 1, 0); //buf(-6*stride + 1*w)
      uint8_t NNNWWWW = Px(4, 3, 0); //buf(4*stride + 3*w)
      uint8_t NNNEEEE = Px(-4, 3, 0); //buf(-4*stride + 3*w)
      uint8_t NEEEEE = Px(-5, 1, 0); //buf(-5*stride + 1*w)
      uint8_t NEEEEEEE = Px(-7, 1, 0); //buf(-7*stride + 1*w)
      //uint8_t NNNNEEE = Px(-3, 4, 0); //buf(-3*stride + 4*w)
      //uint8_t NNNNWWW = Px(3, 4, 0); //buf(3*stride + 4*w)

      // mixer context: edge detection

      // Vertical ↓ (0)
      int r_N_vert = rabs(N, (NN * 2 - NNN));
      int r_W_vert = rabs(W, (NW * 2 - NNW));
      int r_NE_vert = rabs(NE, (NNE * 2 - NNNE));
      int score_vert = (2 * r_N_vert + r_W_vert + r_NE_vert) / 4;

      // Horizontal → (1)
      int r_W_horiz = rabs(W, (WW * 2 - WWW));
      int r_N_horiz = rabs(N, (NW * 2 - NWW));
      int score_horiz = (2 * r_W_horiz + r_N_horiz) / 3;

      // Diagonal ↘ (2)
      int r_NW_diag1 = rabs(NW, (NNWW * 2 - NNNWWW));
      int r_N_diag1 = rabs(N, (NNW * 2 - NNNWW));
      int r_W_diag1 = rabs(W, (NWW * 2 - NNWWW));
      int score_diag1 = (2 * r_NW_diag1 + r_N_diag1 + r_W_diag1) / 4;

      // Diagonal ↙ (3)
      int r_NE_diag2 = rabs(NE, (NNEE * 2 - NNNEEE));
      int r_N_diag2 = rabs(N, (NNE * 2 - NNNEE));
      int r_NEE_diag2 = rabs(NEE, (NNEEE * 2 - NNNEEEE));
      int score_diag2 = (2 * r_NE_diag2 + r_N_diag2 + r_NEE_diag2) / 4;

      ctx_best_direction = 0; // 0..3

      int ctx_best_score = score_vert;
      if (score_horiz < ctx_best_score) { ctx_best_score = score_horiz; ctx_best_direction = 1; }
      if (score_diag1 < ctx_best_score) { ctx_best_score = score_diag1; ctx_best_direction = 2; }
      if (score_diag2 < ctx_best_score) { ctx_best_score = score_diag2; ctx_best_direction = 3; }

      ctx_best_residual = DiffQt(0, ctx_best_score); // 0..7

      //what was the total cost at the neighboring pixes of this same channel
      lossQ = //0 x 6 .. 255 x 6 = 0 .. 1530
        Ls(1, 0) + // W
        Ls(0, 1) + // N
        Ls(2, 0) + // WW
        Ls(0, 2) + // NN
        Ls(1, 1) + // NW
        Ls(-1, 1); // NE

      // let's trim the higher part (640-1530) - it is almost always completely empty OR such high values indicate that we are off frame
      // cap at 639 = 16*40 - 1, so lossQ4 = lossQ/40 fits in [0..15]
      lossQ = min(lossQ, 639);
      shared->State.Image.lossQ = lossQ; //0..639

      // Written in reverse order (nRM+nOLS-1 down to 0) so that GetPredErr()'s
      // read formula  predErr((offset-1) * (nRM+nOLS) + ctxIndex + 1)  resolves correctly:
      // after writing nRM values, predictor ctxIndex sits at ring offset -(ctxIndex+1).


      uint32_t lowestErr = 255;
      uint8_t bestPredictorIndex = 0;
      static_assert(nRM + nOLS <= 255); // the index need to fit to a byte
      for (int i = nRM + nOLS - 1; i >= 0; i--) {
        short prediction = predictions[i] & 65535;
        uint8_t err;
        if (prediction == INT16_MAX) // currently never happens, todo
          err = 255;
        else
          err = rabs(c1, prediction); // 0..128
        predErrBuf.add(err); //0..63 or 255

        uint32_t linearError = abs(c1 - prediction);
        if (linearError < lowestErr) {
          lowestErr = linearError;
          bestPredictorIndex = i;
        }
      }
      bestPredictorIndexes.add(bestPredictorIndex);

      // these contexts are best for photographic images

      int contextIdx = 0;

      //for tuning:
      //int z = 0;
      //int toskip = shared->tuning_param + z;
      //z++; if (toskip != z) MakePredictionC(contextIdx++, p1);

      // note about p1 and p2:
      // their semantics change depending on 'color':
      // if the image is RGB:
      //  color = 0 (R) -> p1 = previous pixel's B, p2 = previous pixel's G
      //  color = 1 (G) -> p1 =  current pixel's R, p2 = previous pixel's B
      //  color = 2 (B) -> p1 =  current pixel's G, p2 =  current pixel's R <- this is the intention

      // p1-based predictors

      MakePredictionC(contextIdx++, p1); // very strong

      MakePredictionTrend(contextIdx++, p1, Np1, N); // strong
      MakePredictionSmooth(contextIdx++, p1, Np1, N); // strong
      MakePrediction(contextIdx++, p1, Np1, N);

      MakePredictionTrend(contextIdx++, p1, Wp1, W);
      MakePredictionSmooth(contextIdx++, p1, Wp1, W);
      MakePrediction(contextIdx++, p1, Wp1, W);

      MakePredictionTrend(contextIdx++, p1, NEp1, NE); // very strong
      MakePrediction(contextIdx++, p1, NEp1, NE);
      MakePredictionTrend(contextIdx++, p1, NNp1, NN); // very strong
      MakePrediction(contextIdx++, p1, NNp1, NN);

      MakePredictionTrend(contextIdx++, p1, NWp1, NW);
      MakePredictionSmooth(contextIdx++, p1, NWp1, NW);
      MakePrediction(contextIdx++, p1, NWp1, NW);

      MakePredictionTrend(contextIdx++, p1, WWp1, WW);
      MakePrediction(contextIdx++, p1, WWp1, WW);
//    MakePredictionSmooth(contextIdx++, p1, WWp1, WW); // weak

      MakePredictionSmooth(contextIdx++, p1, NNEEp1, NE);

      MakePredictionTrend(contextIdx++, p1, (-3 * WWp1 + 8 * Wp1 + (NEEp1 * 2 - NNEEp1)) / 6, (-3 * WW + 8 * W + (NEE * 2 - NNEE)) / 6);
      MakePredictionTrend(contextIdx++, p1, avg(WWp1, (NEEp1 * 2 - NNEEp1)), avg(WW, (NEE * 2 - NNEE)));

      MakePredictionTrend(contextIdx++, p1, Wp1 + Np1 - NWp1, W + N - NW); // strong
      MakePredictionTrend(contextIdx++, p1, Np1 + NWp1 - NNWp1, N + NW - NNW);
      MakePredictionTrend(contextIdx++, p1, Np1 + NEp1 - NNEp1, N + NE - NNE); //strong
      MakePredictionTrend(contextIdx++, p1, Np1 + NNp1 - NNNp1, N + NN - NNN);
      MakePredictionTrend(contextIdx++, p1, Wp1 + WWp1 - WWWp1, W + WW - WWW);
      MakePredictionTrend(contextIdx++, p1, Wp1 + NEEp1 - NEp1, W + NEE - NE);

//    MakePredictionTrend(contextIdx++, p1, NNp1 + Wp1 - NNWp1, NN + W - NNW); // weak
      MakePredictionTrend(contextIdx++, p1, Np1 * 2 - NNp1, N * 2 - NN); // strong
      MakePredictionTrend(contextIdx++, p1, Wp1 * 2 - WWp1, W * 2 - WW); // very strong

      MakePredictionTrend(contextIdx++, p1, Wp1 + NEp1 - Np1, W + NE - N);
      MakePredictionTrend(contextIdx++, p1, NEp1 + NWp1 - NNp1, NE + NW - NN);
      MakePredictionTrend(contextIdx++, p1, NWp1 + Wp1 - NWWp1, NW + W - NWW);

//    MakePredictionTrend(contextIdx++, p1, NEp1 * 2 - NNEEp1, NE * 2 - NNEE); // weak
//    MakePredictionTrend(contextIdx++, p1, Np1 * 3 - NNp1 * 3 + NNNp1, N * 3 - NN * 3 + NNN); // weak

      // p2-based predictors

      MakePredictionC(contextIdx++, p2); // very strong

      MakePredictionTrend(contextIdx++, p2, Np2, N);
      MakePredictionSmooth(contextIdx++, p2, Np2, N);

      MakePredictionTrend(contextIdx++, p2, Wp2, W);
      MakePredictionSmooth(contextIdx++, p2, Wp2, W);

      MakePredictionTrend(contextIdx++, p2, NEp2, NE);
      MakePredictionTrend(contextIdx++, p2, NNp2, NN);
      MakePredictionSmooth(contextIdx++, p2, NWp2, NW); 
      
      MakePredictionTrend(contextIdx++, p2, WWp2, WW);
//    MakePredictionSmooth(contextIdx++, p2, WWp2, WW); // weak

      MakePredictionSmooth(contextIdx++, p2, NNEEp2, NE); // very strong

      MakePredictionTrend(contextIdx++, p2, Wp2 + Np2 - NWp2, W + N - NW);
      MakePredictionTrend(contextIdx++, p2, Np2 + NWp2 - NNWp2, N + NW - NNW);
      MakePredictionTrend(contextIdx++, p2, Np2 + NEp2 - NNEp2, N + NE - NNE); // strong
//    MakePredictionTrend(contextIdx++, p2, Np2 + NNp2 - NNNp2, N + NN - NNN); // weak
      MakePredictionTrend(contextIdx++, p2, Wp2 + WWp2 - WWWp2, W + WW - WWW);
//    MakePredictionTrend(contextIdx++, p2, Wp2 + NEEp2 - NEp2, W + NEE - NE); // weak

//    MakePredictionTrend(contextIdx++, p2, NNp2 + Wp2 - NNWp2, NN + W - NNW); // weak
      MakePredictionTrend(contextIdx++, p2, Np2 * 2 - NNp2, N * 2 - NN);
      MakePredictionTrend(contextIdx++, p2, Wp2 * 2 - WWp2, W * 2 - WW);

//    MakePredictionTrend(contextIdx++, p2, Wp2 + NEp2 - Np2, W + NE - N);  // weak
//    MakePredictionTrend(contextIdx++, p2, NEp2 + NWp2 - NNp2, NE + NW - NN);  // weak
//    MakePredictionTrend(contextIdx++, p2, NWp2 + Wp2 - NWWp2, NW + W - NWW);  // weak

      MakePredictionTrend(contextIdx++, p2, NWp2 * 2 - NNWWp2, NW * 2 - NNWW);
      MakePredictionTrend(contextIdx++, p2, NEp2 * 2 - NNEEp2, NE * 2 - NNEE);
      
      // predictors using only the current color plane
      
      MakePredictionC(contextIdx++, (N * 3 + W * 3 - NN - WW + 2) >> 2);
      MakePredictionAvg(contextIdx++, W, NEE);


      MakePredictionTrend(contextIdx++, W, NW, N); // very strong
      MakePredictionTrend(contextIdx++, WW, NNWW, NN); //strong?
      MakePredictionTrend(contextIdx++, WWW, NNNWWW, NNN); //strong?
      MakePredictionTrend(contextIdx++, W, N, NE); // strong
//    MakePredictionTrend(contextIdx++, WW, NN, NNEE);  // weak
//    MakePredictionTrend(contextIdx++, WWW, NNN, NNNEEE);  // weak
      MakePredictionTrend(contextIdx++, N, NNW, NW);
      MakePredictionTrend(contextIdx++, N, NNE, NE);
      MakePredictionTrend(contextIdx++, NN, NNNNEE, NNEE);
      MakePredictionTrend(contextIdx++, N, NNN, NN);
      MakePredictionTrend(contextIdx++, NN, NNNNNN, NNNN);
      MakePredictionTrend(contextIdx++, W, WWW, WW); // strong
      MakePredictionTrend(contextIdx++, WW, WWWWWW, WWWW); 
      MakePredictionTrend(contextIdx++, W, NE, NEE); 
      MakePredictionTrend(contextIdx++, WW, NNEE, NNEEEE); // strong
      MakePredictionTrend(contextIdx++, NW, NWW, W); // very strong
      MakePredictionTrend(contextIdx++, NNWW, NNWWWW, WW);
      MakePredictionTrend(contextIdx++, NN, NNW, W);
//    MakePredictionTrend(contextIdx++, NNNN, NNNNNNWW, NNWW);  // weak
      MakePredictionTrend(contextIdx++, NNNN, NNNNNNEE, NNEE);
      MakePredictionTrend(contextIdx++, NE, NN, NW); // strong
      
      MakePredictionSmooth(contextIdx++, W, NNE, NE); // very strong
      MakePredictionTrend(contextIdx++, NNE, NNNE, NE);
      MakePredictionTrend(contextIdx++, NEE, NNEEE, NEE); // strong
      MakePredictionTrend(contextIdx++, NEEE, NNNEEE, NEEE);
      MakePredictionTrend(contextIdx++, NNE, NN, W);  // strong
      MakePredictionTrend(contextIdx++, NNW, NNWW, W);  // strong
      
      MakePredictionC(contextIdx++, (N * 3 - NN * 3 + NNN));
      MakePredictionC(contextIdx++, (W * 3 - WW * 3 + WWW));
      MakePredictionC(contextIdx++, clamp4(N * 3 - NN * 3 + NNN, N, W, NE, NW)); // very strong
      MakePredictionC(contextIdx++, clamp4(W * 3 - WW * 3 + WWW, N, W, NE, NW)); // strong
      
      MakePredictionC(contextIdx++, ((15 * N - 20 * NN + 15 * NNN - 6 * NNNN + NNNNN + clamp4(4 * W - 6 * NWW + 4 * NNWWW - NNNWWWW, W, NW, N, NN)) / 6)); // very strong
      MakePredictionC(contextIdx++, ((6 * NE - 4 * NNEE + NNNEEE + (4 * W - 6 * NW + 4 * NNW - NNNW)) / 4));
      MakePredictionC(contextIdx++, (((N + 3 * NW) / 4) * 3 - avg(NNW, NNWW) * 3 + (NNNWW * 3 + NNNWWW) / 4));
      MakePredictionC(contextIdx++, ((W * 2 + NW) - (WW + 2 * NWW) + NWWW)); // strong
//    MakePredictionC(contextIdx++, ((W * 2 - NW) + (W * 2 - NWW) + N + NE) / 4);  // weak
//    MakePredictionSmooth(contextIdx++, avg(N, W), N, W);  // weak
      MakePredictionAvg(contextIdx++, NEEEE, NEEEEEE); // strong
      MakePredictionAvg(contextIdx++, WWWW, WWWWWW);
      MakePredictionAvg(contextIdx++, NNNN, NNNNNN);// strong
//    MakePredictionTrend(contextIdx++, NNNN, NNNNNN, NN); // weak
//    MakePredictionC(contextIdx++, avg(NNNNWWWW, NNWW)); // weak
//    MakePredictionC(contextIdx++, avg(NNNNEEEE, NNEE)); // weak
      MakePredictionAvg(contextIdx++, NNNNWW, NNWW);
      MakePredictionAvg(contextIdx++, NNNNEE, NNEE);
//    MakePredictionTrend(contextIdx++, NNEE, NNNNEEEE, NNEE); // weak
//    MakePredictionTrend(contextIdx++, NNEE, NNNNEE, NE); // weak
      MakePredictionAvg(contextIdx++, WWW, WWWWWW);
//    MakePredictionTrend(contextIdx++, WWW, WWWWWW, W);
//    MakePredictionTrend(contextIdx++, WWWW, WWWWWW, WW); // weak
      MakePredictionC(contextIdx++, W);
      MakePredictionC(contextIdx++, N);  // strong
      MakePredictionC(contextIdx++, NN);

      MakePredictionTrend(contextIdx++, W, WW, W); // strong
      MakePredictionTrend(contextIdx++, WW, WWWW, WW); // strong?
      MakePredictionTrend(contextIdx++, N, NN, N); // strong
      MakePredictionTrend(contextIdx++, NN, NNNN, NN);
      MakePredictionTrend(contextIdx++, NW, NNWW, NW);
      MakePredictionTrend(contextIdx++, NNWW, NNNNWWWW, NNWW);
      MakePredictionTrend(contextIdx++, NE, NNEE, NE);
      MakePredictionTrend(contextIdx++, NNEEE, NNNNEEEE, NNEEE);

//    MakePredictionTrend(contextIdx++, NN, NNNW, NW);
      MakePredictionTrend(contextIdx++, W, NEE, NEEE);
//    MakePredictionTrend(contextIdx++, NEE, NNEEE, NE);
      MakePredictionTrend(contextIdx++, NWW, NWWWW, WW);
//    MakePredictionC(contextIdx++, ((W + NW) * 3 - NWW * 6 + NWWW + NNWWW) / 2);
//    MakePredictionTrend(contextIdx++, (NE * 2 + NNE), (NNEE + NNNEE * 2), NNNNEEE);
      MakePredictionC(contextIdx++, (W + N + NEEEEE + NEEEEEEE) / 4);
      MakePredictionTrend(contextIdx++, WW, N, NEE);
//    MakePredictionAvg(contextIdx++, N, NNN);
      MakePredictionTrend(contextIdx++, N, NNWW, NWW);
//    MakePredictionTrend(contextIdx++, (N * 2 + NE), (NN + 2 * NNE), NNNE);
//    MakePredictionTrend(contextIdx++, (NW * 2 + NNW), (NNWW + NNNWW * 2), NNNNWWW);
//    MakePredictionTrend(contextIdx++, (N * 2 + NW), (NN + 2 * NNW), NNNW);

      MakePredictionAvg(contextIdx++, 2 * N - NN, 2 * W - WW);
      MakePredictionAvg(contextIdx++, 2 * W - WW, 2 * NW - NNWW);

      MakePredictionC(contextIdx++, paeth(W, N, NW));
      MakePredictionC(contextIdx++, gap(W, N, NW, NE, WW, NNE, NN));

      // 3rd-order horizontal + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (6 * W - 4 * WW + WWW + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 4);
      // average of 1st- through 4th-order horizontal + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (10 * W - 10 * WW + 5 * WWW - WWWW + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 5);
      // 2nd-order H + weighted blend of 3rd-order NE and 3rd-order NEEE diagonals
      MakePredictionC(contextIdx++, (15 * W - 4 * WW + 10 * (3 * NE - 3 * NNE + NNNE) - (3 * NEEE - 3 * NNEEE + NNNEEE)) / 20); //strong
      // 2× 1st-order + 3× 2nd-order horizontal + 3rd-order NEE diagonal
      MakePredictionC(contextIdx++, (8 * W - 3 * WW + (3 * NEE - 3 * NNEE + NNNEE)) / 6); // very strong

      // 2nd-order H + 2nd-order V, interaction-corrected
      MakePredictionC(contextIdx++, (2 * W - WW) + (2 * N - NN) - (2 * NW - NNWW));

      // 3rd-order horizontal + 4th-order NW diagonal
//    MakePredictionC(contextIdx++, (6 * W - 4 * WW + WWW + 4 * NW - 6 * NNW + 4 * NNNW - NNNNW) / 4); // weak
      // 4th-order horizontal + 4th-order NW diagonal
//    MakePredictionC(contextIdx++, (10 * W - 10 * WW + 5 * WWW - WWWW + 4 * NW - 6 * NNW + 4 * NNNW - NNNNW) / 5); // weak

      // 3rd-order vertical + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (6 * N - 4 * NN + NNN + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 4);
      // 4th-order vertical + 4th-order NE diagonal
      MakePredictionC(contextIdx++, ((10 * N - 10 * NN + 5 * NNN - NNNN) + (4 * NE - 6 * NNE + 4 * NNNE - NNNNE)) / 5);

      // 3rd-order vertical + 4th-order NW diagonal
      MakePredictionC(contextIdx++, (6 * N - 4 * NN + NNN + 4 * NW - 6 * NNW + 4 * NNNW - NNNNW) / 4);
      // 4th-order vertical + 4th-order NW diagonal
      MakePredictionC(contextIdx++, ((10 * N - 10 * NN + 5 * NNN - NNNN) + (4 * NW - 6 * NNW + 4 * NNNW - NNNNW)) / 5);

      // 3rd-order NE diagonal alone
      MakePredictionC(contextIdx++, (6 * NE - 4 * NNEE + NNNEEE) / 3);
      // 3rd-order NW diagonal alone
//    MakePredictionC(contextIdx++, (6 * NW - 4 * NNWW + NNNWWW) / 3); // weak

      // 3rd-order H + 3rd-order V, interaction-corrected
      MakePredictionC(contextIdx++, ((6 * W - 4 * WW + WWW) + (6 * N - 4 * NN + NNN) - (6 * NW - 4 * NNWW + NNNWWW)) / 3);

      // 5th-order horizontal
      MakePredictionC(contextIdx++, (15 * W - 20 * WW + 15 * WWW - 6 * WWWW + WWWWW) / 5); // strong

      // symmetric NE+NW blend, N-corrected
      MakePredictionC(contextIdx++, (2 * NE - NNEE) + (2 * NW - NNWW) - (2 * N - NN));

      // 2nd-order H + 2nd-order V + 2nd-order NE + 2nd-order NW, fully interaction-corrected
//    MakePredictionC(contextIdx++, (2 * W - WW) + (2 * N - NN) - (2 * NE - NNEE));

      // 4th-order horizontal
//    MakePredictionC(contextIdx++, (10 * W - 10 * WW + 5 * WWW - WWWW) / 4);

      // 3rd-order vertical
//    MakePredictionC(contextIdx++, (6 * N - 4 * NN + NNN) / 3);

      // 4th-order vertical
//    MakePredictionC(contextIdx++, (10 * N - 10 * NN + 5 * NNN - NNNN) / 4);
      // 5th-order vertical
//    MakePredictionC(contextIdx++, (15 * N - 20 * NN + 15 * NNN - 6 * NNNN + NNNNN) / 5);

      // 2nd-order NE diagonal
//    MakePredictionC(contextIdx++, (2 * NE - NNEE));

      // 2nd-order NW diagonal
//    MakePredictionC(contextIdx++, (2 * NW - NNWW));
      
      MakePredictionC(contextIdx++, 0);

      assert(contextIdx == nRM);

      //quality metric: quantized past loss in the pixel neighborhood 
      lossQ4 =
        color == 2 ? (lossQ < 1 ? lossQ : min(1 + ((lossQ - 1) / 40u), 7)) :
        color == 1 ? (lossQ < 8 ? (lossQ >> 2) : min(2 + ((lossQ - 8) / 40u), 7)) :
        min(lossQ / 40u, 7); //0..7 (3 bits)

      // map the predictions to histograms

      for (int i = 0; i < nRM; i++) {
        uint32_t spread = predictions[i] >> 16;
        short prediction = predictions[i] & 65535;
        if (prediction == INT16_MAX) { // currently never happens, todo
          mapR1.skip();
          mapR2.skip();
          mapR3.skip();
        }
        else {
          uint32_t predErrAvg = GetPredErrAvg(i);
          mapR1.set(prediction, min(predErrAvg, 31) << 2 | color); // 0..31, 0..3 (5+2 bits)
          mapR2.set(prediction, lossQ4 << 2 | color); // 0..7, 0..3 (3+2 bits)
          mapR3.set(prediction, min(spread, 31) << 2 | color); // 5+2 bits
        }
      }

      // OLS predictors

      int k = (color > 0) ? color - 1 : stride - 1; // previous color index
      for (int j = 0; j < nOLS; j++) {
        ols[j][k]->update(p1);
        auto ols_j_color = ols[j][color].get();
        auto ols_ctx_j = olsCtxs[j];
        for (int ctx_idx = 0; ctx_idx < num[j]; ctx_idx++) {
          float val = *ols_ctx_j[ctx_idx];
          ols_j_color->add(val);
        }
        float pred = ols_j_color->predict();
        short prediction = short(roundf(pred));
        MakePredictionC(contextIdx++, prediction);

        uint32_t predErrAvg = GetPredErrAvg(j + nRM);
        mapOLS1.set(prediction, min(predErrAvg, 31) << 2 | color); // 0..31, 0..3 (5+2 bits)
        mapOLS2.set(prediction, lossQ4 << 2 | color); // 0..7, 0..3 (3+2 bits)
      }

      assert(contextIdx == nRM + nOLS);

      // these contexts are best for non-photographic images (logos, icons, screenshots, infographics)
      // todo: review and optimize these contexts

      uint64_t i = color * 1024;
      const uint8_t R_ = CM_USE_RUN_STATS;

      //for tuning:
      //int z = 0;
      //int toskip = shared->tuning_param + z;
      //z++; if (toskip == z) cm.skip(R_), ++i; else ...

      cm.set(R_, hash(++i, W, p1));
      cm.set(R_, hash(++i, W, p2));
      cm.set(R_, hash(++i, N, p1));
      cm.set(R_, hash(++i, N, p2));
      cm.set(R_, hash(++i, p1, p2)); // strong
      cm.set(R_, hash(++i, N, NN, p1));
      cm.set(R_, hash(++i, N, NN, p2));
      cm.set(R_, hash(++i, W, WW, p1));
      cm.set(R_, hash(++i, W, WW, p2));
      cm.set(R_, hash(++i, N, W, p1, p2));

      cm.set(R_, hash(++i, W, p1 - Wp1, p2 - Wp2)); // strong
      cm.set(R_, hash(++i, N, p1 - Np1, p2 - Np2)); // very strong
      cm.set(R_, hash(++i, NW, p1 - NWp1, p2 - NWp2));
      cm.set(R_, hash(++i, NE, p1 - NEp1, p2 - NEp2));

//    cm.set(R_, hash(++i, N, NN));    //week
//    cm.set(R_, hash(++i, NE, NNEE)); //weak
//    cm.set(R_, hash(++i, NW, NNWW)); //weak
//    cm.set(R_, hash(++i, W, NEE));   //weak
//    cm.set(R_, hash(++i, N, NN, NNN)); //weak
//    cm.set(R_, hash(++i, W, WW, WWW)); //weak

      cm.set(R_, hash(++i, W, WW, N, NN));
      cm.set(R_, hash(++i, W, N, NE, NW));

      cm.set(R_, hash(++i, (NNN + N + 4) >> 3, (N * 3 - NN * 3 + NNN) >> 1));
      cm.set(R_, hash(++i, (W + N - NW) >> 1, (W + p1 - Wp1) >> 3, (W + p2 - Wp2) >> 3, (N + p1 - Np1) >> 3, (N + p2 - Np2) >> 3));
      cm.set(R_, hash(++i, W >> 2, DiffQt(W, p1), DiffQt(W, p2)));
      cm.set(R_, hash(++i, N >> 2, DiffQt(N, p1), DiffQt(N, p2)));
      cm.set(R_, hash(++i, (W + N + 4) >> 3, p1 >> 4, p2 >> 4)); // strong

      cm.set(R_, hash(++i, (N * 2 - NN) >> 1, DiffQt(N, (NN * 2 - NNN))));
      cm.set(R_, hash(++i, (W * 2 - WW) >> 1, DiffQt(W, (WW * 2 - WWW))));
      cm.set(R_, hash(++i, (N * 2 - NN), DiffQt(W, (NW * 2 - NNW))));
      cm.set(R_, hash(++i, (W * 2 - WW), DiffQt(N, (NW * 2 - NWW))));
      cm.set(R_, hash(++i, (W + NEE + 1) >> 1, DiffQt(W, (WW + NE + 1) >> 1)));
      cm.set(R_, hash(++i, (W + NEE - NE), DiffQt(W, (WW + NE - N))));

      int div7 = max((x / stride) >> 10, 7);
      int div17 = max((x / stride) >> 9, 17);
      int div29 = max((x / stride) >> 8, 29);

      cm.set(R_, hash(++i, w, x / stride / div7));
      cm.set(R_, hash(++i, w, x / stride / div17, line / 17));
      cm.set(R_, hash(++i, w, x / stride / div29, (W + N - NW) >> 1));

      cm.set(R_, hash(++i, w, x / stride / div29, W >> 2, NE >> 2));

      assert(i - color * 1024 == nCM);

      ctx[0] =  // 9 bits
        (static_cast<int>(rabs(W, NW) > 3) << 8) |
        (static_cast<int>(rabs(NW, N) > 3) << 7) |
        (static_cast<int>(rabs(N, NE) > 3) << 6) |
        (static_cast<int>(N > NW) << 5) |
        (static_cast<int>(N > NE) << 4) |
        (static_cast<int>(N > NN) >> 3) |
        (static_cast<int>(W > N) << 2) |
        (static_cast<int>(W > NW) << 1) |
        (static_cast<int>(W > WW) << 0);


      ctx[1] =  // 8 bits
        (DiffQt(p1, (Np1 + NEp1 - NNEp1))) << 4 |
        (DiffQt((N + NE - NNE), (N + NW - NNW)));

      shared->State.Image.plane = color;
      shared->State.Image.pixels.W = W;
      shared->State.Image.pixels.N = N;
      shared->State.Image.pixels.NN = NN;
      shared->State.Image.pixels.WW = WW;
      shared->State.Image.ctx = ((color << 9) | ctx[0]) >> 3; // 8 bits
    }
  }

  //for every bit

  if (color != 4) {
    INJECT_SHARED_c0

    //these contexts are best for non-photographic images (logos, icons, screenshots, infographics)
    //but they also work well for modelling with the direct pixel neigborhood in not-too-noisy photographic images

    int i = (c0 << 2 | color) * 256;

    //for tuning:
    //int toskip = shared->tuning_param - 1 + i;
    //if (toskip == i) mapL.skip(), i++; else 

    mapL.set(hash(++i, p2)); // very strong
    mapL.set(hash(++i, p1, p2)); // strong

    // W
//  mapL.set(hash(++i, W, p1)); // weak
    mapL.set(hash(++i, W, p2)); // strong
    mapL.set(hash(++i, W, p1, p2));

    // N
//  mapL.set(hash(++i, N, p1)); // weak
//  mapL.set(hash(++i, N, p2)); // weak
    mapL.set(hash(++i, N, Np1, Np2));

    // W + N
//  mapL.set(hash(++i, W, N, p1, p2));  // weak
    mapL.set(hash(++i, W, p1, p2, N, Np1));
    mapL.set(hash(++i, W, p1, p2, N, Np1, Np2));

    // W + NE
    mapL.set(hash(++i, W, p1, p2, NE));
    mapL.set(hash(++i, W, p1, p2, NE, NEp1));
    mapL.set(hash(++i, W, p1, p2, NE, NEp1, NEp2));


    // N + NW
//  mapL.set(hash(++i, N, NW, p2)); // weak

    // N + NE
    mapL.set(hash(++i, N, NE, p1));
//  mapL.set(hash(++i, N, NE, p2)); // weak

    //N + NN
//  mapL.set(hash(++i, N, NN, p1));  // weak
//  mapL.set(hash(++i, N, NN, p2)); // weak

    // W + WW
//  mapL.set(hash(++i, W, WW, p1)); // weak
//  mapL.set(hash(++i, W, WW, p2)); // weak

    // NW + NE (cross-diagonal)
    mapL.set(hash(++i, NW, NE, p1));
    mapL.set(hash(++i, NW, NE, p2));

    // NW + NN
    mapL.set(hash(++i, NW, NN, p1));

    // W + N + NE
    mapL.set(hash(++i, W, N, NE, p1));

    // W + N + NW
    mapL.set(hash(++i, W, N, NW, p1));
//  mapL.set(hash(++i, W, p1, p2, N, NW, NWp1)); //weak

    // N + NE + NW
    mapL.set(hash(++i, N, Np1, NE, NW, p1));

    mapL.set(hash(++i, NE, NEE, p1)); // strong
    mapL.set(hash(++i, NE, NEE, p2));
    mapL.set(hash(++i, NE, NEp1, NEp2, NEE)); // strong

    mapL.set(hash(++i, NN, NNN, p1));
    mapL.set(hash(++i, NN, NNN, p2));
    mapL.set(hash(++i, NN, NNp1, NNp2, NNN));

//  mapL.set(hash(++i, WW, WWW, p1)); // weak
//  mapL.set(hash(++i, WW, WWW, p2)); // weak
    mapL.set(hash(++i, WW, WWp1, WWp2, WWW));

    mapL.set(hash(++i, Np1, Np2, Wp1, Wp2));
    mapL.set(hash(++i, NWp1, NWp2, NEp1, NEp2));

    mapL.set(hash(++i, (W + N - NW) >> 1, p1));  // very strong
    mapL.set(hash(++i, (W + N - NW) >> 1, p1, p2)); // very strong
    mapL.set(hash(++i, (N + NE - NNE) >> 1, p1)); // strong
    mapL.set(hash(++i, (N + NE - NNE) >> 1, p1, p2));
    mapL.set(hash(++i, (W * 2 - WW) >> 1, p1));
    mapL.set(hash(++i, (N * 2 - NN) >> 1, p1));
    mapL.set(hash(++i, (W * 2 - WW) >> 1, p1, p2));
    mapL.set(hash(++i, (N * 2 - NN) >> 1, p1, p2));

    mapL.set(hash(++i, (NW + W - NWW) >> 1, p1));
    mapL.set(hash(++i, (NW + W - NWW) >> 1, p1, p2));
    mapL.set(hash(++i, (NW + N - NNW) >> 1, p1));
    mapL.set(hash(++i, (NW + N - NNW) >> 1, p1, p2));

//  mapL.set(hash(++i, (NE * 2 - NNEE) >> 1, p1)); 
//  mapL.set(hash(++i, (NE * 2 - NNEE) >> 1, p1, p2));
//  mapL.set(hash(++i, (NW * 2 - NNWW) >> 1, p1));
//  mapL.set(hash(++i, (NW * 2 - NNWW) >> 1, p1, p2)); // weak

//  mapL.set(hash(++i, ((W * 2 - WW) + (N * 2 - NN) - (NW * 2 - NNWW)) >> 1, p1)); // weak
//  mapL.set(hash(++i, ((W * 2 - WW) + (N * 2 - NN) - (NW * 2 - NNWW)) >> 1, p1, p2)); // weak

//  mapL.set(hash(++i, (N * 3 - NN * 3 + NNN) >> 1, p1));
//  mapL.set(hash(++i, (W * 3 - WW * 3 + WWW) >> 1, p1));
//  mapL.set(hash(++i, (N * 3 - NN * 3 + NNN) >> 1, p1, p2)); // weak
//  mapL.set(hash(++i, (W * 3 - WW * 3 + WWW) >> 1, p1, p2)); // weak

//  mapL.set(hash(++i, ((NE * 2 - NNEE) + (NW * 2 - NNWW) - (N * 2 - NN)) >> 1, p1)); // weak
//  mapL.set(hash(++i, ((NE * 2 - NNEE) + (NW * 2 - NNWW) - (N * 2 - NN)) >> 1, p1, p2));

    mapL.set(hash(++i, paeth(W, N, NW) >> 1, p1));
    mapL.set(hash(++i, paeth(W, N, NW) >> 1, p1, p2)); // strong
//  mapL.set(hash(++i, gap(W, N, NW, NE, WW, NNE, NN) >> 1, p1)); // weak

    assert(i - ((c0 << 2 | color) * 256) == nLSM);


  }
}

void Image24BitModel::init() {
  stride = 3 + alpha;
  padding = w % stride;
  x = color = line = 0;
  if( lastPos > 0 && false ) { // todo: when shall we reset?
    for (int i = 0; i < nLSM; i++) {
      mapL.reset();
    }
  }
  lossBuf.setSize(nextPowerOf2(LOSS_BUF_ROWS * w));
  lossBuf.fill(255);

  predErrBuf.setSize(nextPowerOf2(PRED_ERR_BUF_ROWS * w * (nRM + nOLS)));
  predErrBuf.fill(255);

  bestPredictorIndexes.setSize(nextPowerOf2(BEST_PRED_ROWS * w));
}

void Image24BitModel::setParam(int width, uint32_t alpha0) {
  w = width;
  alpha = alpha0;
}

void Image24BitModel::mix(Mixer &m) {

  // predict next bit
  if (color < 4) { // pixel area
    cm.mix(m);

    mapR1.mix(m);
    mapR2.mix(m);
    mapR3.mix(m);
    mapOLS1.mix(m);
    mapOLS2.mix(m);
    mapL.mix(m);

    INJECT_SHARED_bpos
    INJECT_SHARED_c0
    assert(color < 4);

    const int bp = (UINT32_C(0x33322210) >> (bpos << 2)) & 0xF; // {bpos:0}->0  {bpos:1}->1  {bpos:2,3,4}->2  {bpos:5,6,7}->3

    m.set(1 + bpos, 1 + 8);   // 1: account for padding zone

    m.set(lossQ4 /*0..7*/, 8);
    m.set(((line & 7) << 3) | bpos, 8 * 8);

    m.set(((x / stride) & 7) << 3 | (line & 7), 8 * 8);
    m.set(((x / stride) & 15), 16);

    int div3 = max((x / stride) >> 9, 3);
    int div19 = max((x / stride) >> 8, 19);
    m.set((x / stride / div3) & 511, 512);
    m.set((x / stride / div19) & 255, 256);

    int pred1 = 0x100 | ((N + W + 1) >> 1);
    int pred2 = 0x100 | int8_t(N + W - NW);
    int pred3 = 0x100 | int8_t(2 * N - NN);
    int pred4 = 0x100 | int8_t(2 * W - WW);
    m.set(
      (c0 == (pred1 >> (8 - bpos))) << 6 |
      (c0 == (pred2 >> (8 - bpos))) << 5 |
      (c0 == (pred3 >> (8 - bpos))) << 4 |
      (c0 == (pred4 >> (8 - bpos))) << 3 |
      bpos, 16 * 8);
    m.set(lossQ4 << 2 | bp, 8 * 4);
    m.set((ctx[1] << 2) | bp, 1024);
    m.set(ctx_best_direction << 3 | ctx_best_residual, 4 * 8);

    m.set(((x / stride + line) >> 5) & 255, 256);

    auto bestPredictorIndexW =
      color == 0 ? bestPredictorIndexes(stride) : // W, same channel
      bestPredictorIndexes(1);                    // current pixel, prev channel (co-located i.e. within the same pixel as what we predict)
    auto bestPredictorIndexN = bestPredictorIndexes(w);                    // N, same channel

    m.set(bestPredictorIndexW, (nRM + nOLS));
    m.set(bestPredictorIndexN, (nRM + nOLS));

    int bestErrN = GetPredErrAvg(bestPredictorIndexN);
    int bestErrW = GetPredErrAvg(bestPredictorIndexW);
    m.set(min(31, bestErrW) << 5 | min(31, bestErrN), 32 * 32);

    auto bestPredictorIndexNE = bestPredictorIndexes(w + stride); // NW, same channel
    auto bestPredictorIndexNW = bestPredictorIndexes(w - stride); // NE, same channel

    int bestErrNE = GetPredErrAvg(bestPredictorIndexNE);
    int bestErrNW = GetPredErrAvg(bestPredictorIndexNW);
    m.set(min(31, bestErrNE) << 5 | min(31, bestErrNW), 32 * 32);

    uint32_t rabsBits3 = ctx[0] >> 6; // the top 3 rabs bits
    m.set(rabsBits3, 8);
    uint32_t cmpBits6 = ctx[0] & 0x3F; // the 6 comparison bits
    m.set(cmpBits6 << 3 | lossQ4, 64 * 8);

    int trendN = 2 * N - NN;
    int trendW = 2 * W - WW;
    int trendNE = 2 * NE - NNEE;
    int trendNW = 2 * NW - NNWW;

    int trendMin = min(min(min(trendN, trendW), trendNE), trendNW);
    int trendMax = max(max(max(trendN, trendW), trendNE), trendNW);

    m.set((min(trendMax - trendMin, 255) >> 2) << 2 | (bpos >> 1), 64 * 4);

    m.set(((p1 >> 5) << 5) | ((p2 >> 5) << 2) | (bpos >> 1), 8 * 8 * 4);


  }
  else {
    // padding zone
    m.add(-2047);  //predict 0
    m.set(0, MIXERCONTEXTS);
  }
}

// ===== src/model/Image4BitModel.cpp =====


Image4BitModel::Image4BitModel(const Shared* const sh, const uint64_t size) : shared(sh),
  rnd(),
  hashTable(size >> 6),
  mask(uint32_t(hashTable.size() - 1)),
  hashBits(ilog2(mask + 1)),
  hashes(S),
  sm {sh, S, 256, 1023, StateMapType::BitHistory},
  mapL (sh, S, 22)
{}

void Image4BitModel::setParam(int widthInBytes) {
  w = widthInBytes;
}

void Image4BitModel::update() {
  INJECT_SHARED_y
  for( int i = 0; i < S; i++ ) {
    StateTable::update(cp[i], y, rnd);
  }
  INJECT_SHARED_bpos
  if (bpos == 0 || bpos == 4) {
    col++;
    if (col == w * 2) {
      col = 0;
      line++;
    }
  }
}

void Image4BitModel::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);

  INJECT_SHARED_bpos
  INJECT_SHARED_c0
  if( bpos == 0 || bpos == 4 ) {
    WW = W;
    NWW = NW;
    NW = N;
    N = NE;
    NE = NEE;
    NNWW = NWW;
    NNW = NN;
    NN = NNE;
    NNE = NNEE;
    INJECT_SHARED_buf
    if( bpos == 0 ) {
      INJECT_SHARED_c1
      W = c1 & 0xF;
      NEE = buf(w - 1) >> 4;
      NNEE = buf(w * 2 - 1) >> 4;
    } else {
      W = c0 & 0xF;
      NEE = buf(w - 1) & 0xF;
      NNEE = buf(w * 2 - 1) & 0xF;
    }
    run = (W != WW || col == 0) ? (prevColor = WW, 0) : min(0xFFF, run + 1);
  }

  const uint32_t msb = 1 << (bpos & 3); //bpos = {0,1,2,3 / 4,5,6,7} -> 1,2,4,8
  const uint32_t px = msb | (c0 & (msb - 1)); // 0001, 001x, 01xx, 1xxx

  uint64_t i = 0;
  uint64_t j = static_cast<uint64_t>(px * 256);

  hashes[i++] = hash(++j, W, NW, N);
  hashes[i++] = hash(++j, N, min(0xFFF, col / 8));
  hashes[i++] = hash(++j, W, NW, N, NN, NE);
  hashes[i++] = hash(++j, W, N, NE + NNE * 16, NEE + NNEE * 16);
  hashes[i++] = hash(++j, W, N, NW + NNW * 16, NWW + NNWW * 16);
  hashes[i++] = hash(++j, W, ilog2(run + 1), prevColor, col / max(1, w / 2));
  hashes[i++] = hash(++j, NE, min(0x3FF, (col + line) / max(1, w * 8)));
  hashes[i++] = hash(++j, NW, (col - line) / max(1, w * 8));
  hashes[i++] = hash(++j, WW * 16 + W, NN * 16 + N, NNWW * 16 + NW);
  hashes[i++] = hash(++j, N, NN);
  hashes[i++] = hash(++j, W, WW);
  hashes[i++] = hash(++j, W, NE);
  hashes[i++] = hash(++j, WW, NN, NEE);
  hashes[i++] = hash(++j);

  assert(i == S);
  assert(j == px * 256 + S);
     
  for (size_t i = 0; i < S; i++) {
    uint64_t contexthash = hashes[i];
    mapL.set(contexthash);
    const uint32_t ctx = finalize64(contexthash, hashBits);
    const uint16_t chk = checksum16(contexthash, hashBits);
    cp[i] = &hashTable[ctx].find(chk, &rnd)->bitState;
  }

  mapL.mix(m);

  sm.subscribe();
  for( int i = 0; i < S; i++ ) {
    const uint8_t state = *cp[i];
    const int n0 = StateTable::getNextState(state, 2);
    const int n1 = StateTable::getNextState(state, 3);
    const int p1 = sm.p2(i, state);
    const int st = stretch(p1) >> 1;
    m.add(st);
    m.add((p1 - 2048) >> 2);
    const int bitIsUncertain = int(n0 != 0 && n1 != 0);
    m.add((bitIsUncertain - 1) & st); // when both counts are nonzero add(0) otherwise add(st)
  }

  m.set((W << 4) | px, 256);
  m.set(min(31, col / max(1, w / 16)) | (N << 5), 512);
  m.set((bpos & 3) | (W << 2) | (min(7, ilog2(run + 1)) << 6), 512);
  m.set(W | (NE << 4) | ((bpos & 3) << 8), 1024);
  m.set(px, 16);
  m.set(0, 1);
}


// ===== src/model/Image8BitModel.cpp =====
#include <cmath> // round


Image8BitModel::Image8BitModel(Shared* const sh, const uint64_t size) :
  shared(sh),
  cm(sh, size, nCM, 64),
  map{ /* StationaryMap : BitsOfContext, InputBits, Scale=64, Rate=16  */
    /*nSM0: 0- 1*/ {sh, 0,8}, {sh,15,1},
  },
  mapR1{ sh, nRM, 1 << 5, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapR2{ sh, nRM, 1 << 3, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapR3{ sh, nRM, 1 << 5, 74 }, /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapOLS1{ sh, nOLS, 1 << 5, 74 },  /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  mapOLS2{ sh, nOLS, 1 << 3, 74 },  /* ResidualMap: numContexts, histogramsPerContext, scale=64 */
  pltMap{   /* SmallStationaryContextMap: BitsOfContext, InputBits, Rate, Scale */
    {sh,11,1,7,64}, {sh,11,1,7,64}, {sh,11,1,7,64}, {sh,11,1,7,64}
  },
  sceneMap{ /* IndirectMap: BitsOfContext, InputBits, Scale, Limit */
    {sh,8,8,64,255}, {sh,8,8,64,255}, {sh,22,1,64,255}, {sh,11,1,64,255}, {sh,11,1,64,255}
  },
  iCtx{     /* IndirectContext<U8>: BitsPerContext, InputBits */
    {16,8}, {16,8}, {16,8}, {16,8}
  } {
  for (int i = 0; i < nOLS; i++) {
    ols[i] = create_OLS_float(sh->chosenSimd, num[i], 1, lambda[i], nu);
  }

  sceneOls = create_OLS_float(sh->chosenSimd, 13, 1, 0.994f, nu);
}

ALWAYS_INLINE uint8_t Image8BitModel::Ls(int relX, int relY) const {
  if (line - relY < 0)  //the lossBuf buffer is pre-filled with 255 initially, so this check is not really necessary
    return 255;
  if (x - relX < 0)
    return 255;
  if (x - relX >= w)
    return 255;
  int offset = relY * w + relX;
  const uint32_t valuesPerByte = 1;
  return lossBuf((offset - 1) * valuesPerByte + 1);
}

ALWAYS_INLINE uint8_t Image8BitModel::GetPredErr(const uint32_t ctxIndex, int relX, int relY) const {
  if (line - relY < 0)  //the predErrBuf buffer is pre-filled with 255 initially, so this check is not really necessary
    return 255;
  if (x - relX < 0)
    return 255;
  if (x - relX >= w)
    return 255;
  int offset = relY * w + relX;
  const uint32_t valuesPerByte = (nRM + nOLS);
  return predErrBuf((offset - 1) * valuesPerByte + ctxIndex + 1);
}

ALWAYS_INLINE uint32_t Image8BitModel::GetPredErrAvg(const uint32_t predictorIndex) const {
  // Current pixel's prediction confidence is based on the already known error at neighboring pixel predictions
  uint32_t predErrW = GetPredErr(predictorIndex, 1, 0);
  uint32_t predErrN = GetPredErr(predictorIndex, 0, 1);
  uint32_t predErrNW = GetPredErr(predictorIndex, 1, 1);
  uint32_t predErrNE = GetPredErr(predictorIndex, -1, 1);
  uint32_t predErrWW = GetPredErr(predictorIndex, 2, 0);
  uint32_t predErrNN = GetPredErr(predictorIndex, 0, 2);
  uint32_t predErrAvg = (2 * predErrW + 2 * predErrN + predErrNE + predErrNW + predErrWW + predErrNN) >> 3; // 0..255
  return predErrAvg;
}

ALWAYS_INLINE static int avg(int x, int y) {
  return (x + y + 1) >> 1;  //note: rounding here works properly only when x+y is non-negative, but we don't really need the function to be aware of negative values as they are rare
}

// Stores a prediction with a spread signal based on the distance between two reference pixels.
// ref1 and ref2 are spatial reference pixels; their difference measures local variation -
// higher difference = less confident = used as a context component in the probability map.
// Use when the prediction value is computed externally (e.g. a complex algebraic expression)
// but a natural pair of reference pixels still exists to supply the spread signal.
ALWAYS_INLINE void Image8BitModel::MakePrediction(int i, uint8_t ref1, uint8_t ref2, int prediction) {
  uint32_t absdiff = rabs(ref1, ref2);
  predictions[i] = absdiff << 16 | ((prediction) & 65535);
}

// Trend extrapolation: continues the gradient observed from pxFar to px,
// starting from the spatial origin.
// prediction = origin + (px - pxFar)
// spread = rabs(px - pxFar): larger gradient = more uncertain prediction.
// Use when pixels are expected to follow a consistent directional trend
// (e.g. continuing a horizontal, vertical, or diagonal gradient).
ALWAYS_INLINE void Image8BitModel::MakePredictionTrend(int i, int px, int pxFar, int origin) {
  uint32_t absdiff = rabs(px, pxFar);
  int prediction = origin + px - pxFar;
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

// Smoothed trend: applies the gradient at half strength instead of full extrapolation.
// prediction = avg(origin, origin + (px - pxFar))
// spread = rabs(px - pxFar): same signal as Trend.
// Use in smoother regions where a full trend extrapolation would overshoot,
// or when the gradient is expected to taper rather than continue linearly.
ALWAYS_INLINE void Image8BitModel::MakePredictionSmooth(int i, int px, int pxFar, int origin) {
  uint32_t absdiff = rabs(px, pxFar);
  int prediction = avg(origin, origin + px - pxFar);
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

ALWAYS_INLINE void Image8BitModel::MakePredictionAvg(int i, int px1, int px2) {
  uint32_t absdiff = rabs(px1, px2);
  int prediction = avg(px1, px2);
  predictions[i] = absdiff << 16 | (prediction & 65535);
}

// Stores only a prediction value; the spread signal is absent (upper 16 bits are zero).
// Use for complex algebraic combinations of multiple pixels where no single
// natural pair of reference pixels exists to derive a meaningful spread signal.
ALWAYS_INLINE void Image8BitModel::MakePredictionC(int i, int prediction) {
  predictions[i] = prediction & 65535;
}

void Image8BitModel::init(int pos) {
  x = line = jump = 0;
  columns[0] = max(1, w / max(1, ilog2(w) * 2));
  columns[1] = max(1, columns[0] / max(1, ilog2(columns[0])));
  if (isGray) {
    if (lastPos != 0 && false) { // todo: when shall we reset ?
      for (int i = 0; i < nSM; i++) {
        map[i].reset();
      }
    }
  }
  else if (frameWidth != w) {
    for (int i = 0; i < nPltMaps; i++) {
      iCtx[i].reset();
      pltMap[i].reset();
    }
  }
  prevFramePos = framePos;
  framePos = pos;
  prevFrameWidth = frameWidth;
  frameWidth = w;

  lossBuf.setSize(nextPowerOf2(LOSS_BUF_ROWS * w));
  lossBuf.fill(255);

  predErrBuf.setSize(nextPowerOf2(PRED_ERR_BUF_ROWS * w * (nRM + nOLS)));
  predErrBuf.fill(255);
}

void Image8BitModel::setParam(int width, uint32_t isGray) {
  this->w = width;
  this->isGray = isGray;
}

void Image8BitModel::mix(Mixer& m) {

  loss += shared->State.loss; // += 0..1023

  INJECT_SHARED_bpos
  if (bpos == 0) {
    INJECT_SHARED_pos
    if (pos != lastPos + 1) {
      init(pos);
    }
    else {
      x++;
      if (x >= w) {
        x = 0;
        line++;
      }
    }
    lastPos = pos;

    INJECT_SHARED_buf
    INJECT_SHARED_c1
    if (x == 0) {
      memset(&jumps[0], 0, sizeof(short) * jumps.size());
      if (line > 0 && w > 8) {
        uint8_t bMask = 0xFF - ((1 << isGray) - 1);
        uint32_t pMask = bMask * 0x01010101;
        uint32_t left = 0;
        uint32_t right = 0;
        int l = min(w, static_cast<int>(jumps.size()));
        int end = l - 4;
        do {
          left = ((buf(l - x) << 24) | (buf(l - x - 1) << 16) | (buf(l - x - 2) << 8) | buf(l - x - 3)) & pMask;
          int i = end;
          while (i >= x + 4) {
            right = ((buf(l - i - 3) << 24) | (buf(l - i - 2) << 16) | (buf(l - i - 1) << 8) | buf(l - i)) & pMask;
            if (left == right) {
              int j = (i + 3 - x - 1) / 2;
              int k = 0;
              for (; k <= j; k++) {
                if (k < 4 || (buf(l - x - k) & bMask) == (buf(l - i - 3 + k) & bMask)) {
                  jumps[x + k] = -(x + (l - i - 3) + 2 * k);
                  jumps[i + 3 - k] = i + 3 - x - 2 * k;
                }
                else {
                  break;
                }
              }
              x += k;
              end -= k;
              break;
            }
            i--;
          }
          x++;
          if (x > end) {
            break;
          }
        } while (x + 4 < l);
        x = 0;
      }
    }

    column[0] = x / columns[0];
    column[1] = x / columns[1];

    WWWWW = buf(5);
    WWWW = buf(4);
    WWW = buf(3);
    WW = buf(2);
    W = buf(1);
    NWWWW = buf(w + 4);
    NWWW = buf(w + 3);
    NWW = buf(w + 2);
    NW = buf(w + 1);
    N = buf(w);
    NE = buf(w - 1);
    NEE = buf(w - 2);
    NEEE = buf(w - 3);
    NEEEE = buf(w - 4);
    NNWWW = buf(w * 2 + 3);
    NNWW = buf(w * 2 + 2);
    NNW = buf(w * 2 + 1);
    NN = buf(w * 2);
    NNE = buf(w * 2 - 1);
    NNEE = buf(w * 2 - 2);
    NNEEE = buf(w * 2 - 3);
    NNNWW = buf(w * 3 + 2);
    NNNW = buf(w * 3 + 1);
    NNN = buf(w * 3);
    NNNE = buf(w * 3 - 1);
    NNNEE = buf(w * 3 - 2);
    NNNNW = buf(w * 4 + 1);
    NNNN = buf(w * 4);
    NNNNE = buf(w * 4 - 1);
    NNNNN = buf(w * 5);
    NNNNNN = buf(w * 6);

    uint8_t WWWWWW = buf(6);
    uint8_t NNWWWW = buf(2 * w + 4);
    uint8_t NNNWWW = buf(3 * w + 3);
    uint8_t NNNWWWW = buf(3 * w + 4);
    uint8_t NNNNWW = buf(4 * w + 2);
    uint8_t NNNNWWW = buf(4 * w + 3);
    uint8_t NNNNWWWW = buf(4 * w + 4);
    uint8_t NEEEEE = buf(1 * w - 5);
    uint8_t NEEEEEE = buf(1 * w - 6);
    uint8_t NNEEEE = buf(2 * w - 4);
    uint8_t NNNEEE = buf(3 * w - 3);
    uint8_t NNNEEEE = buf(3 * w - 4);
    uint8_t NNNNEEE = buf(4 * w - 3);
    uint8_t NNNNEE = buf(4 * w - 2);
    uint8_t NNNNEEEE = buf(4 * w - 4);
    uint8_t NNNNNNEE = buf(6 * w - 2);
    uint8_t NNNWWWWW = buf(3 * w + 5);
    uint8_t NEEEEEEE = buf(1 * w - 7);

    if (prevFramePos > 0 && prevFrameWidth == w) {
      int offset = prevFramePos + line * w + x;
      prvFrmPx = buf[offset];
      if (isGray != 0) {
        sceneOls->update((float)W);
        sceneOls->add((float)W);
        sceneOls->add((float)NW);
        sceneOls->add((float)N);
        sceneOls->add((float)NE);
        for (int i = -1; i < 2; i++) {
          for (int j = -1; j < 2; j++) {
            sceneOls->add((float)buf[offset + i * w + j]);
          }
        }
        float prediction = sceneOls->predict();
        prvFrmPrediction = clip(int(roundf(prediction)));
      }
      else {
        prvFrmPrediction = W;
      }
    }
    else {
      prvFrmPx = prvFrmPrediction = W;
    }
    sceneMap[0].setDirect(prvFrmPx);
    sceneMap[1].setDirect(prvFrmPrediction);

    jump = jumps[min(x, static_cast<int>(jumps.size()) - 1)];
    uint64_t i = isGray * 1024;
    const uint8_t R_ = CM_USE_RUN_STATS;
    cm.set(R_, hash(++i, (jump != 0) ? (0x100 | buf(abs(jump))) * (1 - 2 * static_cast<int>(jump < 0)) : N, line & 3));
    if (!isGray) {
      for (int j = 0; j < nPltMaps; j++) {
        iCtx[j] += W;
      }
      iCtx[0] = W | (NE << 8);
      iCtx[1] = W | (N << 8);
      iCtx[2] = W | (WW << 8);
      iCtx[3] = N | (NN << 8);
//    cm.set(R_, hash(++i, W)); // ineffective, covered by NormalModel
      cm.set(R_, hash(++i, W, column[0]));
      cm.set(R_, hash(++i, N));
      cm.set(R_, hash(++i, N, column[0]));
      cm.set(R_, hash(++i, NW));
      cm.set(R_, hash(++i, NW, column[0]));
      cm.set(R_, hash(++i, NE));
      cm.set(R_, hash(++i, NE, column[0]));
      cm.set(R_, hash(++i, NWW));
      cm.set(R_, hash(++i, NEE));
      cm.set(R_, hash(++i, WW));
      cm.set(R_, hash(++i, NN));
      cm.set(R_, hash(++i, W, N));
      cm.set(R_, hash(++i, W, NW));
      cm.set(R_, hash(++i, W, NE));
      cm.set(R_, hash(++i, W, NEE));
      cm.set(R_, hash(++i, W, NWW));
      cm.set(R_, hash(++i, N, NW));
      cm.set(R_, hash(++i, N, NE));
      cm.set(R_, hash(++i, NW, NE));
//    cm.set(R_, hash(++i, W, WW)); // ineffective, covered by NormalModel
      cm.set(R_, hash(++i, N, NN));
      cm.set(R_, hash(++i, NW, NNWW));
      cm.set(R_, hash(++i, NE, NNEE));
      cm.set(R_, hash(++i, NW, NWW));
      cm.set(R_, hash(++i, NW, NNW));
      cm.set(R_, hash(++i, NE, NEE));
      cm.set(R_, hash(++i, NE, NNE));
      cm.set(R_, hash(++i, N, NNW));
      cm.set(R_, hash(++i, N, NNE));
      cm.set(R_, hash(++i, N, NNN));
      cm.set(R_, hash(++i, W, WWW));
      cm.set(R_, hash(++i, WW, NEE));
      cm.set(R_, hash(++i, WW, NN));
      cm.set(R_, hash(++i, W, NEEE));
      cm.set(R_, hash(++i, W, NEEEE));
      cm.set(R_, hash(++i, W, N, NW));
      cm.set(R_, hash(++i, N, NN, NNN));
      cm.set(R_, hash(++i, W, NE, NEE));
      cm.set(R_, hash(++i, W, NW, N, NE));
      cm.set(R_, hash(++i, N, NE, NN, NNE));
      cm.set(R_, hash(++i, N, NW, NNW, NN));
      cm.set(R_, hash(++i, W, WW, NWW, NW));
      cm.set(R_, hash(++i, W, NW << 8 | N, WW << 8 | NWW));
      cm.set(R_, hash(++i, column[0]));
      cm.set(R_, hash(++i, N, column[1]));
      cm.set(R_, hash(++i, W, column[1]));
      for (int j = 0; j < nPltMaps; j++) {
        cm.set(R_, hash(++i, iCtx[j]()));
      }

      ctx = min(0x1F, x / min(0x20, columns[0]));
      res = W;
    }
    else { // gray

      lossBuf.add(static_cast<uint8_t>(min(loss >> 2, 255)));  // 0..255
      loss = 0;

      //what was the total cost at the neighboring pixes
      lossQ = //0 x 6 .. 255 x 6 = 0 .. 1530
        Ls(1, 0) + // W
        Ls(0, 1) + // N
        Ls(2, 0) + // WW
        Ls(0, 2) + // NN
        Ls(1, 1) + // NW
        Ls(-1, 1); // NE

      // let's trim the higher part (640-1530) - it is almost always completely empty OR such high values indicate that we are off frame
      // cap at 639 = 16*40 - 1, so lossQ4 = lossQ/40 fits in [0..15]
      lossQ = min(lossQ, 639);
      shared->State.Image.lossQ = lossQ; //0..639

      for (int i = nRM + nOLS - 1; i >= 0; i--) {
        short prediction = predictions[i] & 65535;
        uint8_t err;
        if (prediction == INT16_MAX) // currently never happens, todo
          err = 255;
        else
          err = rabs(c1, prediction); // 0..128
        predErrBuf.add(err); //0..63 or 255
      }
      int contextIdx = 0;



      MakePredictionC(contextIdx++, (N * 3 + W * 3 - NN - WW + 2) >> 2);
      MakePredictionAvg(contextIdx++, W, NEE);

      MakePredictionTrend(contextIdx++, W, NW, N); // very strong
      MakePredictionTrend(contextIdx++, WW, NNWW, NN); //strong?
      MakePredictionTrend(contextIdx++, WWW, NNNWWW, NNN); //strong?
      MakePredictionTrend(contextIdx++, W, N, NE); // strong
      MakePredictionTrend(contextIdx++, N, NNW, NW);
      MakePredictionTrend(contextIdx++, N, NNE, NE);
      MakePredictionTrend(contextIdx++, NN, NNNNEE, NNEE);
      MakePredictionTrend(contextIdx++, N, NNN, NN);
      MakePredictionTrend(contextIdx++, NN, NNNNNN, NNNN);
      MakePredictionTrend(contextIdx++, W, WWW, WW); // strong
      MakePredictionTrend(contextIdx++, WW, WWWWWW, WWWW);
      MakePredictionTrend(contextIdx++, W, NE, NEE);
      MakePredictionTrend(contextIdx++, WW, NNEE, NNEEEE); // strong
      MakePredictionTrend(contextIdx++, NW, NWW, W); // very strong
      MakePredictionTrend(contextIdx++, NNWW, NNWWWW, WW);
      MakePredictionTrend(contextIdx++, NN, NNW, W);
      MakePredictionTrend(contextIdx++, NNNN, NNNNNNEE, NNEE);
      MakePredictionTrend(contextIdx++, NE, NN, NW); // strong

      MakePredictionSmooth(contextIdx++, W, NNE, NE); // very strong
      MakePredictionTrend(contextIdx++, NNE, NNNE, NE);
      MakePredictionTrend(contextIdx++, NEE, NNEEE, NEE); // strong
      MakePredictionTrend(contextIdx++, NEEE, NNNEEE, NEEE);
      MakePredictionTrend(contextIdx++, NNE, NN, W);  // strong
      MakePredictionTrend(contextIdx++, NNW, NNWW, W);  // strong

      MakePredictionC(contextIdx++, (N * 3 - NN * 3 + NNN));
      MakePredictionC(contextIdx++, (W * 3 - WW * 3 + WWW));
      MakePredictionC(contextIdx++, clamp4(N * 3 - NN * 3 + NNN, N, W, NE, NW)); // very strong
      MakePredictionC(contextIdx++, clamp4(W * 3 - WW * 3 + WWW, N, W, NE, NW)); // strong

      MakePredictionC(contextIdx++, ((15 * N - 20 * NN + 15 * NNN - 6 * NNNN + NNNNN + clamp4(4 * W - 6 * NWW + 4 * NNWWW - NNNWWWW, W, NW, N, NN)) / 6)); // very strong
      MakePredictionC(contextIdx++, ((6 * NE - 4 * NNEE + NNNEEE + (4 * W - 6 * NW + 4 * NNW - NNNW)) / 4));
      MakePredictionC(contextIdx++, (((N + 3 * NW) / 4) * 3 - avg(NNW, NNWW) * 3 + (NNNWW * 3 + NNNWWW) / 4));
      MakePredictionC(contextIdx++, ((W * 2 + NW) - (WW + 2 * NWW) + NWWW)); // strong
      MakePredictionAvg(contextIdx++, NEEEE, NEEEEEE); // strong
      MakePredictionAvg(contextIdx++, WWWW, WWWWWW);
      MakePredictionAvg(contextIdx++, NNNN, NNNNNN);// strong
      MakePredictionAvg(contextIdx++, NNNNWW, NNWW);
      MakePredictionAvg(contextIdx++, NNNNEE, NNEE);
      MakePredictionAvg(contextIdx++, WWW, WWWWWW);
      MakePredictionC(contextIdx++, W);
      MakePredictionC(contextIdx++, N);  // strong
      MakePredictionC(contextIdx++, NN);

      MakePredictionTrend(contextIdx++, W, WW, W); // strong
      MakePredictionTrend(contextIdx++, WW, WWWW, WW); // strong?
      MakePredictionTrend(contextIdx++, N, NN, N); // strong
      MakePredictionTrend(contextIdx++, NN, NNNN, NN);
      MakePredictionTrend(contextIdx++, NW, NNWW, NW);
      MakePredictionTrend(contextIdx++, NNWW, NNNNWWWW, NNWW);
      MakePredictionTrend(contextIdx++, NE, NNEE, NE);
      MakePredictionTrend(contextIdx++, NNEEE, NNNNEEEE, NNEEE);

      //MakePredictionTrend(contextIdx++, NN, NNNW, NW);
      MakePredictionTrend(contextIdx++, W, NEE, NEEE);
      //MakePredictionTrend(contextIdx++, NEE, NNEEE, NE);
      MakePredictionTrend(contextIdx++, NWW, NWWWW, WW);
      //MakePredictionC(contextIdx++, ((W + NW) * 3 - NWW * 6 + NWWW + NNWWW) / 2);
      //MakePredictionTrend(contextIdx++, (NE * 2 + NNE), (NNEE + NNNEE * 2), NNNNEEE);
      MakePredictionC(contextIdx++, (W + N + NEEEEE + NEEEEEEE) / 4);
      MakePredictionTrend(contextIdx++, WW, N, NEE);
      //MakePredictionAvg(contextIdx++, N, NNN);
      MakePredictionTrend(contextIdx++, N, NNWW, NWW); //strong
      //MakePredictionTrend(contextIdx++, (N * 2 + NE), (NN + 2 * NNE), NNNE);
      //MakePredictionTrend(contextIdx++, (NW * 2 + NNW), (NNWW + NNNWW * 2), NNNNWWW);
      //MakePredictionTrend(contextIdx++, (N * 2 + NW), (NN + 2 * NNW), NNNW);

      MakePredictionAvg(contextIdx++, 2 * N - NN, 2 * W - WW);
      MakePredictionAvg(contextIdx++, 2 * W - WW, 2 * NW - NNWW);

      MakePredictionC(contextIdx++, paeth(W, N, NW));
      MakePredictionC(contextIdx++, gap(W, N, NW, NE, WW, NNE, NN));

      // 3rd-order horizontal + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (6 * W - 4 * WW + WWW + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 4);
      // average of 1st- through 4th-order horizontal + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (10 * W - 10 * WW + 5 * WWW - WWWW + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 5);
      // 2nd-order H + weighted blend of 3rd-order NE and 3rd-order NEEE diagonals
      MakePredictionC(contextIdx++, (15 * W - 4 * WW + 10 * (3 * NE - 3 * NNE + NNNE) - (3 * NEEE - 3 * NNEEE + NNNEEE)) / 20); //strong
      // 2× 1st-order + 3× 2nd-order horizontal + 3rd-order NEE diagonal
      MakePredictionC(contextIdx++, (8 * W - 3 * WW + (3 * NEE - 3 * NNEE + NNNEE)) / 6); // very strong

      // 2nd-order H + 2nd-order V, interaction-corrected
      MakePredictionC(contextIdx++, (2 * W - WW) + (2 * N - NN) - (2 * NW - NNWW));

      // 3rd-order vertical + 4th-order NE diagonal
      MakePredictionC(contextIdx++, (6 * N - 4 * NN + NNN + 4 * NE - 6 * NNE + 4 * NNNE - NNNNE) / 4);
      // 4th-order vertical + 4th-order NE diagonal
      MakePredictionC(contextIdx++, ((10 * N - 10 * NN + 5 * NNN - NNNN) + (4 * NE - 6 * NNE + 4 * NNNE - NNNNE)) / 5);

      // 3rd-order vertical + 4th-order NW diagonal
      MakePredictionC(contextIdx++, (6 * N - 4 * NN + NNN + 4 * NW - 6 * NNW + 4 * NNNW - NNNNW) / 4);
      // 4th-order vertical + 4th-order NW diagonal
      MakePredictionC(contextIdx++, ((10 * N - 10 * NN + 5 * NNN - NNNN) + (4 * NW - 6 * NNW + 4 * NNNW - NNNNW)) / 5);

      // 3rd-order NE diagonal alone
      MakePredictionC(contextIdx++, (6 * NE - 4 * NNEE + NNNEEE) / 3);

      // 3rd-order H + 3rd-order V, interaction-corrected
      MakePredictionC(contextIdx++, ((6 * W - 4 * WW + WWW) + (6 * N - 4 * NN + NNN) - (6 * NW - 4 * NNWW + NNNWWW)) / 3);

      // 5th-order horizontal
      MakePredictionC(contextIdx++, (15 * W - 20 * WW + 15 * WWW - 6 * WWWW + WWWWW) / 5); // strong

      // symmetric NE+NW blend, N-corrected
      MakePredictionC(contextIdx++, (2 * NE - NNEE) + (2 * NW - NNWW) - (2 * N - NN));

      MakePredictionC(contextIdx++, 0);

      assert(contextIdx == nRM);

      //quality metric: quantized past loss in the pixel neighborhood 
      lossQ4 = min(lossQ / 40u, 7); //0..7 (3 bits)

      for (int i = 0; i < nRM; i++) {
        uint32_t spread = predictions[i] >> 16;
        short prediction = predictions[i] & 65535;
        if (prediction == INT16_MAX) { // currently never happens, todo
          mapR1.skip();
          mapR2.skip();
          mapR3.skip();
        }
        else {
          uint32_t predErrAvg = GetPredErrAvg(i);
          mapR1.set(prediction, min(predErrAvg, 31)); // 0..31 (5 bits)
          mapR2.set(prediction, lossQ4); // 0..7 (3 bits)
          mapR3.set(prediction, min(spread, 31)); // 5 bits
        }
      }

      for (int j = 0; j < nOLS; j++) {
        auto ols_j = ols[j].get();
        ols_j->update((float)W);
        auto ols_ctx_j = olsCtxs[j];
        for (int ctx_idx = 0; ctx_idx < num[j]; ctx_idx++) {
          float val = *ols_ctx_j[ctx_idx];
          ols_j->add(val);
        }

        float pred = ols_j->predict();
        short prediction = short(roundf(pred));
        predictions[nRM + j] = clip(prediction);
        uint32_t predErrAvg = GetPredErrAvg(nRM + j);
        mapOLS1.set(prediction, min(predErrAvg, 31));
        mapOLS2.set(prediction, lossQ4);
      }

      //cm.set(R_, hash(++i, W)); // ineffective, covered by NormalModel
      cm.set(R_, hash(++i, N)); 
      cm.set(R_, hash(++i, NW));
      cm.set(R_, hash(++i, NE));

      cm.set(R_, hash(++i, N, NN));
      cm.set(R_, hash(++i, NE, NNEE));
      cm.set(R_, hash(++i, NW, NNWW));
      cm.set(R_, hash(++i, W, NEE));

      cm.set(R_, hash(++i, N, NN, NNN));
      //cm.set(R_, hash(++i, W, WW, WWW)); // ineffective, covered by NormalModel

      cm.set(R_, hash(++i, W, WW, N, NN));
      cm.set(R_, hash(++i, W, N, NE, NW));

      cm.set(R_, hash(++i, (NNN + N + 4) >> 3, (N * 3 - NN * 3 + NNN) >> 1));

      cm.set(R_, hash(++i, (N * 2 - NN) >> 1, DiffQt(N, (NN * 2 - NNN))));
      cm.set(R_, hash(++i, (W * 2 - WW) >> 1, DiffQt(W, (WW * 2 - WWW))));
      cm.set(R_, hash(++i, (N * 2 - NN), DiffQt(W, (NW * 2 - NNW))));
      cm.set(R_, hash(++i, (W * 2 - WW), DiffQt(N, (NW * 2 - NWW))));
      cm.set(R_, hash(++i, (W + NEE + 1) >> 1, DiffQt(W, (WW + NE + 1) >> 1)));
      cm.set(R_, hash(++i, (W + NEE - NE), DiffQt(W, (WW + NE - N))));

      int div7 = max((x) >> 10, 7);
      int div17 = max((x) >> 9, 17);
      int div29 = max((x) >> 8, 29);

      cm.set(R_, hash(++i, w, x / div7));
      cm.set(R_, hash(++i, w, x / div17, line / 17));
      cm.set(R_, hash(++i, w, x / div29, uint8_t(W + N - NW) >> 1));

      cm.set(R_, hash(++i, w, W >> 2, NE >> 2, x / div29));

      ctx = min(0x1F, x / max(1, w / min(32, columns[0]))) |
        (((static_cast<int>(abs(W - N) * 16 > W + N) << 1) | static_cast<int>(abs(N - NW) > 8)) << 5) | ((W + N) & 0x180);

      res = clamp4(W + N - NW, W, NW, N, NE);
    }

    assert(i - isGray * 1024 <= nCM);

    shared->State.Image.pixels.W = W;
    shared->State.Image.pixels.N = N;
    shared->State.Image.pixels.NN = NN;
    shared->State.Image.pixels.WW = WW;
    shared->State.Image.ctx = ctx >> isGray;
  }
  INJECT_SHARED_c0
  uint8_t b = (c0 << (8 - bpos));
  if (isGray) {
    int i = 0;
    map[i++].set(0);
    map[i++].set(((static_cast<uint8_t>(clip(W + N - NW) - b)) * 8 + bpos) |
      (DiffQt(clip(N + NE - NNE), clip(N + NW - NNW)) << 11));
  }
  sceneMap[2].setDirect(finalize64(hash(x, line), 19) * 8 + bpos);
  sceneMap[3].setDirect((prvFrmPx - b) * 8 + bpos);
  sceneMap[4].setDirect((prvFrmPrediction - b) * 8 + bpos);

  // predict next bit
  cm.mix(m);
  if (isGray) {
    for (int i = 0; i < nSM; i++) {
      map[i].mix(m);
    }
    mapR1.mix(m);
    mapR2.mix(m);
    mapR3.mix(m);
    mapOLS1.mix(m);
    mapOLS2.mix(m);
  }
  else {
    for (int i = 0; i < nPltMaps; i++) {
      pltMap[i].set((bpos << 8) | iCtx[i]());
      pltMap[i].mix(m);
    }
  }
  for (int i = 0; i < nIM; i++) {
    const int scale = (prevFramePos > 0 && prevFrameWidth == w) ? 64 : 0;
    sceneMap[i].setScale(scale);
    sceneMap[i].mix(m);
  }

  col = (col + 1) & 7;
  m.set(ctx, 512);
  m.set(col << 1 | static_cast<int>(c0 == ((0x100 | res) >> (8 - bpos))), 16);
  m.set((N + W) >> 4, 32);
  m.set(c0 - 1, 255);
  m.set((static_cast<int>(abs(W - N) > 4) << 9) | (static_cast<int>(abs(N - NE) > 4) << 8) |
    (static_cast<int>(abs(W - NW) > 4) << 7) | (static_cast<int>(W > N) << 6) | (static_cast<int>(N > NE) << 5) |
    (static_cast<int>(W > NW) << 4) | (static_cast<int>(W > WW) << 3) | (static_cast<int>(N > NN) << 2) |
    (static_cast<int>(NW > NNWW) << 1) | static_cast<int>(NE > NNEE), 1024);
  m.set(min(63, column[0]), 64);
  m.set(min(127, column[1]), 128);
  m.set(min(255, (x + line) / 32), 256);
}

// ===== src/model/JpegModel.cpp =====
#include <algorithm>

namespace
{

  // The standard Huffman tables (JPEG standard, section K.3). Used when an
  // image has no Huffman tables of its own.
  // IMPORTANT: only valid for 8 bit sample precision.
  constexpr uint8_t bitsDcLuminance[16] = { 0, 1, 5, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0 };
  constexpr uint8_t valuesDcLuminance[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  constexpr uint8_t bitsDcChrominance[16] = { 0, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0 };
  constexpr uint8_t valuesDcChrominance[12] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11 };

  constexpr uint8_t bitsAcLuminance[16] = { 0, 2, 1, 3, 3, 2, 4, 3, 5, 5, 4, 4, 0, 0, 1, 0x7d };
  constexpr uint8_t valuesAcLuminance[162] =
  { 0x01, 0x02, 0x03, 0x00, 0x04, 0x11, 0x05, 0x12, 0x21, 0x31, 0x41, 0x06, 0x13, 0x51,
    0x61, 0x07, 0x22, 0x71, 0x14, 0x32, 0x81, 0x91, 0xa1, 0x08, 0x23, 0x42, 0xb1, 0xc1,
    0x15, 0x52, 0xd1, 0xf0, 0x24, 0x33, 0x62, 0x72, 0x82, 0x09, 0x0a, 0x16, 0x17, 0x18,
    0x19, 0x1a, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39,
    0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56, 0x57,
    0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74, 0x75,
    0x76, 0x77, 0x78, 0x79, 0x7a, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89, 0x8a, 0x92,
    0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5, 0xa6, 0xa7,
    0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0xc2, 0xc3,
    0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6, 0xd7, 0xd8,
    0xd9, 0xda, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf1, 0xf2,
    0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
  };

  constexpr uint8_t bitsAcChrominance[16] = { 0, 2, 1, 2, 4, 4, 3, 4, 7, 5, 4, 4, 0, 1, 2, 0x77 };
  constexpr uint8_t valuesAcChrominance[162] =
  { 0x00, 0x01, 0x02, 0x03, 0x11, 0x04, 0x05, 0x21, 0x31, 0x06, 0x12, 0x41, 0x51, 0x07,
    0x61, 0x71, 0x13, 0x22, 0x32, 0x81, 0x08, 0x14, 0x42, 0x91, 0xa1, 0xb1, 0xc1, 0x09,
    0x23, 0x33, 0x52, 0xf0, 0x15, 0x62, 0x72, 0xd1, 0x0a, 0x16, 0x24, 0x34, 0xe1, 0x25,
    0xf1, 0x17, 0x18, 0x19, 0x1a, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x35, 0x36, 0x37, 0x38,
    0x39, 0x3a, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x53, 0x54, 0x55, 0x56,
    0x57, 0x58, 0x59, 0x5a, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69, 0x6a, 0x73, 0x74,
    0x75, 0x76, 0x77, 0x78, 0x79, 0x7a, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,
    0x8a, 0x92, 0x93, 0x94, 0x95, 0x96, 0x97, 0x98, 0x99, 0x9a, 0xa2, 0xa3, 0xa4, 0xa5,
    0xa6, 0xa7, 0xa8, 0xa9, 0xaa, 0xb2, 0xb3, 0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba,
    0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8, 0xc9, 0xca, 0xd2, 0xd3, 0xd4, 0xd5, 0xd6,
    0xd7, 0xd8, 0xd9, 0xda, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7, 0xe8, 0xe9, 0xea, 0xf2,
    0xf3, 0xf4, 0xf5, 0xf6, 0xf7, 0xf8, 0xf9, 0xfa
  };

  // The standard tables by Tc * 2 + Th.
  constexpr const uint8_t* standardBits[4] = { bitsDcLuminance, bitsDcChrominance, bitsAcLuminance, bitsAcChrominance };
  constexpr const uint8_t* standardValues[4] = { valuesDcLuminance, valuesDcChrominance, valuesAcLuminance, valuesAcChrominance };

  // SOI followed by the start of a valid marker: FF D8 FF, then SOF0/SOF1, DHT,
  // or DB..FE (DQT, DNL, DRI, ..., APPx, COM).
  bool isJpegStart(const RingBuffer<uint8_t>& buf) {
    return buf(4) == 0xFF && buf(3) == 0xD8 && buf(2) == 0xFF &&
      ((buf(1) & 0xFE) == 0xC0 || buf(1) == 0xC4 || (buf(1) >= 0xDB && buf(1) <= 0xFE));
  }

} // namespace

JpegModel::JpegModel(Shared* const sh, const MixerFactory* const mf, const uint64_t size) :
  t(size),
  MJPEGMap(sh, 21, 3, 128, 127), /* BitsOfContext, InputBits, Scale, Limit */
  sMap(sh, nSM, 27, 3, 86), /* NumContexts, BitsOfMemory (2^27 bytes = 128 MiB), InputBits, Scale */
  sm(sh, N, 256, 1023, StateMapType::BitHistory),
  smx(sh, 1, 256 * 256, 1023, StateMapType::Generic),
  apm1(sh, 0x20000, 18, 1023),
  apm2(sh, 0x4000, 20, 1023), apm3(sh, 0x4000, 21, 1023), apm4(sh, 0x4000, 22, 1023), apm5(sh, 0x4000, 23, 1023),
  apm6(sh, 0x4000, 20, 1023), apm7(sh, 0x4000, 21, 1023), apm8(sh, 0x4000, 22, 1023), apm9(sh, 0x4000, 23, 1023), apm10(sh, 0x4000, 24, 1023),
  apm11(sh, 0x8000, 22, 1023), apm12(sh, 0x8000, 22, 1023), apm13(sh, 0x8000, 22, 1023), apm14(sh, 0x8000, 22, 1023),
  shared(sh) {
  // The inner mixer. It combines the bit history contexts and the stationary
  // maps into one prediction, which the APMs in mix() then refine.
  // Must match the m1->set() calls in mix(), in number and in size.
  constexpr int innerMixerContexts =
    1024 + // horizontal position in the image
    2 +    // no block to the west
    1024 + // coefficient and bits of the Huffman code so far
    1024 + // Huffman code so far and frequency
    8192 + // Huffman code so far (hashed)
    8 +    // which extrapolations are zero
    24;    // position inside the symbol
  m1 = mf->createMixer(
    N + 1 /*bias*/ + IndirectMap::MIXERINPUTS + nSM * StationaryMapForJpegModel::MIXERINPUTS,
    innerMixerContexts, 7, 0);
  m1->setScaleFactor(960, 320, 128);
}

JpegModel::~JpegModel() {
  delete m1;
}

void JpegModel::finishImage(const uint32_t pos) {
  const uint32_t length = pos - images[idx].offset;
  memset(&images[idx], 0, sizeof(JPEGImage));
  mcuSize = 0;
  dqtState = -1;
  idx -= static_cast<int>(idx > 0);
  if (images[idx].app <= length) {
    images[idx].app = 0;
  }
  else {
    images[idx].app -= length;
  }
}

void JpegModel::loadStandardHuffmanTables() {
  for (int t = 0; t < 4; ++t) { // t = Tc * 2 + Th
    const int tc = t >> 1;
    const int th = t & 1;
    HUF* const h = &huf[tc * 64 + th * 16];
    const int start = tc * 1024 + th * 256; // where the symbols go in hBuf
    int hval = start;
    int code = 0;
    int count = 0; // number of symbols
    for (int i = 0; i < 16; ++i) {
      const int x = standardBits[t][i]; // number of codes of length i+1
      h[i].min = code;
      h[i].max = (code += x);
      h[i].val = hval;
      hval += x;
      code += code;
      count += x;
    }
    for (int i = 0; i < count; ++i) {
      hBuf[start + i] = standardValues[t][i];
    }
  }
  images[idx].htSize = 4;
}

int JpegModel::extrapolateDct(const int zz2, const int q, const uint32_t avail) const {
  const int u = zzu[zz2];
  const int v = zzv[zz2];
  int64_t tailU = 0;
  int64_t tailV = 0;
  for (int i = u + 1; i < 8; ++i) {
    tailU += sumU[i];
  }
  for (int i = v + 1; i < 8; ++i) {
    tailV += sumV[i];
  }
  const int64_t fromN = static_cast<int64_t>(sumU[u]) * (2 + u) - 2 * tailU; // sumU[] comes from the north block
  const int64_t fromW = static_cast<int64_t>(sumV[v]) * (2 + v) - 2 * tailV; // sumV[] comes from the west block
  int64_t ex = 0;
  switch (avail) {
  case 3: ex = fromN + fromW; break;
  case 2: ex = 2 * fromN; break; // no west block
  case 1: ex = 2 * fromW; break; // no north block
  default: return 0;
  }
  ex = ex * 4 / (u + v + 16);
  return static_cast<int>(ex / ((images[idx].qTable[q + zz2] + 1) * 185));
}

void JpegModel::inBlockNeighbours(const int zz, const int zz2, const int q, int* const out) const {
  const uint8_t* const qt = images[idx].qTable;
  const int cposDc = cPos - zz;
  const int u = zzu[zz2];
  const int v = zzv[zz2];

  // Every in-block neighbour of zz2 comes before it in zigzag order. Either it
  // is already decoded (< zz), or it is inside the run. Then it is zero, and
  // its cBuf2 entry has not been written yet, so it must not be read.
  const auto scaled = [&](const int k) {
    if (k >= zz) {
      return 0;
    }
    return neighbourLog((qt[q + k] + 1) * cBuf2[cposDc + k] / (qt[q + zz2] + 1));
    };
  for (int i = 0; i < 4; ++i) {
    const int a = (i & 1) != 0 ? v : u;
    const int b = (i & 2) != 0 ? 2 : 1;
    out[i] = a < b ? 65535 : scaled(zPos[u + 8 * v - ((i & 1) != 0 ? 8 : 1) * b]);
  }
  if (u * v != 0) {
    out[4] = scaled(zPos[u + 8 * v - 9]);
    out[5] = scaled(zPos[8 * v]);
    out[6] = scaled(zPos[u]);
  }
  else {
    out[4] = out[5] = out[6] = 65535;
  }
}

void JpegModel::alignPredictors(const int r) {
  const int zz = mcuPos & 63;
  const int zz2 = zz + r; // the nonzero coefficient the extra bits belong to
  if (zz2 > 63) {
    return; // invalid run; the decoder stops when the symbol completes
  }
  const int q = 64 * images[idx].qMap[mcuPos >> 6];
  const uint8_t* const qt = images[idx].qTable;
  const int u = zzu[zz2];
  const int v = zzv[zz2];

  // alAdv[0..2]: sumU[] and sumV[] hold the neighbour projections minus the
  // coefficients of this block before zz. The zeros of the run would not change
  // them, so this gives exactly what updatePredictions() would compute for
  // advPred[] at zz2 (zz2 > 0, so there is no DC correction).
  for (int i = 0; i < 3; ++i) {
    int p = sumU[u] * i + sumV[v] * (2 - i);
    p /= (qt[q + zz2] + 1) * 185 * (16 + v) * (16 + u) / 128;
    alAdv[i] = signedLog(p);
  }
  // the same border replacement as in updatePredictions()
  if ((nbAvail & 1) == 0) {
    alAdv[1] = alAdv[2];
    alAdv[0] = 0;
  }
  if ((nbAvail & 2) == 0) {
    alAdv[1] = alAdv[0];
    alAdv[2] = 0;
  }
  alAdv[3] = signedLog(extrapolateDct(zz2, q, nbAvail));

  inBlockNeighbours(zz, zz2, q, alLcp);
}

void JpegModel::updatePredictions() {
  const int aComp = mcuPos >> 6;
  const int q = 64 * images[idx].qMap[aComp];
  const int zz = mcuPos & 63U;
  const int cposDc = cPos - zz;

  const int mcuIndex = column + row * width;
  // True when the MCU that just ended is the last one of a restart
  // interval: the RSTn marker that resets pred[] comes next, but has
  // not been parsed yet, so resetPos is out of date. The same test as
  // the restart padding check in update().
  // resetLen comes from the DRI segment. Without one it is only known
  // after the first RSTn marker, so in the first interval neither test
  // works: the first DC after it is predicted as a difference, and the
  // padding before it is modelled as ordinary bits.
  const bool restartAhead = aComp == 0 && resetLen > 0 && mcuIndex - resetPos == resetLen;
  const bool noReset = resetPos != mcuIndex && !restartAhead;
  // Is this block's DC coded as a difference from cBuf2[cposDc - ls[aComp]]?
  // After a restart this is only true when ls == 64, i.e. when that
  // block is in the same MCU.
  const bool dcRelative = noReset || (ls[aComp] == 64 && aComp != 0);

  // Is the neighbour block (same component) inside the image? It is
  // not when it would be in the previous MCU column at the left
  // border, or in the previous MCU row at the top border.
  const bool westOk = !(column == 0 && blockW[aComp] > 64 * aComp);
  const bool northOk = !(row == 0 && blockN[aComp] > 64 * aComp);
  nbAvail = static_cast<uint32_t>(westOk) | static_cast<uint32_t>(northOk) << 1;

  if (zz == 0) {
    for (int i = 0; i < 8; ++i) {
      sumU[i] = sumV[i] = 0;
    }
    // position in the buffer of the DC coefficient of the west block
    // of the same component (not necessarily in this MCU)
    const int offsetDcW = cposDc - blockW[aComp];
    // position in the buffer of the DC coefficient of the north block
    // of the same component (not necessarily in this MCU)
    const int offsetDcN = cposDc - blockN[aComp];
    for (int i = 0; i < 64; ++i) {
      sumU[zzu[i]] += ((zzv[i] & 1) != 0 ? -1 : 1) * (zzv[i] != 0 ? 16 * (16 + zzv[i]) : 185) * (images[idx].qTable[q + i] + 1) *
        cBuf2[offsetDcN + i];
      sumV[zzv[i]] += ((zzu[i] & 1) != 0 ? -1 : 1) * (zzu[i] != 0 ? 16 * (16 + zzu[i]) : 185) * (images[idx].qTable[q + i] + 1) *
        cBuf2[offsetDcW + i];
    }
  }
  else {
    // the previous coefficient of this block is now known: remove it
    sumU[zzu[zz - 1]] -=
      (zzv[zz - 1] != 0 ? 16 * (16 + zzv[zz - 1]) : 185) * (images[idx].qTable[q + zz - 1] + 1) * cBuf2[cPos - 1];
    sumV[zzv[zz - 1]] -=
      (zzu[zz - 1] != 0 ? 16 * (16 + zzu[zz - 1]) : 185) * (images[idx].qTable[q + zz - 1] + 1) * cBuf2[cPos - 1];
  }

  // advPred[0..2] for this coefficient. The loop also looks up to 9
  // positions ahead: where the next larger coefficient is expected
  // (runPred[]), and the predictions 1-3 positions ahead (advPred1/2/3[]).
  for (int i = 0; i < 3; ++i) {
    runPred[i] = runPred[i + 3] = 0;
    advPred1[i] = advPred2[i] = advPred3[i] = 0;
    for (int st = 0; st < 10 && zz + st < 64; ++st) {
      const int zz2 = zz + st;
      int p = sumU[zzu[zz2]] * i + sumV[zzv[zz2]] * (2 - i);
      p /= (images[idx].qTable[q + zz2] + 1) * 185 * (16 + zzv[zz2]) * (16 + zzu[zz2]) / 128;
      if (zz2 == 0 && dcRelative) {
        p -= cBuf2[cposDc - ls[aComp]];
      }
      p = signedLog(p);
      if (st == 1) {
        advPred1[i] = p;
      }
      else if (st == 2) {
        advPred2[i] = p;
      }
      else if (st == 3) {
        advPred3[i] = p;
      }
      if (st == 0) {
        advPred[i] = p;
      }
      else if (abs(p) > abs(advPred[i]) + 2 && abs(advPred[i]) < 210) {
        if (runPred[i] == 0) {
          runPred[i] = st * 2 + static_cast<int>(p > 0);
        }
        if (abs(p) > abs(advPred[i]) + 21 && runPred[i + 3] == 0) {
          runPred[i + 3] = st * 2 + static_cast<int>(p > 0);
        }
      }
    }
  }

  // advPred[3]: from both neighbour blocks together, even when one of them
  // is outside the image (alAdv[3] below leaves that one out)
  int ex = extrapolateDct(zz, q, 3);
  if (zz == 0 && dcRelative) {
    ex -= cBuf2[cposDc - ls[aComp]];
  }
  advPred[3] = signedLog(ex);

  // lcp[]: the already decoded neighbours of this coefficient in this
  // block, scaled to this coefficient's quantization step
  inBlockNeighbours(zz, zz, q, &lcp[0]);

  lma = 0;
  for (int i = 0; i < 7; ++i) {
    lma |= static_cast<uint32_t>(lcp[i] == 65535) << i;
  }

  // prevCoef, prevCoef2, prevCoefRs: the same coefficient in the
  // blocks of this MCU decoded before this one (other components)
  int prev1 = 0;
  int prev2 = 0;
  int cnt1 = 0;
  int cnt2 = 0;
  int r = 0;
  int s = 0;
  prevCoefRs = coefficientBuffer[cPos - 64];
  for (int i = 0; i < aComp; i++) {
    ex = cBuf2[cPos - (aComp - i) * 64];
    if (zz == 0 && (noReset || ls[i] == 64)) {
      ex -= cBuf2[cposDc - (aComp - i) * 64 - ls[i]];
    }
    if (color[i] == color[aComp] - 1) {
      prev1 += ex;
      cnt1++;
      r += coefficientBuffer[cPos - (aComp - i) * 64] >> 4;
      s += coefficientBuffer[cPos - (aComp - i) * 64] & 0x0F;
    }
    if (color[aComp] > 1 && color[i] == color[0]) {
      prev2 += ex;
      cnt2++;
    }
  }
  if (cnt1 > 0) {
    prev1 /= cnt1;
    r /= cnt1;
    s /= cnt1;
    prevCoefRs = (r << 4) | s;
  }
  if (cnt2 > 0) {
    prev2 /= cnt2;
  }
  prevCoef = signedLog(11 * prev1) + (cnt1 << 20);
  prevCoef2 = signedLog(11 * prev2);

  // Image border: replace the predictions that would read a neighbour
  // block outside the image
  if (!westOk) {
    runPred[1] = runPred[2];
    runPred[0] = 0;
    advPred[1] = advPred[2];
    advPred[0] = 0;
  }
  if (!northOk) {
    runPred[1] = runPred[0];
    runPred[2] = 0;
    advPred[1] = advPred[0];
    advPred[2] = 0;
  }

  // Computed after the border replacement above, so it also marks the
  // advPred[] entries that were set to 0 at the left and top border
  ama = 0;
  for (int i = 0; i < 3; ++i) {
    ama |= static_cast<uint32_t>(advPred[i] == 0) << i;
  }

  // The aligned copies start at this coefficient. alignPredictors()
  // moves them to the end of a run of zeros once the next symbol's rs
  // is known. alAdv[3] is advPred[3] without a neighbour that is
  // outside the image.
  for (int i = 0; i < 3; ++i) {
    alAdv[i] = advPred[i];
  }
  if (nbAvail == 0) {
    alAdv[3] = 0; // no neighbour: no extrapolation, and a DC difference of 0
  }
  else {
    int e3 = extrapolateDct(zz, q, nbAvail);
    if (zz == 0 && dcRelative) {
      e3 -= cBuf2[cposDc - ls[aComp]];
    }
    alAdv[3] = signedLog(e3);
  }
  for (int i = 0; i < 7; ++i) {
    alLcp[i] = lcp[i];
  }
}

JpegResult JpegModel::update() {

  // Train the bit histories used for the previous prediction with the actual
  // bit. This must happen on every bit after a prediction, even when this bit
  // is not predicted (byte stuffing, markers, decoding errors). Otherwise cp[]
  // would later be trained with an unrelated bit.
  if (predicted) {
    INJECT_SHARED_y
    for (int i = 0; i < N; ++i) {
      StateTable::update(cp[i], y, rnd);
    }
    predicted = false;
  }

  specialPrediction = 0;
  INJECT_SHARED_c0
  // The default context for Special bits (byte stuffing and marker bits).
  // bail() and the restart padding below replace it.
  specialCtx = c0;

  INJECT_SHARED_pos
  if (idx < 0) {
    memset(&images[0], 0, sizeof(images));
    idx = 0;
    lastPos = pos;
  }
  shared->State.JPEG.state = 0;

  // Only stop on a byte boundary
  INJECT_SHARED_buf
  INJECT_SHARED_bpos
  if (bpos == 0) {
    // The rest of the SOS header, after jpeg is set to 2, is not image data yet.
    images[idx].nextJpeg = static_cast<uint32_t>(images[idx].jpeg > 1 && pos >= images[idx].data);
  }
  if (bpos != 0 && (images[idx].jpeg == 0)) {
    return bail();
  }
  if (bpos == 0 && images[idx].app > 0) {
    --images[idx].app;
    if (idx < maxEmbeddedLevel - 1 && isJpegStart(buf)) { // an embedded image, e.g. a thumbnail
      memset(&images[++idx], 0, sizeof(JPEGImage));
    }
  }
  if (images[idx].app > 0) {
    return bail();
  }

  //////////////////////////////////////////////////////////////////////////////
  // Parse the headers, one byte at a time
  //////////////////////////////////////////////////////////////////////////////

  if (bpos == 0) {
    // Baseline Huffman coded DCT JPEG syntax:
    //   SOI APPx... misc... SOF0 DHT... SOS data EOI
    //
    // SOI (FF D8): start of image.
    // APPx (FF Ex) len ...: application data, ignored. There may be several.
    //   len is always a 2 byte big-endian length that includes itself but not
    //   the 2 byte marker before it. The other segments below use the same len.
    // misc: DQT, DNL, DRI, COM (COM is ignored).
    // SOF0 (FF C0) len 08 height width Nf [c HV Tq]...
    //   height and width are in pixels, 2 bytes each. Nf (1 byte) is the number
    //   of components. For each component: c is its identifier, HV its
    //   horizontal and vertical sampling factors (high and low 4 bits), and Tq
    //   its quantization table number. An MCU (minimum coded unit) holds H*V
    //   blocks of 64 DCT coefficients for each component.
    // DHT (FF C4) len [TcTh L1..L16 V1,1..V1,L1 ... V16,1..V16,L16]...
    //   defines Huffman table Th (0-3) for Tc (0 = DC, the first coefficient
    //   of a block; 1 = AC, the other 63). L1..L16 are the number of codes of
    //   each length 1-16, and the V values are the symbols, 1 byte each.
    //   An AC symbol RS means: a run of R (0-15) zeros, then a nonzero value
    //   given by the S (0-15) extra bits after the code; the value is negative
    //   if the first extra bit is 0. For example, symbol 0x63 followed by the
    //   bits 1,0,1 gives 7 coefficients: 0, 0, 0, 0, 0, 0, 5.
    //   Symbol 00 means end of block: the remaining AC coefficients are 0.
    //   Symbol F0 (ZRL) means 16 zeros: a run of 15, then a zero value
    //   (S = 0, no extra bits).
    // SOS (FF DA) len Ns [Cs TdTa]... 0 3F 00
    //   start of scan. For each of the Ns (1-4) components: Cs matches a c in
    //   SOF0, and TdTa gives its DC and AC Huffman table numbers (0-3), packed
    //   into one byte.
    // EOI (FF D9): end of image.
    // The Huffman coded data follows SOS. Markers may appear inside it:
    // RST0-RST7 (FF D0 to FF D7) mark the start of an independently coded part.
    // DNL (FF DC) 04 00 height may appear at the end of the scan (ignored).
    // FF 00 in the data stands for FF (so it is not mistaken for a marker).

    // Detect a JPEG: SOI followed by a valid marker
    if ((images[idx].jpeg == 0) && isJpegStart(buf)) {
      images[idx].jpeg = 1;
      images[idx].offset = pos - 4;
      images[idx].sos = images[idx].sof = images[idx].htSize = images[idx].data = images[idx].dri = 0;
      // SOI followed by APPx: skip its 2 length bytes first; then the APPx
      // detection below skips the rest of the segment.
      images[idx].app = static_cast<int>(buf(1) >> 4 == 0xE) * 2;
      mcuSize = huffCode = huffBits = huffSize = mcuPos = cPos = 0;
      rs = -1;
      memset(&huf[0], 0, huf.size() * sizeof(HUF));
      memset(&pred[0], 0, pred.size() * sizeof(int));
      resetPos = resetLen = 0;
    }

    // Detect the end of the JPEG: a marker in the data other than RSTx or
    // byte stuffing (00), or a jump in position since the last byte
    if ((images[idx].jpeg != 0) && (images[idx].data != 0) &&
      ((buf(2) == FF && (buf(1) != 0) && (buf(1) & 0xf8) != RST0) || (pos - lastPos > 1))) {
      JASSERT((buf(1) == EOI) || (pos - lastPos > 1))
      finishImage(pos);
    }
    lastPos = pos;
    if (images[idx].jpeg == 0) {
      return bail();
    }

    // Detect APPx, COM and other markers, so they can be skipped.
    // skippableSegment: C1-CF except DHT (SOF1-SOF15, JPG, DAC) and DC-FE
    // (DNL, DRI, DHP, EXP, APPx, JPGn, COM), only in the top-level image.
    // Skipping SOFn does not lose its position, which is saved below.
    bool skippableSegment = ((((buf(3) >= 0xC1) && (buf(3) <= 0xCF) && (buf(3) != DHT)) || ((buf(3) >= 0xDC) && (buf(3) <= 0xFE))) && idx == 0);
    if ((images[idx].data == 0) && (images[idx].app == 0) && buf(4) == FF &&
      (buf(3) >> 4 == 0xe || buf(3) == COM || skippableSegment)) {
      images[idx].app = buf(2) * 256 + buf(1) + 2;
      if (idx > 0) {
        JASSERT(pos + images[idx].app < images[idx].offset + images[idx - 1].app)
      }
    }

    // Save the positions of SOF, DHT, SOS and the image data
    if (buf(5) == FF && buf(4) == SOS) {
      int len = buf(3) * 256 + buf(2);
      if (len == 6 + 2 * buf(1) && (buf(1) != 0) && buf(1) <= 4) { // buf(1) is Ns
        images[idx].sos = pos - 5;
        images[idx].data = images[idx].sos + len + 2;
        images[idx].jpeg = 2;
      }
    }
    if (buf(4) == FF && buf(3) == DHT && images[idx].htSize < 8) {
      images[idx].ht[images[idx].htSize++] = pos - 4;
    }
    if (buf(4) == FF && buf(3) == DRI) {
      images[idx].dri = pos - 4;
    }
    if (buf(4) == FF && (buf(3) & 0xFE) == SOF0) {
      images[idx].sof = pos - 4;
    }
    // SOF3 is lossless JPEG: its data is not DCT coefficients, so decoding it
    // as baseline would only give useless contexts. Stop decoding this stream
    // and let the general models handle it.
    if (buf(4) == FF && buf(3) == SOF3) {
      images[idx].jpeg = images[idx].nextJpeg = 0;
    }

    // Parse the quantization tables. Each table is a header byte (precision and
    // table number) and 64 entries. An entry is 1 byte, or 2 bytes when the
    // header says 16 bit precision; then only the low byte is kept.
    if (buf(4) == FF && buf(3) == DQT) {
      dqtEnd = pos + buf(2) * 256 + buf(1) - 1;
      dqtState = 0;
      qNum16b = 0;
      qNum16 = 0;
    }
    else if (dqtState >= 0) {
      if (pos >= dqtEnd) {
        dqtState = -1;
      }
      else if (qNum16 != 0) { // 16 bit entries
        if (dqtState % 65 == 0) {
          qNum = buf(1);
          qNum16b = 0;
          qNum16 = qNum >> 4;
          qNum = qNum & 0xf;
        }
        else if ((qNum16b & 1) == 0) {
          JASSERT(buf(1) > 0)
          JASSERT(qNum < 4)
          images[idx].qTable[qNum * 64 + ((dqtState % 65) - 1)] = buf(1) - 1;
        }
        if ((qNum16b & 1) == 0) {
          dqtState++;
        }
        qNum16b++;
      }
      else { // 8 bit entries
        if (dqtState % 65 == 0) {
          qNum = buf(1);
          qNum16b = 0;
          qNum16 = qNum >> 4;
          qNum = qNum & 0xf;
        }
        else {
          JASSERT(buf(1) > 0)
          JASSERT(qNum < 4)
          images[idx].qTable[qNum * 64 + ((dqtState % 65) - 1)] = buf(1) - 1;
        }
        dqtState++;
        qNum16b++;
      }
    }

    // Restart marker: the coded data starts over after it
    if (buf(2) == FF && (buf(1) & 0xf8) == RST0) {
      huffCode = huffBits = huffSize = mcuPos = 0;
      rs = -1;
      memset(&pred[0], 0, pred.size() * sizeof(int));
      resetLen = column + row * width - resetPos;
      resetPos = column + row * width;
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // The first bit of the image data has just been coded: build the decoding tables
  //
  // That is bpos == 1 in the first data byte. The decoder below runs in the
  // same call and adds y, the bit just coded, so this is the first call where
  // y is image data. At bpos == 0, y would still be the last bit of the SOS
  // header.
  //////////////////////////////////////////////////////////////////////////////

  {
    if (pos == images[idx].data && bpos == 1) {
      // Build the Huffman tables.
      // huf[Tc][Th][m] = smallest code of length m+1, largest + 1, position of the symbols
      for (uint32_t i = 0; i < images[idx].htSize; ++i) {
        uint32_t p = images[idx].ht[i] + 4; // position of the current table, after the length field
        uint32_t end = p + buf[p - 2] * 256 + buf[p - 1] - 2; // end of the Huffman table
        uint32_t count = 0; // limits the number of tables, in case the data is invalid
        while (p < end && end < pos && end < p + 2100 && ++count < 10) {
          int tc = buf[p] >> 4;
          int th = buf[p] & 15;
          if (tc >= 2 || th >= 4) {
            break; // invalid table: the JASSERT(p == end) below stops decoding
          }
          HUF* h = &huf[tc * 64 + th * 16]; // [tc][th][0];
          int val = p + 17; // position of the symbols
          int hval = tc * 1024 + th * 256; // where the symbols go in hBuf
          int j = 0;
          for (j = 0; j < 256; ++j) { // copy the symbols
            hBuf[hval + j] = buf[val + j];
          }
          int code = 0;
          for (j = 0; j < 16; ++j) {
            h[j].min = code;
            h[j].max = code += buf[p + j + 1];
            h[j].val = hval;
            val += buf[p + j + 1];
            hval += buf[p + j + 1];
            code *= 2;
          }
          p = val;
          JASSERT(hval - (tc * 1024 + th * 256) <= 256) // at most 256 symbols, within this table's part of hBuf
        }
        JASSERT(p == end)
      }
      huffCode = huffBits = huffSize = 0;
      rs = -1;

      if (images[idx].htSize == 0) {
        loadStandardHuffmanTables();
      }

      // No SOF0/SOF1 before the scan (e.g. progressive SOF2, or arithmetic
      // coding): the data cannot be decoded as baseline, so stop decoding this
      // stream, the same way as for invalid data.
      JASSERT(images[idx].sof != 0)

      // Build the table that selects the Huffman table for each block of the
      // MCU, and get the image width.
      int ns = buf[images[idx].sos + 4];
      int nf = buf[images[idx].sof + 9];
      JASSERT(ns <= 4 && nf <= 4)
      mcuSize = 0; // blocks per MCU
      int hmax = 0; // largest horizontal sampling factor of the components in the scan
      int hmaxFrame = 0; // largest horizontal sampling factor of all components
      for (int j = 0; j < nf; ++j) {
        hmaxFrame = max(hmaxFrame, buf[images[idx].sof + 3 * j + 11] >> 4);
      }
      for (int i = 0; i < ns; ++i) {
        for (int j = 0; j < nf; ++j) {
          if (buf[images[idx].sos + 2 * i + 5] == buf[images[idx].sof + 3 * j + 10]) { // Cs == c ?
            int hv = buf[images[idx].sof + 3 * j + 11]; // sampling factors H and V, packed
            if (hv >> 4 > hmax) {
              hmax = hv >> 4;
            }
            // A non-interleaved scan (Ns = 1) has one block per MCU, whatever
            // the sampling factors are (JPEG standard, A.2.2).
            if (ns == 1) {
              hv = 0x11;
            }
            samplingFactors[i] = hv;
            hv = (hv & 15U) * (hv >> 4); // number of blocks of this component in an MCU
            JASSERT(hv >= 1 && hv + mcuSize <= 10)
            while (hv != 0) {
              JASSERT(mcuSize < 10)
              hufSel[0][mcuSize] = buf[images[idx].sos + 2 * i + 6] >> 4 & 15;
              hufSel[1][mcuSize] = buf[images[idx].sos + 2 * i + 6] & 15;
              JASSERT(hufSel[0][mcuSize] < 4 && hufSel[1][mcuSize] < 4)
              color[mcuSize] = i;
              int tq = buf[images[idx].sof + 3 * j + 12]; // quantization table number (0..3)
              JASSERT(tq >= 0 && tq < 4)
              images[idx].qMap[mcuSize] = tq;
              --hv;
              ++mcuSize;
            }
          }
        }
      }
      JASSERT(hmax >= 1 && hmax <= 10 && hmaxFrame >= hmax)
      int j = 0;
      for (j = 0; j < mcuSize; ++j) {
        ls[j] = 0;
        for (int i = 1; i < mcuSize; ++i) {
          if (color[(j + i) % mcuSize] == color[j]) {
            ls[j] = i;
          }
        }
        ls[j] = (mcuSize - ls[j]) << 6;
      }
      for (j = 0; j < 64; ++j) {
        zPos[zzu[j] + 8 * zzv[j]] = j;
      }
      width = buf[images[idx].sof + 7] * 256 + buf[images[idx].sof + 8]; // in pixels
      if (ns == 1) {
        // One block per MCU: the width is the component's own width in blocks.
        // The component is hmax / hmaxFrame of the image width (rounded up).
        const int componentWidth = (width * hmax + hmaxFrame - 1) / hmaxFrame; // in pixels
        width = (componentWidth + 7) / 8; // in MCUs
      }
      else {
        width = (width - 1) / (hmax * 8) + 1; // in MCUs
      }
      JASSERT(width > 0)

      // The restart interval from DRI, so the restart tests work from the start
      // (see updatePredictions()). Each RSTn marker then measures it again.
      if (images[idx].dri != 0) {
        resetLen = buf[images[idx].dri + 4] * 256 + buf[images[idx].dri + 5];
      }
      mcuSize *= 64; // coefficients per MCU
      row = column = 0;

      // Nothing has been decoded from this image yet, so these predictions left
      // over from the previous image must not be used. This matters for files
      // with many small images.
      nbAvail = 0;
      for (int i = 0; i < 4; ++i) {
        alAdv[i] = 0;
      }
      for (int i = 0; i < 7; ++i) {
        alLcp[i] = 65535;
      }

      // With subsampling a component has several blocks in an MCU. Compute for
      // each block the distance to its west and north neighbour of the same
      // component.
      int x = 0;
      int y = 0;
      for (j = 0; j < (mcuSize >> 6); j++) {
        int i = color[j];
        int w = samplingFactors[i] >> 4;
        int h = samplingFactors[i] & 0x0f;
        blockW[j] = x == 0 ? mcuSize - 64 * (w - 1) : 64;
        blockN[j] = y == 0 ? mcuSize * width - 64 * w * (h - 1) : w * 64;
        x++;
        if (x >= w) {
          x = 0;
          y++;
        }
        if (y >= h) {
          x = 0;
          y = 0;
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Huffman decoding: add the last bit to the current symbol. When the symbol
  // is complete, store its coefficients and compute the predictions for the
  // next coefficient.
  //////////////////////////////////////////////////////////////////////////////

  {
    if ((mcuSize != 0) && buf(1 + static_cast<int>(bpos == 0)) != FF) { // skip the stuffed 00 after FF
      JASSERT(huffBits <= 32)
      INJECT_SHARED_y
      huffCode += huffCode + y;
      ++huffBits;
      if (rs < 0) {
        JASSERT(huffBits >= 1 && huffBits <= 16)
        const int ac = static_cast<int>((mcuPos & 63U) > 0);
        JASSERT((mcuPos >> 6) < 10)
        const int sel = hufSel[ac][mcuPos >> 6];
        JASSERT(sel >= 0 && sel < 4)
        const int i = huffBits - 1;
        const HUF* h = &huf[ac * 64 + sel * 16]; // [ac][sel];
        JASSERT(h[i].min <= h[i].max && h[i].val < 2048)
        if (huffCode < h[i].max) {
          JASSERT(huffCode >= h[i].min)
          int k = h[i].val + huffCode - h[i].min;
          JASSERT(k >= 0 && k < 2048)
          rs = hBuf[k];
          huffSize = huffBits;
          // An AC symbol with a run of zeros and extra bits: the extra bits
          // belong to the coefficient r positions after mcuPos, so move the
          // predictions there. (With s == 0 the symbol is already complete: it
          // is handled right below, and updatePredictions() resets the predictions.)
          if (ac != 0 && (rs >> 4) != 0 && (rs & 15) != 0) {
            alignPredictors(rs >> 4);
          }
        }
      }
      if (rs >= 0) {
        if (huffSize + (rs & 15) == huffBits) { // the symbol is complete
          rs1 = rs;
          int ex = 0; // the value given by the extra bits
          if ((mcuPos & 63) != 0) { // AC
            if (rs == 0) { // end of block
              mcuPos = (mcuPos + 63) & 0xFFFFFFC0;
              JASSERT(mcuPos <= static_cast<uint32_t>(mcuSize) && mcuPos <= 640)
              bool first = true; // the first remaining coefficient is stored as 0
              while ((cPos & 63) != 0) {
                cBuf2.set(cPos, 0);
                coefficientBuffer.set(cPos, first ? 0 : (63 - (cPos & 63U)) << 4);
                cPos++;
                first = false;
              }
            }
            else { // r zeros, then a value given by s extra bits (nonzero, except for ZRL)
              // The value is negative if the first extra bit is 0.
              JASSERT((rs & 15) <= 10)
              const int r = rs >> 4;
              const int s = rs & 15;
              JASSERT(mcuPos >> 6 == (mcuPos + r) >> 6)
              mcuPos += r + 1;
              ex = huffCode & ((1U << s) - 1);
              if (s != 0 && (ex >> (s - 1)) == 0) {
                ex -= (1u << s) - 1;
              }
              for (int i = r; i >= 1; --i) {
                cBuf2.set(cPos, 0);
                coefficientBuffer.set(cPos, (i << 4) | s);
                cPos++;
              }
              cBuf2.set(cPos, ex);
              coefficientBuffer.set(cPos, (s << 4) | (huffCode << 2 >> s & 3U) | 12);
              cPos++;
              sSum += s;
            }
          }
          else { // DC: the symbol is s alone (rs < 12)
            JASSERT(rs < 12)
            ++mcuPos;
            ex = huffCode & ((1U << rs) - 1);
            if (rs != 0 && (ex >> (rs - 1)) == 0) {
              ex -= (1U << rs) - 1;
            }
            JASSERT(mcuPos >> 6 < 10)
            const uint32_t dcComp = color[mcuPos >> 6];
            JASSERT(dcComp < 4)
            dc = pred[dcComp] += ex;

            // realign to a block boundary; needed for some thumbnails in phone images
            while ((cPos & 63) != 0) {
              cPos++;
            }

            cBuf2.set(cPos, dc);
            coefficientBuffer.set(cPos, (dc + 1023) >> 3);
            cPos++;
            if ((mcuPos >> 6) == 0) {
              sSum1 = 0;
              sSum2 = sSum3;
            }
            else {
              if (color[(mcuPos >> 6) - 1] == color[0]) {
                sSum1 += (sSum3 = sSum);
              }
              sSum2 = sSum1;
            }
            sSum = rs;
          }
          JASSERT(mcuPos <= static_cast<uint32_t>(mcuSize))
          if (mcuPos >= static_cast<uint32_t>(mcuSize)) {
            mcuPos = 0;
            if (++column == width) {
              column = 0;
              ++row;
            }
          }
          huffCode = huffSize = huffBits = 0;
          rs = -1;
          updatePredictions();
        }
      }
    }
  }

  //////////////////////////////////////////////////////////////////////////////
  // Is this bit entropy coded, i.e. should JpegModel predict it?
  //////////////////////////////////////////////////////////////////////////////

  if ((images[idx].jpeg == 0) || (images[idx].data == 0) || pos < images[idx].data) { // not yet in the image data
    return bail();
  }
  // Only predict the bits that the Huffman decoder above adds to huffCode, so
  // that every prediction matches one decoder step, and hbCount and cp[] stay
  // in step with the decoder.
  // Each call predicts the current bit and adds the previous bit to the
  // decoder. When bpos > 0 both bits are in the same byte, so both checks look
  // at the byte before it, buf(1). When bpos == 0 they are in different bytes:
  // the previous bit is the last bit of buf(1), which the decoder checks
  // against buf(2); the current bit is the first bit of a new byte, which the
  // check below tests against buf(1). So the two checks only differ when
  // bpos == 0.
  if (buf(1) == FF) { // byte stuffing or marker
    return setResult(JpegResult::Special);
  }
  if (resetLen > 0 && resetLen == column + row * width - resetPos && mcuPos == 0 && huffCode == (1u << huffBits) - 1u) {
    // Padding before a restart marker: all its bits are 1
    specialPrediction = 2047;
    specialCtx = 256 + c0;
    return setResult(JpegResult::Special);
  }

  //////////////////////////////////////////////////////////////////////////////
  // Context values for this bit, used by mix() and by the SSE stage
  //////////////////////////////////////////////////////////////////////////////

  comp = color[mcuPos >> 6];
  coef = (mcuPos & 63) | comp << 6;
  hc = (huffCode * 4 + static_cast<uint32_t>((mcuPos & 63) == 0) * 2 + static_cast<uint32_t>(comp == 0)) | 1u << (huffBits + 2);
  firstCol = column == 0 && static_cast<uint32_t>(blockW[mcuPos >> 6]) > mcuPos;
  zu = zzu[mcuPos & 63];
  zv = zzv[mcuPos & 63];

  // How far we are into the current symbol. While the Huffman code is being
  // read, huffSize is 0, so e is the code length so far; after that, e counts
  // the extra bits.
  const uint32_t e = huffBits - huffSize;

  // While the Huffman code is being read there is one context per node of the
  // code tree. Once the symbol is known, the code itself does not matter any
  // more, only: which symbol it was, which extra bit we are on, and the sign
  // (the first extra bit). Keeping all the extra bits would double the number
  // of contexts with each bit, without any gain.
  symKey = rs < 0 ? hc
    : 0x80000000u | rs << 8 | e << 4 | (e != 0 ? (huffCode >> (e - 1)) & 1 : 0) << 2
    | static_cast<uint32_t>((mcuPos & 63) == 0) << 1 | static_cast<uint32_t>(comp == 0);

  // During a run of zeros, the bits belong to the coefficient r positions after
  // mcuPos, and alAdv[] and alLcp[] describe that one.
  alZz = static_cast<int>(mcuPos & 63) +
    ((rs >= 0 && (mcuPos & 63) != 0) ? min(63 - static_cast<int>(mcuPos & 63), rs >> 4) : 0);
  // How large the model expects this coefficient to be, as a JPEG size
  // category (the number of extra bits, SSSS). Exact below |p| = 32; above
  // that, it can be one too high just above a power of two, because of how
  // ilog rounds.
  const int predMag = abs(alAdv[1]); // 16 * log2(|p| + 1)
  predCat = min(15, (predMag + 15) >> 4);

  // A short summary of this bit for the SSE stage: which kind of bit it is and
  // where in the symbol (symClass), and what our own prediction says about it
  // (predBits). There are three kinds of bits: Huffman code, sign, magnitude.
  // 8 bits. Codes of 7 or more bits lose their leading-1 length marker, and
  // then share values with shorter codes.
  uint32_t symClass = 0;
  uint32_t predBits = 0; // 2 bits
  phase = 0;
  if (rs < 0) { // Huffman code
    // the code read so far, with a leading 1 to mark its length
    symClass = ((((1u << e) | (huffCode & ((1u << e) - 1))) << 1) | 0u) & 0xFF; // bit 0 is 0 for Huffman code bits
    predBits = min(3, static_cast<int>(predCat));
  }
  else {
    const uint32_t s = rs & 15;
    const uint32_t positive = e != 0 ? (huffCode >> (e - 1)) & 1 : 0;
    symClass = 1 | s << 1 | std::min(e, 3u) << 5 | positive << 7;
    if (e == 0) { // sign bit
      phase = 1;
      predBits = static_cast<uint32_t>(alAdv[1] > 0) << 1 |
        static_cast<uint32_t>(static_cast<int>(predCat) >= static_cast<int>(s)); // the prediction reaches the size category
    }
    else { // magnitude bits: each bit halves the range of values still possible
      phase = 2;
      const uint32_t x = huffCode & ((1u << e) - 1);       // the extra bits so far
      const uint32_t mid = (2 * x + 1) << (s - e - 1);     // the smallest coded value where the next bit is 1
      // d > 0: the prediction says the next bit is 1. Negative values are coded
      // as v + 2^s - 1, so for them the bit is 1 when |v| <= 2^s - 1 - mid.
      const int d = positive != 0
        ? predMag - ilog->log(mid + 1)
        : ilog->log((1u << s) - mid) - predMag;
      predBits = static_cast<uint32_t>(d >= 0) << 1 | static_cast<uint32_t>(abs(d) >= 8); // which side, and by at least half an octave
    }
  }

  shared->State.JPEG.state = 1 + (
    predBits << 10 | // 2 bits
    symClass << 2 |  // 8 bits, also tells the phase
    static_cast<uint32_t>(comp == 0) << 1 |
    static_cast<uint32_t>(zu + zv < 5)
    ); // 1 + (0..4095)

  return setResult(JpegResult::EntropyCoded);
}

void JpegModel::mix(Mixer& m) {
  assert(lastResult == JpegResult::EntropyCoded);

  //////////////////////////////////////////////////////////////////////////////
  // Bit history contexts
  //
  // The contexts are looked up at the start of every symbol and then once
  // every 3 bits. For the bits in between, cp[] just moves to the next node of
  // the 7 bit histories in the slot, which saves a hash lookup per bit.
  //
  // The //strong, //medium and //weak notes on the contexts, maps and mixer
  // contexts below record how much each one helped in tuning (see "For
  // tuning"); commented out lines were not worth keeping.
  //////////////////////////////////////////////////////////////////////////////

  if (++hbCount > 2 || huffBits == 0) {
    hbCount = 0;
  }
  if (hbCount == 0) {
    uint64_t n = static_cast<uint64_t>(hc) * 64;
    int i = 0;

    // n + 24 (three contexts) and n + 32 (two contexts) are shared on purpose:
    // these contexts are similar, so sharing their statistics helps.

    // For tuning:
    //int skip = (int)shared->tuning_param;
    //int k = 0;
    //k++; if (k == skip); else cxt[i++] = ...

    cxt[i++] = hash(n + 1, hc);
    cxt[i++] = hash(n + 2, coef, advPred[2] / 12 + (runPred[2] << 8), sSum2 >> 6, prevCoef / 72);
    cxt[i++] = hash(n + 3, coef, advPred[0] / 12 + (runPred[0] << 8), sSum2 >> 6, prevCoef / 72);
    //cxt[i++] = hash(n + 4, coef, advPred[1] / 11 + (runPred[1] << 8), sSum2 >> 6); //weak
    //cxt[i++] = hash(n + 5, rs1, advPred[2] / 7, runPred[5] / 2, prevCoef / 10); //weak
    cxt[i++] = hash(n + 6, rs1, advPred[0] / 7, runPred[3] / 2, prevCoef / 10); //weak
    cxt[i++] = hash(n + 7, rs1, advPred[1] / 11, runPred[4]);
    cxt[i++] = hash(n + 8, advPred[2] / 14, runPred[2], advPred[0] / 14, runPred[0]);
    cxt[i++] = hash(n + 9, coefficientBuffer[cPos - blockN[mcuPos >> 6]] >> 4, advPred[3] / 17, runPred[1], runPred[5]); //weak
    cxt[i++] = hash(n + 10, coefficientBuffer[cPos - blockW[mcuPos >> 6]] >> 4, advPred[3] / 17, runPred[1], runPred[3]);
    cxt[i++] = hash(n + 11, lcp[0] / 22, lcp[1] / 22, advPred[1] / 7, runPred[1]);
    cxt[i++] = hash(n + 12, lcp[0] / 22, lcp[1] / 22, mcuPos & 63, lcp[4] / 30);
    cxt[i++] = hash(n + 13, zu / 2, lcp[0] / 13, lcp[2] / 30, prevCoef / 40 + ((prevCoef2 / 28) << 20));
    cxt[i++] = hash(n + 14, zv / 2, lcp[1] / 13, lcp[3] / 30, prevCoef / 40 + ((prevCoef2 / 28) << 20));
    cxt[i++] = hash(n + 15, rs1, prevCoef / 42, prevCoef2 / 34, lcp[0] / 60, lcp[2] / 14, lcp[1] / 60, lcp[3] / 14); // strong
    cxt[i++] = hash(n + 16, mcuPos & 63, column >> 1);  // strong
    cxt[i++] = hash(n + 17, column >> 3, min(5 + 2 * (comp == 0), zu + zv), lcp[0] / 10, lcp[2] / 40, lcp[1] / 10, lcp[3] / 40);  // strong
    cxt[i++] = hash(n + 18, sSum >> 3, mcuPos & 63);
    cxt[i++] = hash(n + 19, rs1, mcuPos & 63, runPred[1]);
    cxt[i++] = hash(n + 20, coef, sSum2 >> 5, advPred[3] / 30, comp != 0 ? hash(n + prevCoef / 22, prevCoef2 / 50) : sSum / ((mcuPos & 0x3F) + 1));
    cxt[i++] = hash(n + 21, lcp[0] / 40, lcp[1] / 40, advPred[1] / 28, comp != 0 ? prevCoef / 40 + ((prevCoef2 / 40) << 20) : lcp[4] / 22, min(7, zu + zv), sSum / (2 * (zu + zv) + 1)); // very strong
    cxt[i++] = hash(n + 22, zv, coefficientBuffer[cPos - blockN[mcuPos >> 6]], advPred[2] / 28, runPred[2]); // strong
    cxt[i++] = hash(n + 23, zu, coefficientBuffer[cPos - blockW[mcuPos >> 6]], advPred[0] / 28, runPred[0]); // strong
    cxt[i++] = hash(n + 24, advPred[2] / 7, runPred[2]);//weak
    cxt[i++] = hash(n + 24, advPred[0] / 7, runPred[0]);
    cxt[i++] = hash(n + 24, advPred[1] / 7, runPred[1]);//weak
    cxt[i++] = hash(n + 25, zv, lcp[1] / 14, advPred[2] / 16, runPred[5]); // strong
    cxt[i++] = hash(n + 26, zu, lcp[0] / 14, advPred[0] / 16, runPred[3]); // strong
    cxt[i++] = hash(n + 27, lcp[0] / 14, lcp[1] / 14, advPred[3] / 16); //weak
    cxt[i++] = hash(n + 28, coef, prevCoef / 10, prevCoef2 / 20);
    cxt[i++] = hash(n + 29, coef, sSum >> 2, prevCoefRs);
    cxt[i++] = hash(n + 30, coef, advPred[1] / 17, lcp[(zu < zv)] / 24, lcp[2] / 20, lcp[3] / 24); // very strong also for MJPEG files
    cxt[i++] = hash(n + 31, coef, advPred[3] / 11, lcp[(zu < zv)] / 50, lcp[2 + 3 * (zu * zv > 1)] / 50, lcp[3 + 3 * static_cast<uint64_t>(zu * zv > 1)] / 50);
    cxt[i++] = hash(n + 32, hc, advPred[3] / 13, prevCoef / 11, (zu + zv < 4));//weak
    cxt[i++] = hash(n + 32, hc, advPred[2] / 13, prevCoef / 11, (zu + zv < 4));//weak
    //cxt[i++] = hash(n + 32, hc, advPred[1] / 13, prevCoef / 11, static_cast<int>(zu + zv < 4));//weak
    //cxt[i++] = hash(n + 33, hc, advPred[1] / 13, prevCoef2 / 20, prevCoefRs);//weak
    cxt[i++] = hash(n + 34, hc, advPred[2] / 13, prevCoef / 40 + ((prevCoef2 / 40) << 20), prevCoefRs);
    cxt[i++] = hash(n + 35, hc, advPred[0] / 13, prevCoef / 40 + ((prevCoef2 / 40) << 20), (zu + zv < 4));//weak
    cxt[i++] = hash(n + 36, prevCoef2, prevCoefRs);//weak except for MJPEGs
    cxt[i++] = hash(n + 37, hc, runPred[0] / 13, prevCoef / 11, (zu + zv < 4));
    //cxt[i++] = hash(n + 38, hc, advPred[2] / 7, runPred[2]); //weak
    cxt[i++] = hash(n + 39, hc, advPred[2] / 12, runPred[2], sSum2 >> 6, prevCoef / 12); //weak?
    //cxt[i++] = hash(n + 40, hc, advPred[1] / 11, runPred[1], sSum2 >> 6); //weak
    //cxt[i++] = hash(n + 41, hc, rs1, advPred[0] / 7, prevCoef2 / 10); //weak
    cxt[i++] = hash(n + 42, hc, lcp[0] / 12, lcp[1] / 12, mcuPos & 63, lcp[4] / 10); // strong for MJPEG files
    cxt[i++] = hash(n + 43, hc, zu / 2, prevCoef / 40 + ((prevCoef2 / 28) << 20));
    cxt[i++] = hash(n + 44, hc, zv / 2, prevCoef / 40 + ((prevCoef2 / 28) << 20));
    cxt[i++] = hash(n + 45, hc, column >> 3, min(5 + 2 * (comp == 0), zu + zv), prevCoef / 40 + ((prevCoef2 / 28) << 20));

    assert(i == N);
  }

  coldCount = 0; // counted by addContextInputs() below; used as a mixer context

  m1->add(128); // bias
  sm.subscribe();
  assert(hbCount <= 2);
  switch (hbCount) {
  case 0: {
    for (int i = 0; i < N; ++i) {
      cp[i] = t[cxt[i]];
      addContextInputs(m, i);
    }
    break;
  }
  case 1: {
    const size_t hcOffset = 1 + (huffCode & 1) * 3;
    for (int i = 0; i < N; ++i) {
      cp[i] += hcOffset;
      addContextInputs(m, i);
    }
    break;
  }
  default: {
    const size_t hcOffset = 1 + (huffCode & 1);
    for (int i = 0; i < N; ++i) {
      cp[i] += hcOffset;
      addContextInputs(m, i);
    }
    break;
  }
  }
  // cp[] now points at the bit histories used for this prediction. The next
  // update() call must train them, whatever else it does.
  predicted = true;

  //////////////////////////////////////////////////////////////////////////////
  // Stationary maps
  //////////////////////////////////////////////////////////////////////////////

  if (hbCount == 0) {
    const uint64_t h = hc >> 2; // drop the DC/AC and luma flags
    int j = 0;

    // For tuning:
    //int skip = (int)shared->tuning_param;
    //int k = 0;
    //k++; if (k == skip); else sMap[j++].set ...

    // === DCT extrapolation x coefficient position ===
    sMap.set(j++, hash(h, coef, advPred[0] / 11));
    //sMap.set(j++, hash(h, coef, advPred[1] / 11)); //weak
    sMap.set(j++, hash(h, coef, advPred[2] / 11));
    //sMap.set(j++, hash(h, coef, advPred[3] / 11)); //weak

    // === extrapolation of the next positions x coefficient position ===
    sMap.set(j++, hash(h, coef, advPred1[0] / 11));
    sMap.set(j++, hash(h, coef, advPred1[1] / 11));
    sMap.set(j++, hash(h, coef, advPred1[2] / 11));
    sMap.set(j++, hash(h, coef, advPred2[0] / 11)); //strong
    //sMap.set(j++, hash(h, coef, advPred2[1] / 11)); //weak
    sMap.set(j++, hash(h, coef, advPred2[2] / 11));
    //sMap.set(j++, hash(h, coef, advPred3[2] / 11)); //weak
    sMap.set(j++, hash(h, coef, advPred1[2] / 11, advPred2[2] / 11));
    sMap.set(j++, hash(h, advPred[2] / 11, advPred1[2] / 11));

    // === in-block neighbours x coefficient position ===
    sMap.set(j++, hash(h, coef, lcp[0] / 7));
    sMap.set(j++, hash(h, coef, lcp[1] / 7));
    sMap.set(j++, hash(h, coef, lcp[2] / 7));
    sMap.set(j++, hash(h, coef, lcp[3] / 7));
    sMap.set(j++, hash(h, coef, lcp[4] / 7));
    sMap.set(j++, hash(h, coef, lcp[5] / 7));
    sMap.set(j++, hash(h, coef, lcp[6] / 7));

    // === pairs of in-block neighbours ===
    sMap.set(j++, hash(h, lcp[0] / 14, lcp[1] / 14));
    sMap.set(j++, hash(h, lcp[2] / 14, lcp[3] / 14));
    sMap.set(j++, hash(h, lcp[3] / 14, lcp[4] / 14));
    sMap.set(j++, hash(h, coef, lcp[0] / 10));
    sMap.set(j++, hash(h, coef, lcp[2] / 10, lcp[3] / 10));
    sMap.set(j++, hash(h, coef, lcp[5] / 14, lcp[6] / 14)); //strong
    sMap.set(j++, hash(h, lcp[0] / 14, lcp[2] / 14));
    sMap.set(j++, hash(h, lcp[1] / 14, lcp[3] / 14)); //strong
    sMap.set(j++, hash(h, lcp[4] / 14, lcp[5] / 14));

    // === run prediction x coefficient position ===
    sMap.set(j++, hash(h, coef, runPred[0]));
    //sMap.set(j++, hash(h, coef, runPred[1])); //weak
    sMap.set(j++, hash(h, coef, runPred[2]));
    //sMap.set(j++, hash(h, coef, runPred[3])); //weak
    //sMap.set(j++, hash(h, coef, runPred[4])); //weak
    //sMap.set(j++, hash(h, coef, runPred[5])); //weak
    //sMap.set(j++, hash(h, coef, mcuPos & 63, runPred[1])); //weak
    sMap.set(j++, hash(h, coef, runPred[1], runPred[4]));

    // === extrapolation x run prediction ===
    sMap.set(j++, hash(h, advPred[3] / 17, runPred[1], runPred[5]));
    sMap.set(j++, hash(h, advPred[1] / 16, runPred[3]));
    sMap.set(j++, hash(h, advPred[0] / 16, runPred[1])); //strong
    sMap.set(j++, hash(h, advPred[2] / 17, runPred[0], runPred[4])); //strong
    sMap.set(j++, hash(h, advPred1[2] / 17, runPred[0], runPred[2])); //strong
    sMap.set(j++, hash(h, advPred[0] / 17, runPred[1], runPred[3]));
    sMap.set(j++, hash(h, advPred[2] / 16, runPred[2]));
    sMap.set(j++, hash(h, advPred2[2] / 17, runPred[0], runPred[2]));

    // === extrapolation x in-block neighbour ===
    sMap.set(j++, hash(h, advPred[1] / 12, lcp[0] / 12)); //strong
    sMap.set(j++, hash(h, advPred[0] / 12, lcp[1] / 12)); //strong
    sMap.set(j++, hash(h, advPred[2] / 12, lcp[0] / 12));
    sMap.set(j++, hash(h, advPred[2] / 12, lcp[1] / 12)); //strong
    //sMap.set(j++, hash(h, advPred[3] / 12, lcp[0] / 12)); //weak
    sMap.set(j++, hash(h, advPred[1] / 12, lcp[2] / 12));
    sMap.set(j++, hash(h, advPred[0] / 12, lcp[3] / 12));

    // === extrapolation x the same coefficient in other components ===
    sMap.set(j++, hash(h, advPred[0] / 16, prevCoef / 42)); //strong
    sMap.set(j++, hash(h, advPred[1] / 16, prevCoef / 42)); //strong
    sMap.set(j++, hash(h, advPred[2] / 16, prevCoef / 42)); //strong
    sMap.set(j++, hash(h, advPred[3] / 16, prevCoef / 42)); //strong
    sMap.set(j++, hash(h, coef, advPred1[0] / 16, prevCoef / 42));
    //sMap.set(j++, hash(h, coef, advPred1[1] / 16, prevCoef / 42)); //weak
    //sMap.set(j++, hash(h, coef, advPred1[2] / 16, prevCoef / 42)); //weak
    //sMap.set(j++, hash(h, coef, advPred2[0] / 16, prevCoef / 42)); //weak
    //sMap.set(j++, hash(h, coef, advPred2[1] / 16, prevCoef / 42)); //weak
    //sMap.set(j++, hash(h, coef, advPred3[0] / 16, prevCoef / 42)); //weak
    sMap.set(j++, hash(h, coef, advPred3[1] / 16, prevCoef / 42));  //strong
    sMap.set(j++, hash(h, coef, advPred[3] / 13, prevCoef / 11));
    //sMap.set(j++, hash(h, advPred[0] / 16, prevCoef2 / 42)); //weak
    //sMap.set(j++, hash(h, advPred[2] / 16, prevCoef2 / 42)); //weak
    sMap.set(j++, hash(h, advPred[0] / 16, advPred[2] / 16, prevCoef / 42));

    // === previously decoded values x coefficient position ===
    sMap.set(j++, hash(h, coef, rs1));
    sMap.set(j++, hash(h, coef, prevCoef));
    //sMap.set(j++, hash(h, coef, prevCoef2)); //weak
    sMap.set(j++, hash(h, coef, prevCoefRs));
    sMap.set(j++, hash(h, coef, prevCoef / 42));
    sMap.set(j++, hash(h, coef, prevCoef / 22, prevCoef2 / 50));
    sMap.set(j++, hash(h, coef, abs(prevCoef) / 16, abs(prevCoef2) / 16));
    sMap.set(j++, hash(h, prevCoefRs, prevCoef / 42));

    // === the last symbol (rs1) combined with other values ===
    sMap.set(j++, hash(h, rs1, mcuPos & 63));
    sMap.set(j++, hash(h, rs, rs1));
    sMap.set(j++, hash(h, rs1, runPred[0]));
    sMap.set(j++, hash(h, rs1, runPred[1]));
    sMap.set(j++, hash(h, rs1, runPred[2]));
    //sMap.set(j++, hash(h, rs1, runPred[3])); //weak
    sMap.set(j++, hash(h, rs1, prevCoef / 22)); //strong
    sMap.set(j++, hash(h, rs1, prevCoefRs));
    sMap.set(j++, hash(h, advPred[0] / 7, rs1));
    sMap.set(j++, hash(h, advPred[2] / 7, rs1));
    //sMap.set(j++, hash(h, rs1, coef, comp)); //weak
    //sMap.set(j++, hash(h, rs1, prevCoef2 / 22)); //weak
    sMap.set(j++, hash(h, advPred[3] / 7, rs1));
    sMap.set(j++, hash(h, advPred1[2] / 7, rs1));
    //sMap.set(j++, hash(h, rs1, runPred[4], runPred[5])); //weak

    // === extra bit sums (sSum and related) ===
    sMap.set(j++, hash(h, coef, sSum >> 2));
    //sMap.set(j++, hash(h, coef, sSum2 >> 2)); //weak
    sMap.set(j++, hash(h, coef, sSum1 >> 2));
    //sMap.set(j++, hash(h, coef, sSum3 >> 2)); //weak
    sMap.set(j++, hash(h, coef, min(63, sSum)));
    sMap.set(j++, hash(h, sSum >> 3, sSum2 >> 3));
    sMap.set(j++, hash(h, sSum2 >> 2, prevCoef2 / 42));
    sMap.set(j++, hash(h, sSum >> 1, prevCoef2 / 10));
    sMap.set(j++, hash(h, sSum3 >> 2, prevCoef / 42));
    sMap.set(j++, hash(h, sSum1 >> 2, prevCoef / 42));
    sMap.set(j++, hash(h, sSum >> 2, rs1));

    // === neighbour blocks (north and west, same component) ===
    sMap.set(j++, hash(h, coefficientBuffer[cPos - blockN[mcuPos >> 6]], coefficientBuffer[cPos - blockW[mcuPos >> 6]]));
    sMap.set(j++, hash(h, coefficientBuffer[cPos - blockN[mcuPos >> 6]] >> 4, advPred[3] / 17));
    sMap.set(j++, hash(h, coefficientBuffer[cPos - blockW[mcuPos >> 6]] >> 4, advPred[3] / 17));
    sMap.set(j++, hash(h, coefficientBuffer[cPos - blockN[mcuPos >> 6]] >> 4, advPred[2] / 17));
    sMap.set(j++, hash(h, coefficientBuffer[cPos - blockW[mcuPos >> 6]] >> 4, advPred[0] / 17));

    // === frequency and position ===
    sMap.set(j++, hash(h, coef));
    //sMap.set(j++, hash(h, coef, zu, zv));  //weak
    sMap.set(j++, hash(h, coef, column >> 3));
    //sMap.set(j++, hash(h, mcuPos & 63, column >> 1)); //weak

    // === masks of the missing predictions ===
    sMap.set(j++, hash(h, ama, coef));
    //sMap.set(j++, hash(h, lma, coef)); //weak
    //sMap.set(j++, hash(h, lma, ama, coef)); //weak
    sMap.set(j++, hash(h, lma, advPred[1] / 12, lcp[0] / 12));

    assert(j == nSM);

    MJPEGMap.set(hash(mcuPos, column, row, hc >> 2));
  }

  for (int j = 0; j < nSM; ++j) {
    int st = sMap.mix(*m1, j);
    m.add(st);
  }
  MJPEGMap.mix(*m1);

  //////////////////////////////////////////////////////////////////////////////
  // Where we are inside the current symbol. A Huffman code bit and a magnitude
  // bit need different weights, so both mixers use this as a context.
  //////////////////////////////////////////////////////////////////////////////

  const uint32_t e = huffBits - huffSize;
  const int posInSym =
    phase == 0 ? min(15, static_cast<int>(huffBits)) : // 0..15: depth in the Huffman code tree
    phase == 1 ? 16 :                                  // the sign bit
    17 + min(6, static_cast<int>(e) - 1);              // 17..23: which magnitude bit

  //////////////////////////////////////////////////////////////////////////////
  // Inner mixer contexts
  //////////////////////////////////////////////////////////////////////////////

  // Horizontal position in MCUs. Images wider than 1024 MCUs: divided by
  // width / 1024 (rounded down), then clamped to 1023.
  int colCtx = (width > 1024) ? (min(1023, column / max(1, width / 1024))) : column;
  m1->set(colCtx, 1024);
  m1->set(static_cast<uint32_t>(firstCol), 2); // no block to the west
  m1->set(coef | (min(3, huffBits) << 8), 1024);
  m1->set(((hc & 0x1FE) << 1) | min(3, ilog2(zu + zv)), 1024);
  m1->set(finalize64(hash(hc), 13), 8192); // the Huffman code so far
  m1->set(ama, 8); // which extrapolations are zero; strong for MJPEG files
  m1->set(posInSym, 24);

  // A direct prediction from the symbol state and the coefficient alone.
  m.add(stretch(smx.p1(finalize64(hash(symKey, coef), 16))));

  //////////////////////////////////////////////////////////////////////////////
  // The inner mixer's prediction, refined by the APMs
  //
  // apm1 adjusts the inner mixer's prediction for the current symbol and
  // coefficient. The APM chains below then adjust it further, one feature per
  // APM. The outer mixer does not get the APM outputs as probabilities, but as
  // the change each APM made to its input (in the stretched domain). The
  // changes are the new information, and they are much less correlated with
  // each other than the probabilities.
  //////////////////////////////////////////////////////////////////////////////

  // Adds to the outer mixer how much pr differs from sParent (both stretched),
  // and returns stretch(pr), so it can be the parent of the next APM.
  const auto refine = [&m](const int pr, const int sParent) {
    const int s = stretch(pr);
    m.add(std::clamp((s - sParent) * 2, -2047, 2047));
    return s;
    };

  // APM key parts. alAdv[] and alLcp[] describe the coefficient the current
  // bit belongs to, also after a run of zeros.
  // The sign of a prediction is only used on the sign bit (the first extra
  // bit): the Huffman symbol only depends on the size, and on the later extra
  // bits the actual sign is already in symKey.
  const bool signBit = rs >= 0 && e == 0;
  const auto bucket = [signBit](const int v) {
    const int b = abs(v) / 12;
    return signBit ? b * 3 + 1 + static_cast<int>(v > 0) - static_cast<int>(v < 0) : b;
    };
  const int kA0 = bucket(alAdv[0]);
  const int kA1 = bucket(alAdv[1]);
  const int kA2 = bucket(alAdv[2]);
  const int kA3 = bucket(alAdv[3]);
  const int kL0 = bucket(alLcp[0]);
  const int kL1 = bucket(alLcp[1]);
  const int kL2 = bucket(alLcp[2]);
  const int kL3 = bucket(alLcp[3]);
  const int kL4 = bucket(alLcp[4]);
  // The keys based on neighbour blocks also include nbAvail, so that the
  // replaced or zeroed predictions at the image border get their own entries.
  // nbAvail is 3 inside the image, so there it only changes the hash, and does
  // not split the entries. lcp[] only reads inside the block, so its keys do
  // not need it.

  const int pr0 = m1->p();
  const int s0 = stretch(pr0);
  m.add(s0 >> 1);
  m.add((pr0 - 2048) >> 3);

  const int pr1 = apm1.p(pr0, finalize64(hash(symKey, coef), 17));
  const int s1 = refine(pr1, s0);

  // chain: extrapolations from both neighbours, then from the west
  const int pr2 = apm2.p(pr1, finalize64(hash(symKey, kA1, nbAvail), 14));
  const int s2 = refine(pr2, s1);
  const int pr3 = apm3.p(pr2, finalize64(hash(symKey, kA0, nbAvail), 14));
  refine(pr3, s2);

  // chain: extrapolation from the north, then from both (advPred[3] style)
  const int pr4 = apm4.p(pr1, finalize64(hash(symKey, kA2, nbAvail), 14));
  const int s4 = refine(pr4, s1);
  const int pr5 = apm5.p(pr4, finalize64(hash(symKey, kA3, nbAvail), 14));
  refine(pr5, s4);

  // chain: the in-block neighbours
  const int pr6 = apm6.p(pr1, finalize64(hash(symKey, kL0), 14));
  const int s6 = refine(pr6, s1);
  const int pr7 = apm7.p(pr6, finalize64(hash(symKey, kL1), 14));
  const int s7 = refine(pr7, s6);
  const int pr8 = apm8.p(pr7, finalize64(hash(symKey, kL2), 14));
  const int s8 = refine(pr8, s7);
  const int pr9 = apm9.p(pr8, finalize64(hash(symKey, kL3), 14));
  const int s9 = refine(pr9, s8);
  const int pr10 = apm10.p(pr9, finalize64(hash(symKey, kL4), 14));
  refine(pr10, s9);

  // Pairs of APMs on pr0, each on two features. The second APM also refines
  // pr0, but the mixer gets its difference from the first APM's output instead
  // of from pr0, so the two inputs overlap less.
  const int pr11 = apm11.p(pr0, finalize64(hash(symKey, kL0, kL1), 15));
  const int s11 = refine(pr11, s0);
  const int pr12 = apm12.p(pr0, finalize64(hash(symKey, kA0, kA1, nbAvail), 15));
  refine(pr12, s11);

  const int pr13 = apm13.p(pr0, finalize64(hash(symKey, kA1, kA2, nbAvail), 15));
  const int s13 = refine(pr13, s0);
  const int pr14 = apm14.p(pr0, finalize64(hash(symKey, kA2, kA3, nbAvail), 15));
  refine(pr14, s13);

  //////////////////////////////////////////////////////////////////////////////
  // Outer mixer contexts
  //
  // Each context set selects its own weights, so the mixer can weight the
  // inputs above differently depending on what is being decoded and how well
  // the model knows this part of the image.
  //////////////////////////////////////////////////////////////////////////////

  // For tuning:
  //int skip = (int)shared->tuning_param;
  //int k = 0;
  //k++; if (k == skip); else m.set(...

  // What is being decoded: the symbol, and the coefficient or frequency. The
  // most specific sets, so they only pay off on large files.
  m.set(finalize64(hash(symKey, coef), 13), 8192); // strong only for large files
  m.set(finalize64(hash(symKey, min(3, (zu + zv) / 3)), 13), 8192); // strong for large files
  //m.set(static_cast<uint32_t>(zu + zv < 5) | (static_cast<uint32_t>(huffBits > 8) << 1) | (static_cast<uint32_t>(firstCol) << 2), 8); //2*2*2 //weak
  //m.set((min(3, huffBits / 2) << 8) | coef, 1024); //256*4 //weak

  // How large the neighbour blocks say this coefficient should be, and how
  // large its already decoded neighbours in the block are.
  m.set((predCat << 6) | (min(15, abs(alAdv[0]) / 16) << 2) | min(3, huffBits), 1024); // 16*16*4
  m.set((min(15, abs(lcp[2]) / 16) << 8) | (min(15, abs(lcp[1]) / 16) << 4) | min(15, abs(lcp[0]) / 16), 4096); // 16*16*16 //strong for MJPEG

  // Where the next larger coefficient is expected.
  m.set((min(7, runPred[1]) << 6) | (min(7, runPred[0]) << 3) | min(7, runPred[2]), 512); //8*8*8

  // How many of the in-block neighbours do not exist at this frequency.
  m.set(bitCount(lma) * 3 + static_cast<int>(phase), 24); //0..7, 0..2 //mostly for small files

  // A Huffman code bit needs different weights than a magnitude bit.
  m.set(posInSym * 2 + static_cast<int>(comp == 0), 48); //strong for smaller files

  // Does the predicted size reach the size category of the decoded symbol?
  // The clearest available sign of whether the model is right about this
  // coefficient.
  const int sCategory = rs < 0 ? 0 : (rs & 15);
  const int catCtx = rs < 0
    ? min(15, static_cast<int>(predCat))
    : 16 + (min(3, max(-3, static_cast<int>(predCat) - sCategory)) + 3) * 4 + min(3, static_cast<int>(e));
  m.set(catCtx, 64); //medium, stronger for small files

  // Which coefficient of the block the decoder is at.
  m.set(static_cast<int>(mcuPos & 63) * 3 + static_cast<int>(phase), 192); //medium

  // How many contexts have never been seen. Early in an image most contexts
  // are still empty, and the mixer should rely on the maps and the APMs more.
  m.set(min(15, coldCount) * 3 + static_cast<int>(phase), 48); //strong, especially for smaller files

  // Which coefficient this bit belongs to. Differs from mcuPos during the extra
  // bits after a run of zeros.
  m.set(alZz * 3 + static_cast<int>(phase), 192); //strong
  m.set(min(7, zzu[alZz] + zzv[alZz]) * 24 + posInSym, 192);
  m.set(alZz * 4 + min(3, static_cast<int>(comp)), 256); //weak
}

// ===== src/model/MatchInfo.cpp =====

#include <cassert>
#include <algorithm>

bool MatchInfo::isInNoMatchMode() const {
  return length == 0 && !delta && lengthBak == 0;
}

bool MatchInfo::isInPreRecoveryMode() const {
  return length == 0 && !delta && lengthBak != 0;
}

bool MatchInfo::isInRecoveryMode() const {
  return length != 0 && lengthBak != 0;
}

uint32_t MatchInfo::recoveryModePos() const {
  assert(isInRecoveryMode()); //must be in recovery mode
  return length - lengthBak;
}

uint64_t MatchInfo::prio() const {
  return
    static_cast<uint64_t>(length != 0) << 49 | //normal mode (match)
    static_cast<uint64_t>(delta) << 48 | //delta mode
    static_cast<uint64_t>(delta ? lengthBak : length) << 32 | //the longer wins
    static_cast<uint64_t>(index); //the more recent wins
}

bool MatchInfo::isBetterThan(const MatchInfo* other) const {
  return this->prio() > other->prio();
}

void MatchInfo::update(Shared* shared, uint32_t minlen_rm) {
  if constexpr (false) {
    INJECT_SHARED_bpos
    INJECT_SHARED_blockPos
    printf("- pos %d %d  index %d  length %d  lengthBak %d  delta %d\n", blockPos, bpos, index, length, lengthBak, delta ? 1 : 0);
  }
  INJECT_SHARED_buf
  INJECT_SHARED_bpos
  if (length != 0) {
    const int expectedBit = (expectedByte >> ((8 - bpos) & 7)) & 1;
    INJECT_SHARED_y
    if (y != expectedBit) {
      if (isInRecoveryMode()) { // another mismatch in recovery mode -> give up
        lengthBak = 0;
        indexBak = 0;
      }
      else { //backup match information: maybe we can recover it just after this mismatch
        lengthBak = length;
        indexBak = index;
        delta = true; //enter into delta mode - for the remaining bits in this byte length will be 0; we will exit delta mode and enter into recovery mode on bpos==0
      }
      length = 0;
    }
  }

  if (bpos == 0) {

    // recover match after a 1-byte mismatch
    if (isInPreRecoveryMode()) { // just exited delta mode, so we have a backup
      //the match failed 2 bytes ago, we must increase indexBak by 2:
      indexBak++;
      if (lengthBak < 65535) {
        lengthBak++;
      }
      INJECT_SHARED_c1
      if (buf[indexBak] == c1) { // match continues -> recover
        length = lengthBak;
        index = indexBak;
      }
      else { // still mismatch
        lengthBak = indexBak = 0; // purge backup (give up)
      }
    }

    // extend current match
    if (length != 0) {
      index++;
      if (length < 65535) {
        length++;
      }
      if (isInRecoveryMode() && recoveryModePos() >= minlen_rm) { // recovery seems to be successful and stable -> exit recovery mode
        lengthBak = indexBak = 0; // purge backup
      }
    }
    delta = false;
  }
  if constexpr (false) {
    INJECT_SHARED_bpos
    INJECT_SHARED_blockPos
    printf("  pos %d %d  index %d  length %d  lengthBak %d  delta %d\n", blockPos, bpos, index, length, lengthBak, delta ? 1 : 0);
  }

}

void MatchInfo::registerMatch(const uint32_t pos, const uint32_t LEN, const uint32_t LEN1) {
  assert(pos != 0);
  length = LEN - LEN1 + 1; // rebase
  index = pos;
  lengthBak = indexBak = 0;
  expectedByte = 0;
  delta = false;
}

// ===== src/model/MatchModel.cpp =====

MatchModel::MatchModel(Shared* const sh, const uint64_t hashtablesize, const uint64_t mapmemorysize) : 
  shared(sh),
  hashtable(hashtablesize),
  stateMaps {{sh, 1, 28 * 2 * 8,    1023, StateMapType::Generic},
             {sh, 1, 256 * 8 * 256, 1023, StateMapType::Generic},
             {sh, 1, 256 * 256,     1023, StateMapType::Generic}},
  cm(sh, mapmemorysize, nCM, 64),
  mapL /* LargeStationaryMap : Contexts, HashBits, Scale=64, Rate=16  */
        {sh, nLSM, 20}, // effective bits: ~22
  map { /* StationaryMap : BitsOfContext, InputBits, Scale=64, Rate=16  */
        {sh, 1 /*< leading bit */ + iCtxBits + 5 /*< mode5 */ + 3 /*< bpos*/, 1}
  }, 
  iCtx{5 + 3, 1, iCtxBits}, /* IndirectContext: bitsPerContext, inputBits, contextBits */
  hashBits(ilog2(uint32_t(hashtable.size())))
{
  assert(isPowerOf2(hashtablesize));
  assert(isPowerOf2(mapmemorysize));
}

void MatchModel::update() {

  //update active candidates, remove dead candidates
  size_t n = max(numberOfActiveCandidates, 1);
  for (size_t i = 0; i < n; i++) {
    MatchInfo* matchInfo = &matchCandidates[i];
    matchInfo->update(shared, MINLEN_RM);
    if (numberOfActiveCandidates != 0 && matchInfo->isInNoMatchMode()) {
      numberOfActiveCandidates--;
      if (numberOfActiveCandidates == i)
        break;
      memmove(&matchCandidates[i], &matchCandidates[i + 1], (numberOfActiveCandidates - i) * sizeof(MatchInfo));
      i--;
    }
  }

  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    
    INJECT_SHARED_pos
    uint64_t hash;
    HashElementForMatchPositions* matches;

    hash = shared->State.NormalModel.cxt[LEN3];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN3); //longest
    matches->Add(pos);

    hash = shared->State.NormalModel.cxt[LEN2];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN2); //middle
    matches->Add(pos);

    hash = shared->State.NormalModel.cxt[LEN1];
    matches = &hashtable[finalize64(hash, hashBits)];
    if (numberOfActiveCandidates < N)
      AddCandidates(matches, LEN1); //shortest
    matches->Add(pos);

    INJECT_SHARED_buf
    for (size_t i = 0; i < numberOfActiveCandidates; i++) {
      matchCandidates[i].expectedByte = buf[matchCandidates[i].index];
    }
  }
}

void MatchModel::mix(Mixer &m) {
  update();

  for( uint32_t i = 0; i < nST; i++ ) { // reset contexts
    ctx[i] = 0;
  }
  
  size_t bestCandidateIdx = 0; //default item is the first candidate, let's see if any other candidate is better
  for (size_t i = 1; i < numberOfActiveCandidates; i++) {
    if (matchCandidates[i].isBetterThan(&matchCandidates[bestCandidateIdx]))
      bestCandidateIdx = i;
  }

  const uint32_t length = matchCandidates[bestCandidateIdx].length;
  const uint8_t expectedByte = matchCandidates[bestCandidateIdx].expectedByte;
  const bool isInNoMatchMode= matchCandidates[bestCandidateIdx].isInNoMatchMode();
  const bool isInDeltaMode = matchCandidates[bestCandidateIdx].delta;
  const bool isInPreRecoveryMode = matchCandidates[bestCandidateIdx].isInPreRecoveryMode();
  const bool isInRecoveryMode = matchCandidates[bestCandidateIdx].isInRecoveryMode();

  INJECT_SHARED_bpos
  INJECT_SHARED_c0
  INJECT_SHARED_c1
  const int expectedBit = length != 0 ? (expectedByte >> (7 - bpos)) & 1 : 0;

  uint32_t n0 = 0;
  uint32_t n1 = 0;
  for (size_t i = 0; i < numberOfActiveCandidates; i++) {
    if (matchCandidates[i].length == 0)
      continue;
    int expectedBit = (matchCandidates[i].expectedByte >> (7 - bpos)) & 1;
    n0 += 1 - expectedBit;
    n1 += expectedBit;
  }
  
  const int isUncertain = static_cast<int>(n0 != 0 && n1 != 0);

  uint32_t denselength = 0; // 0..27
  if (length != 0) {
    if (length <= 16) {
      denselength = length - 1; // 0..15
    } else {
      denselength = 12 + (min(length - 1, 63) >> 2); // 16..27
    }
    ctx[0] = 1 + ((denselength << 4) | (expectedBit << 3) | bpos); // 1 .. 28*2*8+1
    ctx[1] = 1 + ((expectedByte << 11) | (bpos << 8) | c1);        // 1 .. 256*8*256+1
    const int sign = 2 * expectedBit - 1;
    m.add(sign * (min(length, 32) << 5)); // +/- 32..1024
    m.add(sign * (ilog->log(min(length, 65535)) << 2)); // +/-  0..1024
  } else { // no match at all or delta mode
    m.add(0);
    m.add(0);
  }

  if( isInDeltaMode ) { // delta mode: helps predicting the remaining bits of a character when a mismatch occurs
    ctx[2] = (expectedByte << 8) | c0;
  }

  for( uint32_t i = 0; i < nST; i++ ) {
    const uint32_t c = ctx[i];
    if( c != 0 ) {
      const int p1 = stateMaps[i].p1(c - 1);
      const int st = stretch(p1);
      m.add(st >> 2);
      m.add((p1 - 2048) >> 3);
    } else {
      m.add(0);
      m.add(0);
    }
  }

  const uint32_t length2 = min(length >> 2, 3); //0-3 (2 bits)

  const uint8_t mode3 = 
    isInNoMatchMode ? 0 :
    isInDeltaMode ? 1 :
    isInPreRecoveryMode ? 2 :
    isInRecoveryMode ? 3 :
    4 + length2; //0..7 //we don't need expectedBit and isUncertain as this context is used on the Byte-level
  
  const uint32_t mode5 =
    isInNoMatchMode ? 0 :
    isInDeltaMode ? 1 :
    isInPreRecoveryMode ? 2 :
    isInRecoveryMode ? 3 :
    4 + (length2 << 2 | expectedBit << 1 | isUncertain); //0..19

  //bytewise contexts
  INJECT_SHARED_c4
  if( bpos == 0 ) {
    const uint8_t R_ = CM_USE_RUN_STATS;
    cm.set(R_, hash((length != 0 ? expectedByte : c1) << 3 | mode3)); //max context bits: 8+8+3 = 19
    cm.set(R_, hash(length != 0 ? expectedByte : ((c4 >> 8) & 0xff), (c1 << 3) | mode3)); //max context bits: 8+8+8+3=27
  }
  cm.mix(m);

  //bitwise contexts
  mapL.set(hash(expectedByte, c0, c4 & 0x00ffffff, mode5)); // max context bits: 8+8+24+3 = 43 bits -> hashed into ~22 bits
  mapL.mix(m);

  INJECT_SHARED_y
  iCtx += y;
  iCtx = mode5 << 3 | bpos; // 5+3 = 8 bits
  map[0].set(iCtx() << 8 | mode5 << 3 | bpos); // (max 6 bits + 1 leading bit) + 5 + 3 = 15 bits
  map[0].mix(m);

  m.set(mode5, 20);

  shared->State.Match.length2 =
    isInDeltaMode ? 1 :
    length == 0 ? 0 :
    length <= 7 ? 2 : 3;
  shared->State.Match.mode3 = mode3; 
  shared->State.Match.mode5 = mode5;
  shared->State.Match.expectedByte = length != 0 ? expectedByte : 256;
}

bool MatchModel::isMatch(const uint32_t pos, const uint32_t MINLEN) const {
  INJECT_SHARED_buf
    for (uint32_t length = 1; length <= MINLEN; length++) {
      if (buf(length) != buf[pos - length])
        return false;
    }
  return true;
}

void MatchModel::AddCandidates(HashElementForMatchPositions* matches, uint32_t LEN) {
  uint32_t i = 0;
  INJECT_SHARED_pos
    while (numberOfActiveCandidates < N && i < HashElementForMatchPositions::N) {
      uint32_t matchpos = matches->matchPositions[i];
      if (matchpos == 0)
        break;
      if (isMatch(matchpos, LEN)) {
        bool isSame = false;
        //is this position already registered?
        for (uint32_t j = 0; j < numberOfActiveCandidates; j++) {
          MatchInfo* oldcandidate = &matchCandidates[j];
          isSame = oldcandidate->index == matchpos;
          if (isSame)
            break;
        }
        if (!isSame) { //don't register an already registered sequence
          matchCandidates[numberOfActiveCandidates].registerMatch(matchpos, LEN, LEN1);
          numberOfActiveCandidates++;
        }
      }
      i++;
    }
}

// ===== src/model/NormalModel.cpp =====

NormalModel::NormalModel(Shared* const sh, const uint64_t cmSize) :
  shared(sh), cm(sh, cmSize, nCM, 64),
  smOrder0(sh, 255, 1023, 16),
  smOrder1(sh, 255 * 256, 1023, 32)
{
  assert(isPowerOf2(cmSize));
}

void NormalModel::reset() {
  memset(&shared->State.NormalModel.cxt[0], 0, sizeof(shared->State.NormalModel.cxt));
}

void NormalModel::updateHashes() {
  INJECT_SHARED_c1
  INJECT_SHARED_blockType
  uint64_t base_context = c1;
  BlockType normalizedBlockType = blockType;
  if (blockType == BlockType::JPEG && shared->State.JPEG.state == 0)
    normalizedBlockType = BlockType::DEFAULT;
  else if (blockType == BlockType::IMAGE24 || blockType == BlockType::IMAGE32) {
    base_context = base_context << 2 | shared->State.Image.plane;
  }
  base_context = (base_context * (uint64_t)BlockType::Count) + (uint64_t)normalizedBlockType;
  uint64_t* cxt = shared->State.NormalModel.cxt;
  for( uint64_t i = 14; i > 0; --i ) {
    cxt[i] = (cxt[i - 1] + base_context + i) * PHI64;
  }
}

void NormalModel::mix(Mixer &m) {
  INJECT_SHARED_bpos
  if( bpos == 0 ) {
    updateHashes();
    uint64_t* cxtHashes = shared->State.NormalModel.cxt;
    const uint8_t RH = CM_USE_RUN_STATS | CM_USE_BYTE_HISTORY;
    for(uint64_t i = 1; i <= 6; ++i ) {
      cm.set(RH, cxtHashes[i]);
    }
    cm.set(RH, cxtHashes[8]); 
    cm.set(RH, cxtHashes[11]);
    cm.set(RH, cxtHashes[14]);
  }
  cm.mix(m);

  INJECT_SHARED_c0
  INJECT_SHARED_c1

  int pr_slow, pr_fast, st;
  int ct1 = (c0 - 1);
  smOrder0.p(ct1, pr_slow, pr_fast);
  m.add((pr_slow - 2048) >> 3); st = stretch(pr_slow); m.add(st >> 2);
  m.add((pr_fast - 2048) >> 3); st = stretch(pr_fast); m.add(st >> 2);
  int ct2 = ct1 << 8 | c1;
  smOrder1.p(ct2, pr_slow, pr_fast);
  m.add((pr_slow - 2048) >> 3); st = stretch(pr_slow); m.add(st >> 2);
  m.add((pr_fast - 2048) >> 3); st = stretch(pr_fast); m.add(st >> 2);

  const int order = max(0, cm.order - (nCM - 7)); //0-7
  assert(0 <= order && order <= 7);
  m.set(order << 3 | bpos, 64);
  shared->State.NormalModel.order = order;
}

void NormalModel::mixPost(Mixer &m) {
  INJECT_SHARED_c4
  uint32_t c2 = (c4 >> 8) & 0xff;
  uint32_t c3 = (c4 >> 16) & 0xff;
  uint32_t c;

  INJECT_SHARED_c0
  INJECT_SHARED_c1
  INJECT_SHARED_bpos
  INJECT_SHARED_blockType
  m.set((c1 | (bpos > 5) << 8 | (((c0 & ((1u << bpos) - 1)) == 0) || (c0 == ((2u << bpos) - 1))) << 9), 1024);
  m.set(c0, 256);
  // 0: generic, 1: JPEG, 2: raw image data, 3: container/stream block (HDR, ZLIB, GIF, PNG, RLE, LZW)
  uint32_t bt = blockType == BlockType::DEFAULT ? 0 :
                blockType == BlockType::JPEG ? 1 :
                (static_cast<int>(blockType) >= static_cast<int>(BlockType::IMAGE1) &&
                 static_cast<int>(blockType) <= static_cast<int>(BlockType::IMAGE32)) ? 2 : 3;
  m.set(shared->State.NormalModel.order | ((c1 >> 6) & 3) << 3 | (bpos == 0) << 5 | (c1 == c2) << 6 | bt << 7, 512);
  m.set(c2, 256);
  m.set(c3, 256);
  if( bpos != 0 ) {
    c = c0 << (8 - bpos);
    if( bpos == 1 ) {
      c |= c3 >> 1;
    }
    c = min(bpos, 5) << 8 | c1 >> 5 | (c2 >> 5) << 3 | (c & 192);
  } else {
    c = c3 >> 7 | (c4 >> 31) << 1 | (c2 >> 6) << 2 | (c1 & 240);
  }
  m.set(c, 1536);
}

// ===== src/Models.cpp =====

/*
 relationship between compression level, shared->mem and NormalModel memory use as an example

 level   shared->mem    NormalModel memory use (shared->mem*32)
 -----   -----------    ----------------------
   1      0.125 MB              4 MB
   2      0.25	MB              8 MB
   3      0.5   MB             16 MB
   4      1.0   MB             32 MB
   5      2.0   MB             64 MB
   6      4.0   MB            128 MB
   7      8.0   MB            256 MB
   8     16.0   MB            512 MB
   9     32.0   MB           1024 MB
   10     64.0   MB           2048 MB
   11    128.0   MB           4096 MB
   12    256.0   MB           4096 MB (capped)

*/


Models::Models(Shared* const sh, MixerFactory* mf) : shared(sh), mixerFactory(mf) {}

auto Models::normalModel() -> NormalModel & {
  static NormalModel instance {shared, min(shared->mem * 32, UINT64_C(1) << 32 )}; //cap at 4 GB
  return instance;
}

auto Models::matchModel() -> MatchModel & {
  static MatchModel instance {shared, shared->mem / 4 /*hashtablesize*/, shared->mem /*mapmemorysize*/ }; /**< Not the actual memory use - see in the model */
  return instance;
}

auto Models::jpegModel() -> JpegModel & {
  static JpegModel instance {shared, mixerFactory, shared->mem}; /**< Not the actual memory use - see in the model */
  return instance;
}

auto Models::image24BitModel() -> Image24BitModel & {
  static Image24BitModel instance {shared, shared->mem * 4};
  return instance;
}

auto Models::image8BitModel() -> Image8BitModel & {
  static Image8BitModel instance {shared, shared->mem * 4};
  return instance;
}

auto Models::image4BitModel() -> Image4BitModel & {
  static Image4BitModel instance {shared, shared->mem / 2};
  return instance;
}

auto Models::image1BitModel() -> Image1BitModel & {
  static Image1BitModel instance {shared};
  return instance;
}


// ===== src/MTFList.hpp =====

#include <cstdint>

/**
 * Move To Front List
 */
class MTFList {
private:
  int root, Index;
  Array<int, 16> previous;
  Array<int, 16> next;
public:
  explicit MTFList(uint16_t n);
  int getFirst();
  int getNext();
  void moveToFront(int i);
};


// ===== src/MTFList.cpp =====

MTFList::MTFList(const uint16_t n) : root(0), Index(0), previous(n), next(n) {
  assert(n > 0);
  for( int i = 0; i < n; i++ ) {
    previous[i] = i - 1;
    next[i] = i + 1;
  }
  next[n - 1] = -1;
}

int MTFList::getFirst() {
  return Index = root;
}

int MTFList::getNext() {
  if( Index >= 0 ) {
    Index = next[Index];
  }
  return Index;
}

void MTFList::moveToFront(const int i) {
  assert(uint32_t(i) < previous.size());
  if((Index = i) == root ) {
    return;
  }
  const int p = previous[Index];
  const int n = next[Index];
  if( p >= 0 ) {
    next[p] = next[Index];
  }
  if( n >= 0 ) {
    previous[n] = previous[Index];
  }
  previous[root] = Index;
  next[Index] = root;
  root = Index;
  previous[root] = -1;
}

// ===== src/OLS_float_Scalar.hpp =====


/**
 * Scalar implementation of OLS for float precision
 * Inherits from OLS<float> and overrides some methods for optimization
 * Primary goal: order of operations need to match with that of the SSE2 code path for binary compatibility
 * Secondary goal: it's still faster than the naive reference implementation due to better cache locality.
 */
class OLS_float_Scalar : public OLS<float>
{
public:
  OLS_float_Scalar(size_t n, size_t solveInterval, float lambda, float nu);

  float predict() override;
  void update(float y) override;

protected:
  bool factor() override;
  void solve() override;
};

// ===== src/OLS_float_SSE3.hpp =====


#ifdef X64_SIMD_AVAILABLE

/**
 * SSE2 implementation of OLS for float precision
 * Inherits from OLS_float_Scalar and overrides some methods for optimization
 */
class OLS_float_SSE3 : public OLS_float_Scalar
{
public:
  OLS_float_SSE3(size_t n, size_t solveInterval, float lambda, float nu);

  float predict() override;

protected:
  bool factor() override;
};

#endif

// ===== src/OLS_factory.cpp =====

#ifdef X64_SIMD_AVAILABLE
#endif

std::unique_ptr<OLS_float> create_OLS_float(SIMDType simd, size_t n, size_t solveInterval, float lambda, float nu) {
#ifdef X64_SIMD_AVAILABLE
  if (simd >= SIMDType::SIMD_SSE3)
    return std::make_unique<OLS_float_SSE3>(n, solveInterval, lambda, nu);
  else
#endif
    return std::make_unique<OLS_float_Scalar>(n, solveInterval, lambda, nu);
}

// ===== src/OLS_float_Scalar.cpp =====
#include <cmath>


OLS_float_Scalar::OLS_float_Scalar(size_t n, size_t solveInterval, float lambda, float nu)
  : OLS<float>(n, solveInterval, lambda, nu) {
}

float OLS_float_Scalar::predict() {
  assert(featureIndex == n);
  featureIndex = 0;

  // Prediction: y = xᵀ·w
  // Process 4 floats at a time to match alignment
  float sum0 = 0.0f;
  float sum1 = 0.0f;
  float sum2 = 0.0f;
  float sum3 = 0.0f;

  for (size_t i = 0; i < nPadded; i += 4) {
    sum0 += x[i] * w[i];
    sum1 += x[i + 1] * w[i + 1];
    sum2 += x[i + 2] * w[i + 2];
    sum3 += x[i + 3] * w[i + 3];
  }

  // Horizontal sum
  float sum02 = sum0 + sum2;
  float sum13 = sum1 + sum3;
  float sum = sum02 + sum13;
  return sum;
}

void OLS_float_Scalar::update(float y) {
  const float alpha = lambda;      // Retention factor
  const float beta = 1.0f - lambda; // Forgetting factor

  // The following code is auto-vectorized to SSE2 on x64 by GCC (optimal)

#if defined(__GNUC__)
  // Tell compiler these pointers are 32-byte aligned
  float* __restrict__ C_aligned = static_cast<float*>(__builtin_assume_aligned(&mCovariance[0], 32));
  float* __restrict__ b_aligned = static_cast<float*>(__builtin_assume_aligned(&b[0], 32));
  const float* __restrict__ x_aligned = static_cast<float*>(__builtin_assume_aligned(&x[0], 32));
#else
  float* C_aligned = &mCovariance[0];
  float* b_aligned = &b[0];
  const float* x_aligned = &x[0];
#endif

  ASSUME((nPadded & 3) == 0); // hint: help the compiler to vectorize better

  // C = λC + (1-λ)x·xᵀ (lower triangle only)
  size_t i_n = 0; // i * nPadded
  for (size_t i = 0; i < n; i++) {
    float xi_beta = x_aligned[i] * beta;
    for (size_t j = 0; j <= i; j++) {  // Only compute lower triangle
      C_aligned[i_n + j] = alpha * C_aligned[i_n + j] + (x_aligned[j] * xi_beta);
    }
    i_n += nPadded;
  }

  // b = λb + (1-λ)y·x
  for (size_t i = 0; i < nPadded; i++) {
    b_aligned[i] = alpha * b_aligned[i] + y * x_aligned[i] * beta;
  }

  // Periodic re-solve
  // C·w = b
  samplesSinceLastSolve++;
  if (samplesSinceLastSolve >= solveInterval) {
    bool isSuccess = factor();
    if (isSuccess) {
      solve();
    }
    samplesSinceLastSolve = 0;
  }
}

bool OLS_float_Scalar::factor() {

  // Copy lower triangle only + apply regularization
  size_t i_n = 0;
  for (size_t i = 0; i < n; i++) {
    // Copy elements below diagonal
    for (size_t j = 0; j < i; j++) {
      mCholesky[i_n + j] = mCovariance[i_n + j];
    }

    // Diagonal with regularization
    mCholesky[i_n + i] = mCovariance[i_n + i] + nu;
    i_n += nPadded;
  }

  // Cholesky factorization: C = L·Lᵀ
  // Decomposes regularized covariance matrix into lower triangular L
  const size_t nb = n >> 2;  // Number of 4x4 blocks
  const size_t b = nb << 2;  // First boundary row

  float* krow0 = &mCholesky[0];           // row 4k
  float* krow1 = krow0 + nPadded;         // row 4k+1
  float* krow2 = krow1 + nPadded;         // row 4k+2
  float* krow3 = krow2 + nPadded;         // row 4k+3

  for (size_t k = 0; k < nb; k++) {
    size_t kc = 4 * k;

    // --------------------------------------------------
    // 1) Full diagonal update (4x4 block)
    // --------------------------------------------------

    // Load diagonal values to be updated
    float krow0_kc0 = krow0[kc];

    float krow1_kc0 = krow1[kc];
    float krow1_kc1 = krow1[kc + 1];

    float krow2_kc0 = krow2[kc];
    float krow2_kc1 = krow2[kc + 1];
    float krow2_kc2 = krow2[kc + 2];

    float krow3_kc0 = krow3[kc];
    float krow3_kc1 = krow3[kc + 1];
    float krow3_kc2 = krow3[kc + 2];
    float krow3_kc3 = krow3[kc + 3];

    for (size_t p = 0; p < k; p++) {
      size_t pc = 4 * p;

      float a00 = krow0[pc];
      float a01 = krow0[pc + 1];
      float a02 = krow0[pc + 2];
      float a03 = krow0[pc + 3];

      float a10 = krow1[pc];
      float a11 = krow1[pc + 1];
      float a12 = krow1[pc + 2];
      float a13 = krow1[pc + 3];

      float a20 = krow2[pc];
      float a21 = krow2[pc + 1];
      float a22 = krow2[pc + 2];
      float a23 = krow2[pc + 3];

      float a30 = krow3[pc];
      float a31 = krow3[pc + 1];
      float a32 = krow3[pc + 2];
      float a33 = krow3[pc + 3];

      // Update krow0[kc]
      krow0_kc0 -= ((a00 * a00 + a01 * a01) + (a02 * a02 + a03 * a03));

      // Update krow1[kc:kc+1]
      krow1_kc0 -= ((a10 * a00 + a11 * a01) + (a12 * a02 + a13 * a03));
      krow1_kc1 -= ((a10 * a10 + a11 * a11) + (a12 * a12 + a13 * a13));

      // Update krow2[kc:kc+2]
      krow2_kc0 -= ((a20 * a00 + a21 * a01) + (a22 * a02 + a23 * a03));
      krow2_kc1 -= ((a20 * a10 + a21 * a11) + (a22 * a12 + a23 * a13));
      krow2_kc2 -= ((a20 * a20 + a21 * a21) + (a22 * a22 + a23 * a23));

      // Update krow3[kc:kc+3]
      krow3_kc0 -= ((a30 * a00 + a31 * a01) + (a32 * a02 + a33 * a03));
      krow3_kc1 -= ((a30 * a10 + a31 * a11) + (a32 * a12 + a33 * a13));
      krow3_kc2 -= ((a30 * a20 + a31 * a21) + (a32 * a22 + a33 * a23));
      krow3_kc3 -= ((a30 * a30 + a31 * a31) + (a32 * a32 + a33 * a33));
    }

    // Store results back
    krow0[kc] = krow0_kc0;

    krow1[kc] = krow1_kc0;
    krow1[kc + 1] = krow1_kc1;

    krow2[kc] = krow2_kc0;
    krow2[kc + 1] = krow2_kc1;
    krow2[kc + 2] = krow2_kc2;

    krow3[kc] = krow3_kc0;
    krow3[kc + 1] = krow3_kc1;
    krow3[kc + 2] = krow3_kc2;
    krow3[kc + 3] = krow3_kc3;

    // --------------------------------------------------
    // 2) Factor diagonal 4x4 block
    // --------------------------------------------------

    // Column 0
    float d00 = krow0[kc];
    if (d00 <= min_diagonal)
      goto fail;
    d00 = sqrt(d00);
    krow0[kc] = d00;

    float d10 = krow1[kc] / d00;
    krow1[kc] = d10;

    float d20 = krow2[kc] / d00;
    krow2[kc] = d20;

    float d30 = krow3[kc] / d00;
    krow3[kc] = d30;

    // Column 1
    float d11 = krow1[kc + 1] - d10 * d10;
    if (d11 <= min_diagonal)
      goto fail;
    d11 = sqrt(d11);
    krow1[kc + 1] = d11;

    float d21 = (krow2[kc + 1] - d20 * d10) / d11;
    krow2[kc + 1] = d21;

    float d31 = (krow3[kc + 1] - d30 * d10) / d11;
    krow3[kc + 1] = d31;

    // Column 2
    float d22 = krow2[kc + 2] - d20 * d20 - d21 * d21;
    if (d22 <= min_diagonal)
      goto fail;
    d22 = sqrt(d22);
    krow2[kc + 2] = d22;

    float d32 = (krow3[kc + 2] - d30 * d20 - d31 * d21) / d22;
    krow3[kc + 2] = d32;

    // Column 3
    float d33 = krow3[kc + 3] - d30 * d30 - d31 * d31 - d32 * d32;
    if (d33 <= min_diagonal)
      goto fail;
    d33 = sqrt(d33);
    krow3[kc + 3] = d33;

    // --------------------------------------------------
    // 3) Panel update + TRSM (4x4 blocks)
    // --------------------------------------------------
    float* irow0 = krow0 + 4 * nPadded;
    float* irow1 = irow0 + nPadded;
    float* irow2 = irow1 + nPadded;
    float* irow3 = irow2 + nPadded;

    for (size_t i = k + 1; i < nb; i++) {

      // Load the 4x4 block we're updating
      float irow0_kc0 = irow0[kc];
      float irow0_kc1 = irow0[kc + 1];
      float irow0_kc2 = irow0[kc + 2];
      float irow0_kc3 = irow0[kc + 3];

      float irow1_kc0 = irow1[kc];
      float irow1_kc1 = irow1[kc + 1];
      float irow1_kc2 = irow1[kc + 2];
      float irow1_kc3 = irow1[kc + 3];

      float irow2_kc0 = irow2[kc];
      float irow2_kc1 = irow2[kc + 1];
      float irow2_kc2 = irow2[kc + 2];
      float irow2_kc3 = irow2[kc + 3];

      float irow3_kc0 = irow3[kc];
      float irow3_kc1 = irow3[kc + 1];
      float irow3_kc2 = irow3[kc + 2];
      float irow3_kc3 = irow3[kc + 3];

      for (size_t p = 0; p < k; p++) {
        size_t pc = 4 * p;

        float aik0 = irow0[pc];
        float aik1 = irow0[pc + 1];
        float aik2 = irow0[pc + 2];
        float aik3 = irow0[pc + 3];

        float aik4 = irow1[pc];
        float aik5 = irow1[pc + 1];
        float aik6 = irow1[pc + 2];
        float aik7 = irow1[pc + 3];

        float aik8 = irow2[pc];
        float aik9 = irow2[pc + 1];
        float aik10 = irow2[pc + 2];
        float aik11 = irow2[pc + 3];

        float aik12 = irow3[pc];
        float aik13 = irow3[pc + 1];
        float aik14 = irow3[pc + 2];
        float aik15 = irow3[pc + 3];

        float b00 = krow0[pc];
        float b01 = krow0[pc + 1];
        float b02 = krow0[pc + 2];
        float b03 = krow0[pc + 3];

        float b10 = krow1[pc];
        float b11 = krow1[pc + 1];
        float b12 = krow1[pc + 2];
        float b13 = krow1[pc + 3];

        float b20 = krow2[pc];
        float b21 = krow2[pc + 1];
        float b22 = krow2[pc + 2];
        float b23 = krow2[pc + 3];

        float b30 = krow3[pc];
        float b31 = krow3[pc + 1];
        float b32 = krow3[pc + 2];
        float b33 = krow3[pc + 3];

        // Update irow0
        irow0_kc0 -= ((aik0 * b00 + aik1 * b01) + (aik2 * b02 + aik3 * b03));
        irow0_kc1 -= ((aik0 * b10 + aik1 * b11) + (aik2 * b12 + aik3 * b13));
        irow0_kc2 -= ((aik0 * b20 + aik1 * b21) + (aik2 * b22 + aik3 * b23));
        irow0_kc3 -= ((aik0 * b30 + aik1 * b31) + (aik2 * b32 + aik3 * b33));

        // Update irow1
        irow1_kc0 -= ((aik4 * b00 + aik5 * b01) + (aik6 * b02 + aik7 * b03));
        irow1_kc1 -= ((aik4 * b10 + aik5 * b11) + (aik6 * b12 + aik7 * b13));
        irow1_kc2 -= ((aik4 * b20 + aik5 * b21) + (aik6 * b22 + aik7 * b23));
        irow1_kc3 -= ((aik4 * b30 + aik5 * b31) + (aik6 * b32 + aik7 * b33));

        // Update irow2
        irow2_kc0 -= ((aik8 * b00 + aik9 * b01) + (aik10 * b02 + aik11 * b03));
        irow2_kc1 -= ((aik8 * b10 + aik9 * b11) + (aik10 * b12 + aik11 * b13));
        irow2_kc2 -= ((aik8 * b20 + aik9 * b21) + (aik10 * b22 + aik11 * b23));
        irow2_kc3 -= ((aik8 * b30 + aik9 * b31) + (aik10 * b32 + aik11 * b33));

        // Update irow3
        irow3_kc0 -= ((aik12 * b00 + aik13 * b01) + (aik14 * b02 + aik15 * b03));
        irow3_kc1 -= ((aik12 * b10 + aik13 * b11) + (aik14 * b12 + aik15 * b13));
        irow3_kc2 -= ((aik12 * b20 + aik13 * b21) + (aik14 * b22 + aik15 * b23));
        irow3_kc3 -= ((aik12 * b30 + aik13 * b31) + (aik14 * b32 + aik15 * b33));
      }

      // TRSM (triangular solve for 4x4 block)
      // Column 0
      float col0_0 = irow0_kc0 / d00;
      float col0_1 = irow1_kc0 / d00;
      float col0_2 = irow2_kc0 / d00;
      float col0_3 = irow3_kc0 / d00;

      // Column 1
      float col1_0 = (irow0_kc1 - d10 * col0_0) / d11;
      float col1_1 = (irow1_kc1 - d10 * col0_1) / d11;
      float col1_2 = (irow2_kc1 - d10 * col0_2) / d11;
      float col1_3 = (irow3_kc1 - d10 * col0_3) / d11;

      // Column 2
      float col2_0 = (irow0_kc2 - d20 * col0_0 - d21 * col1_0) / d22;
      float col2_1 = (irow1_kc2 - d20 * col0_1 - d21 * col1_1) / d22;
      float col2_2 = (irow2_kc2 - d20 * col0_2 - d21 * col1_2) / d22;
      float col2_3 = (irow3_kc2 - d20 * col0_3 - d21 * col1_3) / d22;

      // Column 3
      float col3_0 = (irow0_kc3 - d30 * col0_0 - d31 * col1_0 - d32 * col2_0) / d33;
      float col3_1 = (irow1_kc3 - d30 * col0_1 - d31 * col1_1 - d32 * col2_1) / d33;
      float col3_2 = (irow2_kc3 - d30 * col0_2 - d31 * col1_2 - d32 * col2_2) / d33;
      float col3_3 = (irow3_kc3 - d30 * col0_3 - d31 * col1_3 - d32 * col2_3) / d33;

      // Store results
      irow0[kc] = col0_0;
      irow0[kc + 1] = col1_0;
      irow0[kc + 2] = col2_0;
      irow0[kc + 3] = col3_0;

      irow1[kc] = col0_1;
      irow1[kc + 1] = col1_1;
      irow1[kc + 2] = col2_1;
      irow1[kc + 3] = col3_1;

      irow2[kc] = col0_2;
      irow2[kc + 1] = col1_2;
      irow2[kc + 2] = col2_2;
      irow2[kc + 3] = col3_2;

      irow3[kc] = col0_3;
      irow3[kc + 1] = col1_3;
      irow3[kc + 2] = col2_3;
      irow3[kc + 3] = col3_3;

      irow0 += 4 * nPadded;
      irow1 += 4 * nPadded;
      irow2 += 4 * nPadded;
      irow3 += 4 * nPadded;
    }

    krow0 += 4 * nPadded;
    krow1 += 4 * nPadded;
    krow2 += 4 * nPadded;
    krow3 += 4 * nPadded;
  }

  // --------------------------------------------------
  // Boundary handling for remaining 1-3 rows
  // --------------------------------------------------

  // Process each boundary row
  for (size_t i = b; i < n; i++) {
    float* irow = &mCholesky[i * nPadded];

    // Panel update + TRSM for each 4x4 block
    float* krow0 = &mCholesky[0];
    float* krow1 = krow0 + nPadded;
    float* krow2 = krow1 + nPadded;
    float* krow3 = krow2 + nPadded;

    for (size_t k = 0; k < nb; k++) {
      size_t kc = k * 4;

      // Panel update: subtract contributions from previous blocks
      float sum0 = irow[kc];
      float sum1 = irow[kc + 1];
      float sum2 = irow[kc + 2];
      float sum3 = irow[kc + 3];

      for (size_t p = 0; p < k; p++) {
        size_t pc = p * 4;

        float a0 = irow[pc];
        float a1 = irow[pc + 1];
        float a2 = irow[pc + 2];
        float a3 = irow[pc + 3];

        float b00 = krow0[pc];
        float b01 = krow0[pc + 1];
        float b02 = krow0[pc + 2];
        float b03 = krow0[pc + 3];

        float b10 = krow1[pc];
        float b11 = krow1[pc + 1];
        float b12 = krow1[pc + 2];
        float b13 = krow1[pc + 3];

        float b20 = krow2[pc];
        float b21 = krow2[pc + 1];
        float b22 = krow2[pc + 2];
        float b23 = krow2[pc + 3];

        float b30 = krow3[pc];
        float b31 = krow3[pc + 1];
        float b32 = krow3[pc + 2];
        float b33 = krow3[pc + 3];

        sum0 -= ((a0 * b00 + a1 * b01) + (a2 * b02 + a3 * b03));
        sum1 -= ((a0 * b10 + a1 * b11) + (a2 * b12 + a3 * b13));
        sum2 -= ((a0 * b20 + a1 * b21) + (a2 * b22 + a3 * b23));
        sum3 -= ((a0 * b30 + a1 * b31) + (a2 * b32 + a3 * b33));
      }

      // TRSM: solve against block k's factored diagonal
      float d00 = krow0[kc];
      float d10 = krow1[kc];
      float d11 = krow1[kc + 1];
      float d20 = krow2[kc];
      float d21 = krow2[kc + 1];
      float d22 = krow2[kc + 2];
      float d30 = krow3[kc];
      float d31 = krow3[kc + 1];
      float d32 = krow3[kc + 2];
      float d33 = krow3[kc + 3];

      float col0 = sum0 / d00;
      float col1 = (sum1 - d10 * col0) / d11;
      float col2 = (sum2 - d20 * col0 - d21 * col1) / d22;
      float col3 = (sum3 - d30 * col0 - d31 * col1 - d32 * col2) / d33;

      irow[kc] = col0;
      irow[kc + 1] = col1;
      irow[kc + 2] = col2;
      irow[kc + 3] = col3;

      krow0 += 4 * nPadded;
      krow1 += 4 * nPadded;
      krow2 += 4 * nPadded;
      krow3 += 4 * nPadded;
    }
  }

  // Now scalar Cholesky for boundary rows among themselves
  i_n = b * nPadded;
  for (size_t i = b; i < n; i++) {

    // Off-diagonal: L[i,j] for j in [b, i)
    for (size_t j = b; j < i; j++) {
      size_t j_n = j * nPadded;
      float sum0 = 0.0f;
      float sum1 = 0.0f;
      float sum2 = 0.0f;
      float sum3 = 0.0f;

      size_t k_end = j & ~3ULL;
      size_t k = 0;
      for (; k < k_end; k += 4) {
        float a0 = mCholesky[i_n + k];
        float a1 = mCholesky[i_n + k + 1];
        float a2 = mCholesky[i_n + k + 2];
        float a3 = mCholesky[i_n + k + 3];

        float b0 = mCholesky[j_n + k];
        float b1 = mCholesky[j_n + k + 1];
        float b2 = mCholesky[j_n + k + 2];
        float b3 = mCholesky[j_n + k + 3];

        sum0 += a0 * b0;
        sum1 += a1 * b1;
        sum2 += a2 * b2;
        sum3 += a3 * b3;
      }

      float sum = mCholesky[i_n + j] - ((sum0 + sum2) + (sum1 + sum3));

      for (; k < j; k++) {
        sum -= (mCholesky[i_n + k] * mCholesky[j_n + k]);
      }

      mCholesky[i_n + j] = sum / mCholesky[j_n + j];
    }

    // Diagonal: L[i,i]
    float sum0 = 0.0f;
    float sum1 = 0.0f;
    float sum2 = 0.0f;
    float sum3 = 0.0f;

    size_t k_end = i & ~3ULL;
    size_t k = 0;
    for (; k < k_end; k += 4) {
      float a0 = mCholesky[i_n + k];
      float a1 = mCholesky[i_n + k + 1];
      float a2 = mCholesky[i_n + k + 2];
      float a3 = mCholesky[i_n + k + 3];

      sum0 += a0 * a0;
      sum1 += a1 * a1;
      sum2 += a2 * a2;
      sum3 += a3 * a3;
    }

    float sum = mCholesky[i_n + i] - ((sum0 + sum2) + (sum1 + sum3));

    for (; k < i; k++) {
      sum -= (mCholesky[i_n + k] * mCholesky[i_n + k]);
    }

    if (sum > min_diagonal) {
      mCholesky[i_n + i] = sqrt(sum);
    }
    else {
      goto fail;
    }
    i_n += nPadded;
  }

  return true; // Success

fail:
  return false; // Factorization failed: matrix not positive definite
}

void OLS_float_Scalar::solve() {

  // The following code is auto-vectorized to SSE2 on x64 by GCC (optimal)

#if defined(__GNUC__)
  // Tell compiler these pointers are 32-byte aligned
  float* __restrict__ C_aligned = static_cast<float*>(__builtin_assume_aligned(&mCholesky[0], 32));
  float* __restrict__ w_aligned = static_cast<float*>(__builtin_assume_aligned(&w[0], 32));
  float* __restrict__ b_aligned = static_cast<float*>(__builtin_assume_aligned(&b[0], 32));
  const float* __restrict__ x_aligned = static_cast<float*>(__builtin_assume_aligned(&x[0], 32));
#else
  float* C_aligned = &mCholesky[0];
  float* w_aligned = &w[0];
  float* b_aligned = &b[0];
  const float* x_aligned = &x[0];
#endif

  ASSUME((nPadded & 3) == 0); // hint: help the compiler to vectorize better

  // Forward solve: L·w = b
  size_t i_n = 0; // i * nPadded
  for (size_t i = 0; i < n; i++) {
    float sum = 0;
    for (size_t j = 0; j < i; j++) {
      sum -= (C_aligned[i_n + j] * w_aligned[j]);
    }
    sum += b_aligned[i];
    w_aligned[i] = sum / C_aligned[i_n + i];
    i_n += nPadded;
  }

  // Backward solve: Lᵀ·w = w
  i_n -= nPadded;
  for (size_t i = n - 1; ((int)i) >= 0; i--) {
    float sum = 0;
    size_t j_n = i_n + nPadded;
    for (size_t j = i + 1; j < n; j++) {
      sum -= (C_aligned[j_n + i] * w_aligned[j]);
      j_n += nPadded;
    }
    sum += w_aligned[i];
    w_aligned[i] = sum / C_aligned[i_n + i];

    i_n -= nPadded;
  }
}

// ===== src/OLS_float_SSE3.cpp =====

#ifdef X64_SIMD_AVAILABLE

#if (defined(__GNUC__) || defined(__clang__))
#define SSE3_TARGET __attribute__((target("sse3")))
#else
#define SSE3_TARGET
#endif

#include <cmath>

OLS_float_SSE3::OLS_float_SSE3(size_t n, size_t solveInterval, float lambda, float nu)
  : OLS_float_Scalar(n, solveInterval, lambda, nu) {
}

SSE3_TARGET
float OLS_float_SSE3::predict() {
  assert(featureIndex == n);
  featureIndex = 0;

  // Prediction: y = xᵀ·w
  __m128 v_sum = _mm_setzero_ps();
  for (size_t i = 0; i < nPadded; i += 4) {
    __m128 v_x = _mm_load_ps(&x[i]);
    __m128 v_w = _mm_load_ps(&w[i]);
    v_sum = _mm_add_ps(v_sum, _mm_mul_ps(v_x, v_w));
  }

  // Horizontal sum for floats
  v_sum = _mm_add_ps(v_sum, _mm_movehl_ps(v_sum, v_sum));
  v_sum = _mm_add_ss(v_sum, _mm_shuffle_ps(v_sum, v_sum, 0x55));
  float sum = _mm_cvtss_f32(v_sum);

  return sum;
}

SSE3_TARGET
bool OLS_float_SSE3::factor() {

  // Copy lower triangle only + apply regularization
  size_t i_n = 0;
  for (size_t i = 0; i < n; i++) {

    // Copy elements below diagonal
    size_t j_end = i & ~3ULL;
    size_t j = 0;
    for (; j < j_end; j += 4) {
      __m128 src = _mm_load_ps(&mCovariance[i_n + j]);
      _mm_store_ps(&mCholesky[i_n + j], src);
    }

    // Handle remaining elements before diagonal
    for (; j < i; j++) {
      mCholesky[i_n + j] = mCovariance[i_n + j];
    }

    // Diagonal with regularization
    mCholesky[i_n + i] = mCovariance[i_n + i] + nu;
    i_n += nPadded;
  }

  // Cholesky factorization: C = L·Lᵀ
  // Decomposes regularized covariance matrix into lower triangular L
  const size_t nb = n >> 2;  // Number of 4x4 blocks
  const size_t b = nb << 2;  // First boundary row

  float* krow0 = &mCholesky[0];           // row 4k
  float* krow1 = krow0 + nPadded;         // row 4k+1
  float* krow2 = krow1 + nPadded;         // row 4k+2
  float* krow3 = krow2 + nPadded;         // row 4k+3

  for (size_t k = 0; k < nb; k++) {
    size_t kc = 4 * k;

    // --------------------------------------------------
    // 1) Full diagonal update (4x4 block)
    // --------------------------------------------------

    // Load diagonal values to be updated
    // Initialize accumulators as vectors
    __m128 krow0_acc = _mm_load_ss(&krow0[kc]);  // [krow0_kc0, 0, 0, 0]
    __m128 krow1_acc = _mm_setr_ps(krow1[kc], krow1[kc + 1], 0.0f, 0.0f);  // [krow1_kc0, krow1_kc1, 0, 0]
    __m128 krow2_acc = _mm_loadu_ps(&krow2[kc]);  // Load first 3 + junk
    __m128 krow3_acc = _mm_load_ps(&krow3[kc]);

    for (size_t p = 0; p < k; p++) {
      size_t pc = 4 * p;

      __m128 krow0_vals = _mm_load_ps(&krow0[pc]);
      __m128 krow1_vals = _mm_load_ps(&krow1[pc]);
      __m128 krow2_vals = _mm_load_ps(&krow2[pc]);
      __m128 krow3_vals = _mm_load_ps(&krow3[pc]);

      // Compute all updates
      __m128 prod0 = _mm_mul_ps(krow0_vals, krow0_vals);
      __m128 prod1a = _mm_mul_ps(krow1_vals, krow0_vals);
      __m128 prod1b = _mm_mul_ps(krow1_vals, krow1_vals);
      __m128 prod2a = _mm_mul_ps(krow2_vals, krow0_vals);
      __m128 prod2b = _mm_mul_ps(krow2_vals, krow1_vals);
      __m128 prod2c = _mm_mul_ps(krow2_vals, krow2_vals);
      __m128 prod3a = _mm_mul_ps(krow3_vals, krow0_vals);
      __m128 prod3b = _mm_mul_ps(krow3_vals, krow1_vals);
      __m128 prod3c = _mm_mul_ps(krow3_vals, krow2_vals);
      __m128 prod3d = _mm_mul_ps(krow3_vals, krow3_vals);

      // Horizontal adds
      __m128 sum0 = _mm_hadd_ps(prod0, prod0);
      sum0 = _mm_hadd_ps(sum0, sum0);

      __m128 sum1 = _mm_hadd_ps(prod1a, prod1b);
      sum1 = _mm_hadd_ps(sum1, sum1);

      __m128 sum2a = _mm_hadd_ps(prod2a, prod2b);
      __m128 sum2b = _mm_hadd_ps(prod2c, prod2c);
      __m128 sum2 = _mm_hadd_ps(sum2a, sum2b);

      __m128 sum3a = _mm_hadd_ps(prod3a, prod3b);
      __m128 sum3b = _mm_hadd_ps(prod3c, prod3d);
      __m128 sum3 = _mm_hadd_ps(sum3a, sum3b);

      // Accumulate
      krow0_acc = _mm_sub_ss(krow0_acc, sum0);
      krow1_acc = _mm_sub_ps(krow1_acc, sum1);
      krow2_acc = _mm_sub_ps(krow2_acc, sum2);
      krow3_acc = _mm_sub_ps(krow3_acc, sum3);
    }

    // Store results back
    _mm_store_ss(&krow0[kc], krow0_acc);
    _mm_storel_pi((__m64*) & krow1[kc], krow1_acc);

    float temp2[4];
    _mm_store_ps(temp2, krow2_acc);
    krow2[kc] = temp2[0];
    krow2[kc + 1] = temp2[1];
    krow2[kc + 2] = temp2[2];

    _mm_store_ps(&krow3[kc], krow3_acc);

    // --------------------------------------------------
    // 2) Factor diagonal 4x4 block
    // --------------------------------------------------

    // Column 0
    float d00 = krow0[kc];
    if (d00 <= min_diagonal)
      goto fail;
    d00 = sqrt(d00);
    krow0[kc] = d00;

    float d10 = krow1[kc] / d00;
    krow1[kc] = d10;

    float d20 = krow2[kc] / d00;
    krow2[kc] = d20;

    float d30 = krow3[kc] / d00;
    krow3[kc] = d30;

    // Column 1
    float d11 = krow1[kc + 1] - d10 * d10;
    if (d11 <= min_diagonal)
      goto fail;
    d11 = sqrt(d11);
    krow1[kc + 1] = d11;

    float d21 = (krow2[kc + 1] - d20 * d10) / d11;
    krow2[kc + 1] = d21;

    float d31 = (krow3[kc + 1] - d30 * d10) / d11;
    krow3[kc + 1] = d31;

    // Column 2
    float d22 = krow2[kc + 2] - d20 * d20 - d21 * d21;
    if (d22 <= min_diagonal)
      goto fail;
    d22 = sqrt(d22);
    krow2[kc + 2] = d22;

    float d32 = (krow3[kc + 2] - d30 * d20 - d31 * d21) / d22;
    krow3[kc + 2] = d32;

    // Column 3
    float d33 = krow3[kc + 3] - d30 * d30 - d31 * d31 - d32 * d32;
    if (d33 <= min_diagonal)
      goto fail;
    d33 = sqrt(d33);
    krow3[kc + 3] = d33;

    __m128 d00_vec = _mm_set1_ps(d00);
    __m128 d10_vec = _mm_set1_ps(d10);
    __m128 d11_vec = _mm_set1_ps(d11);
    __m128 d20_vec = _mm_set1_ps(d20);
    __m128 d21_vec = _mm_set1_ps(d21);
    __m128 d22_vec = _mm_set1_ps(d22);
    __m128 d30_vec = _mm_set1_ps(d30);
    __m128 d31_vec = _mm_set1_ps(d31);
    __m128 d32_vec = _mm_set1_ps(d32);
    __m128 d33_vec = _mm_set1_ps(d33);

    // --------------------------------------------------
    // 3) Panel update + TRSM (4x4 blocks)
    // --------------------------------------------------
    float* irow0 = krow0 + 4 * nPadded;
    float* irow1 = irow0 + nPadded;
    float* irow2 = irow1 + nPadded;
    float* irow3 = irow2 + nPadded;

    for (size_t i = k + 1; i < nb; i++) {

      // Load the 4x4 block we're updating
      __m128 irow0_kc = _mm_load_ps(&irow0[kc]);
      __m128 irow1_kc = _mm_load_ps(&irow1[kc]);
      __m128 irow2_kc = _mm_load_ps(&irow2[kc]);
      __m128 irow3_kc = _mm_load_ps(&irow3[kc]);

      for (size_t p = 0; p < k; p++) {
        size_t pc = 4 * p;

        // Load 4x4 blocks from each irow
        __m128 aik_row0 = _mm_load_ps(&irow0[pc]);
        __m128 aik_row1 = _mm_load_ps(&irow1[pc]);
        __m128 aik_row2 = _mm_load_ps(&irow2[pc]);
        __m128 aik_row3 = _mm_load_ps(&irow3[pc]);

        // Load 4x4 block from krows
        __m128 b_row0 = _mm_load_ps(&krow0[pc]);
        __m128 b_row1 = _mm_load_ps(&krow1[pc]);
        __m128 b_row2 = _mm_load_ps(&krow2[pc]);
        __m128 b_row3 = _mm_load_ps(&krow3[pc]);

        // Update irow0_kc: 4 dot products
        __m128 prod0a = _mm_mul_ps(aik_row0, b_row0);
        __m128 prod0b = _mm_mul_ps(aik_row0, b_row1);
        __m128 prod0c = _mm_mul_ps(aik_row0, b_row2);
        __m128 prod0d = _mm_mul_ps(aik_row0, b_row3);
        __m128 sum0a = _mm_hadd_ps(prod0a, prod0b);
        __m128 sum0b = _mm_hadd_ps(prod0c, prod0d);
        __m128 sum0 = _mm_hadd_ps(sum0a, sum0b);
        irow0_kc = _mm_sub_ps(irow0_kc, sum0);

        // Update irow1_kc
        __m128 prod1a = _mm_mul_ps(aik_row1, b_row0);
        __m128 prod1b = _mm_mul_ps(aik_row1, b_row1);
        __m128 prod1c = _mm_mul_ps(aik_row1, b_row2);
        __m128 prod1d = _mm_mul_ps(aik_row1, b_row3);
        __m128 sum1a = _mm_hadd_ps(prod1a, prod1b);
        __m128 sum1b = _mm_hadd_ps(prod1c, prod1d);
        __m128 sum1 = _mm_hadd_ps(sum1a, sum1b);
        irow1_kc = _mm_sub_ps(irow1_kc, sum1);

        // Update irow2_kc
        __m128 prod2a = _mm_mul_ps(aik_row2, b_row0);
        __m128 prod2b = _mm_mul_ps(aik_row2, b_row1);
        __m128 prod2c = _mm_mul_ps(aik_row2, b_row2);
        __m128 prod2d = _mm_mul_ps(aik_row2, b_row3);
        __m128 sum2a = _mm_hadd_ps(prod2a, prod2b);
        __m128 sum2b = _mm_hadd_ps(prod2c, prod2d);
        __m128 sum2 = _mm_hadd_ps(sum2a, sum2b);
        irow2_kc = _mm_sub_ps(irow2_kc, sum2);

        // Update irow3_kc
        __m128 prod3a = _mm_mul_ps(aik_row3, b_row0);
        __m128 prod3b = _mm_mul_ps(aik_row3, b_row1);
        __m128 prod3c = _mm_mul_ps(aik_row3, b_row2);
        __m128 prod3d = _mm_mul_ps(aik_row3, b_row3);
        __m128 sum3a = _mm_hadd_ps(prod3a, prod3b);
        __m128 sum3b = _mm_hadd_ps(prod3c, prod3d);
        __m128 sum3 = _mm_hadd_ps(sum3a, sum3b);
        irow3_kc = _mm_sub_ps(irow3_kc, sum3);
      }

      // TRSM (triangular solve for 4x4 block)
      // Extract columns
      __m128 col0 = _mm_shuffle_ps(irow0_kc, irow1_kc, _MM_SHUFFLE(0, 0, 0, 0));
      col0 = _mm_shuffle_ps(col0, _mm_shuffle_ps(irow2_kc, irow3_kc, _MM_SHUFFLE(0, 0, 0, 0)), _MM_SHUFFLE(2, 0, 2, 0));

      __m128 col1 = _mm_shuffle_ps(irow0_kc, irow1_kc, _MM_SHUFFLE(1, 1, 1, 1));
      col1 = _mm_shuffle_ps(col1, _mm_shuffle_ps(irow2_kc, irow3_kc, _MM_SHUFFLE(1, 1, 1, 1)), _MM_SHUFFLE(2, 0, 2, 0));

      __m128 col2 = _mm_shuffle_ps(irow0_kc, irow1_kc, _MM_SHUFFLE(2, 2, 2, 2));
      col2 = _mm_shuffle_ps(col2, _mm_shuffle_ps(irow2_kc, irow3_kc, _MM_SHUFFLE(2, 2, 2, 2)), _MM_SHUFFLE(2, 0, 2, 0));

      __m128 col3 = _mm_shuffle_ps(irow0_kc, irow1_kc, _MM_SHUFFLE(3, 3, 3, 3));
      col3 = _mm_shuffle_ps(col3, _mm_shuffle_ps(irow2_kc, irow3_kc, _MM_SHUFFLE(3, 3, 3, 3)), _MM_SHUFFLE(2, 0, 2, 0));

      // Solve column 0
      col0 = _mm_div_ps(col0, d00_vec);

      // Solve column 1
      col1 = _mm_sub_ps(col1, _mm_mul_ps(d10_vec, col0));
      col1 = _mm_div_ps(col1, d11_vec);

      // Solve column 2
      col2 = _mm_sub_ps(col2, _mm_mul_ps(d20_vec, col0));
      col2 = _mm_sub_ps(col2, _mm_mul_ps(d21_vec, col1));
      col2 = _mm_div_ps(col2, d22_vec);

      // Solve column 3
      col3 = _mm_sub_ps(col3, _mm_mul_ps(d30_vec, col0));
      col3 = _mm_sub_ps(col3, _mm_mul_ps(d31_vec, col1));
      col3 = _mm_sub_ps(col3, _mm_mul_ps(d32_vec, col2));
      col3 = _mm_div_ps(col3, d33_vec);

      // Reconstruct rows from columns
      __m128 tmp0 = _mm_unpacklo_ps(col0, col1);
      __m128 tmp1 = _mm_unpacklo_ps(col2, col3);
      __m128 tmp2 = _mm_unpackhi_ps(col0, col1);
      __m128 tmp3 = _mm_unpackhi_ps(col2, col3);

      irow0_kc = _mm_movelh_ps(tmp0, tmp1);
      irow1_kc = _mm_movehl_ps(tmp1, tmp0);
      irow2_kc = _mm_movelh_ps(tmp2, tmp3);
      irow3_kc = _mm_movehl_ps(tmp3, tmp2);

      // Store results
      _mm_store_ps(&irow0[kc], irow0_kc);
      _mm_store_ps(&irow1[kc], irow1_kc);
      _mm_store_ps(&irow2[kc], irow2_kc);
      _mm_store_ps(&irow3[kc], irow3_kc);

      irow0 += 4 * nPadded;
      irow1 += 4 * nPadded;
      irow2 += 4 * nPadded;
      irow3 += 4 * nPadded;
    }

    krow0 += 4 * nPadded;
    krow1 += 4 * nPadded;
    krow2 += 4 * nPadded;
    krow3 += 4 * nPadded;
  }

  // --------------------------------------------------
  // Boundary handling for remaining 1-3 rows
  // --------------------------------------------------

  // Process each boundary row
  for (size_t i = b; i < n; i++) {
    float* irow = &mCholesky[i * nPadded];

    // Panel update + TRSM for each 4x4 block
    float* krow0 = &mCholesky[0];
    float* krow1 = krow0 + nPadded;
    float* krow2 = krow1 + nPadded;
    float* krow3 = krow2 + nPadded;

    for (size_t k = 0; k < nb; k++) {
      size_t kc = k * 4;

      // Panel update: subtract contributions from previous blocks
      __m128 sum_vec = _mm_load_ps(&irow[kc]);

      for (size_t p = 0; p < k; p++) {
        size_t pc = p * 4;

        __m128 a = _mm_load_ps(&irow[pc]);
        __m128 b_row0 = _mm_load_ps(&krow0[pc]);
        __m128 b_row1 = _mm_load_ps(&krow1[pc]);
        __m128 b_row2 = _mm_load_ps(&krow2[pc]);
        __m128 b_row3 = _mm_load_ps(&krow3[pc]);

        __m128 prod0 = _mm_mul_ps(a, b_row0);
        __m128 prod1 = _mm_mul_ps(a, b_row1);
        __m128 prod2 = _mm_mul_ps(a, b_row2);
        __m128 prod3 = _mm_mul_ps(a, b_row3);

        __m128 sum_a = _mm_hadd_ps(prod0, prod1);
        __m128 sum_b = _mm_hadd_ps(prod2, prod3);
        __m128 update = _mm_hadd_ps(sum_a, sum_b);

        sum_vec = _mm_sub_ps(sum_vec, update);
      }

      // TRSM: solve against block k's factored diagonal
      float d00 = krow0[kc];
      float d10 = krow1[kc];
      float d11 = krow1[kc + 1];
      float d20 = krow2[kc];
      float d21 = krow2[kc + 1];
      float d22 = krow2[kc + 2];
      float d30 = krow3[kc];
      float d31 = krow3[kc + 1];
      float d32 = krow3[kc + 2];
      float d33 = krow3[kc + 3];

      float temp[4];
      _mm_store_ps(temp, sum_vec);

      float col0 = temp[0] / d00;
      float col1 = (temp[1] - d10 * col0) / d11;
      float col2 = (temp[2] - d20 * col0 - d21 * col1) / d22;
      float col3 = (temp[3] - d30 * col0 - d31 * col1 - d32 * col2) / d33;

      irow[kc] = col0;
      irow[kc + 1] = col1;
      irow[kc + 2] = col2;
      irow[kc + 3] = col3;

      krow0 += 4 * nPadded;
      krow1 += 4 * nPadded;
      krow2 += 4 * nPadded;
      krow3 += 4 * nPadded;
    }
  }

  // Now scalar Cholesky for boundary rows among themselves
  i_n = b * nPadded;
  for (size_t i = b; i < n; i++) {

    // Off-diagonal: L[i,j] for j in [b, i)
    for (size_t j = b; j < i; j++) {
      size_t j_n = j * nPadded;

      __m128 sum_acc = _mm_setzero_ps();

      size_t k_end = j & ~3ULL;
      size_t k = 0;
      for (; k < k_end; k += 4) {
        __m128 a = _mm_load_ps(&mCholesky[i_n + k]);
        __m128 b = _mm_load_ps(&mCholesky[j_n + k]);
        sum_acc = _mm_add_ps(sum_acc, _mm_mul_ps(a, b));
      }

      // Horizontal sum
      sum_acc = _mm_add_ps(sum_acc, _mm_movehl_ps(sum_acc, sum_acc));
      sum_acc = _mm_add_ss(sum_acc, _mm_shuffle_ps(sum_acc, sum_acc, 0x55));
      float sum = mCholesky[i_n + j] - _mm_cvtss_f32(sum_acc);

      for (; k < j; k++) {
        sum -= (mCholesky[i_n + k] * mCholesky[j_n + k]);
      }

      mCholesky[i_n + j] = sum / mCholesky[j_n + j];
    }

    // Diagonal: L[i,i]
    __m128 sum_acc = _mm_setzero_ps();

    size_t k_end = i & ~3ULL;
    size_t k = 0;
    for (; k < k_end; k += 4) {
      __m128 a = _mm_load_ps(&mCholesky[i_n + k]);
      sum_acc = _mm_add_ps(sum_acc, _mm_mul_ps(a, a));
    }

    // Horizontal sum
    sum_acc = _mm_add_ps(sum_acc, _mm_movehl_ps(sum_acc, sum_acc));
    sum_acc = _mm_add_ss(sum_acc, _mm_shuffle_ps(sum_acc, sum_acc, 0x55));
    float sum = mCholesky[i_n + i] - _mm_cvtss_f32(sum_acc);

    for (; k < i; k++) {
      sum -= (mCholesky[i_n + k] * mCholesky[i_n + k]);
    }

    if (sum > min_diagonal) {
      mCholesky[i_n + i] = sqrt(sum);
    }
    else {
      goto fail;
    }
    i_n += nPadded;
  }

  return true; // Success

fail:
  return false; // Factorization failed: matrix not positive definite
}

#endif

// ===== src/OLS.cpp =====

#include <cmath>


template <typename T>
OLS<T>::OLS(size_t n, size_t solveInterval, T lambda, T nu) :
  n(n),
  nPadded(computePadding(n)),
  solveInterval(solveInterval),
  lambda(lambda),
  nu(nu),
  x(nPadded),
  w(nPadded),
  b(nPadded),
  mCovariance(n * nPadded),
  mCholesky(n * nPadded),
  samplesSinceLastSolve(0),
  featureIndex(0) {
}

// Helper to compute padded dimension for SIMD alignment
template <typename T>
constexpr size_t OLS<T>::computePadding(size_t n) {
  if constexpr (std::is_same_v<T, float>) {
    // Pad to multiple of 4 for SSE (128-bit = 4 floats)
    return (n + 3) & ~3ULL;
  }
  else {
    static_assert(std::is_same_v<T, float>,
      "computePadding only supports float");
  }
}

template <typename T>
void OLS<T>::add(T val) {
  assert(featureIndex < n);
  x[featureIndex++] = val;
}

// Reference implementation - not in use
template <typename T>
T OLS<T>::predict() {

  assert(featureIndex == n);
  featureIndex = 0;

  // Prediction: y = xᵀ·w
  T sum = static_cast<T>(0.0);
  for (size_t i = 0; i < n; i++) {
    sum += w[i] * x[i];
  }
  return sum;
}

// Reference implementation - not in use
template <typename T>
void OLS<T>::update(T y) {

  const T alpha = lambda;                       // Retention factor
  const T beta = static_cast<T>(1.0) - lambda;  // Forgetting factor

  // C = λC + (1-λ)x·xᵀ
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < n; j++) {
      mCovariance[i * nPadded + j] = alpha * mCovariance[i * nPadded + j] + beta * x[i] * x[j];
    }
  }

  // b = λb + (1-λ)y·x
  for (size_t i = 0; i < n; i++) {
    b[i] = alpha * b[i] + beta * y * x[i];
  }

  // Periodic re-solve
  // C·w = b
  samplesSinceLastSolve++;
  if (samplesSinceLastSolve >= solveInterval) {
    bool success = factor();
    if (success) {
      solve();
    }
    samplesSinceLastSolve = 0;
  }
}

// Reference implementation - not in use
template <typename T>
bool OLS<T>::factor() {

  // Copy the matrix
  memcpy(&mCholesky[0], &mCovariance[0], n * nPadded * sizeof(T));

  // Apply regularization on main diagonal
  for (size_t i = 0; i < n; i++) {
    mCholesky[i * nPadded + i] += nu;
  }

  // Cholesky factorization: C = L·Lᵀ
  // Decomposes regularized covariance matrix into lower triangular L
  for (size_t i = 0; i < n; i++) {
    for (size_t j = 0; j < i; j++) {
      T sum = mCholesky[i * nPadded + j];
      for (size_t k = 0; k < j; k++) {
        sum -= (mCholesky[i * nPadded + k] * mCholesky[j * nPadded + k]);
      }
      mCholesky[i * nPadded + j] = sum / mCholesky[j * nPadded + j];
    }
    T sum = mCholesky[i * nPadded + i];
    for (size_t k = 0; k < i; k++) {
      sum -= (mCholesky[i * nPadded + k] * mCholesky[i * nPadded + k]);
    }
    if (sum > min_diagonal) {
      mCholesky[i * nPadded + i] = sqrt(sum); // Main diagonal
    }
    else {
      return false; // Factorization failed: matrix not positive definite
    }
  }

  return true; // Success
}

// Reference implementation - not in use
template <typename T>
void OLS<T>::solve() {

  // Forward solve: L·w = b
  for (size_t i = 0; i < n; i++) {
    T sum = b[i];
    for (size_t j = 0; j < i; j++) {
      sum -= (mCholesky[i * nPadded + j] * w[j]);
    }
    w[i] = sum / mCholesky[i * nPadded + i];
  }

  // Backward solve: Lᵀ·w = w
  for (size_t i = n - 1; (int)i >= 0; i--) {
    T sum = w[i];
    for (size_t j = i + 1; j < n; j++) {
      sum -= (mCholesky[j * nPadded + i] * w[j]);
    }
    w[i] = sum / mCholesky[i * nPadded + i];
  }
}


template class OLS<float>;

// ===== src/Predictor.cpp =====

Predictor::Predictor(Shared* const sh) : shared(sh) { shared->reset(); }


// ===== src/PredictorBlock.cpp =====

PredictorBlock::PredictorBlock(Shared* const sh) : Predictor(sh), mixerFactory(sh), contextModelBlock(sh, &mixerFactory) {}

uint32_t PredictorBlock::p() {
  uint32_t pr = contextModelBlock.p(); // pr is 12 bits
  pr <<= (ArithmeticEncoder::PRECISION - 12); // scale up for the arithmetic encoder
  return pr;
}


// ===== src/PredictorMain.cpp =====

PredictorMain::PredictorMain(Shared* const sh) : Predictor(sh), sse(sh) {
  mixerFactory = new MixerFactory(shared);
  models = new Models(sh, mixerFactory);
  contextModel = new ContextModel(sh, models, mixerFactory);
}

PredictorMain::~PredictorMain() {
  delete models;
  delete contextModel;
  delete mixerFactory;
}

uint32_t PredictorMain::p() {
  uint32_t pr = contextModel->p();
  pr = sse.p(pr);
  return pr;
}


// ===== src/ProgramChecker.cpp =====

ProgramChecker *ProgramChecker::instance = nullptr;

ProgramChecker* ProgramChecker::getInstance() {
  if( instance == nullptr ) {
    instance = new ProgramChecker();
  }
  return instance;
}

void ProgramChecker::alloc(uint64_t n) {
  memUsed += n;
  if( memUsed > maxMem ) {
    maxMem = memUsed;
  }
}

void ProgramChecker::free(uint64_t n) {
  assert(memUsed >= n);
  memUsed -= n;
}

double ProgramChecker::getRuntime() const {
  const std::chrono::time_point<std::chrono::high_resolution_clock> finishTime = std::chrono::high_resolution_clock::now();
  return std::chrono::duration<double>(finishTime - startTime).count();
}

void ProgramChecker::print() const {
  const double runtime = getRuntime();
  printf("Time %1.2f sec, used %" PRIu64 " MB (%" PRIu64 " bytes) of memory\n", runtime, maxMem >> 20, maxMem);
}

ProgramChecker::~ProgramChecker() {
  assert(memUsed == 0); // We expect that all reserved memory is already properly freed
}


// ===== src/Random.cpp =====

Random::Random() {
  _state = 0;
}

// This pseudo random number generator is a 
// Mixed Congruential Generator with a period of 2^64
// https://en.wikipedia.org/wiki/Linear_congruential_generator

uint32_t Random::operator()(int numberOfBits) {
  assert(numberOfBits > 0 && numberOfBits <= 32);
  _state = (_state + 1) * PHI64;
  return static_cast<uint32_t>(_state >> (64 - numberOfBits));
}


// ===== src/ResidualMap.cpp =====

ResidualMap::ResidualMap(const Shared* const sh, const int numContexts, const int histogramsPerContext, const int scale) :
  shared(sh),
  scale(scale),
  numContexts(numContexts),
  histogramsPerContext(histogramsPerContext),
  currentContextIndex(0),
  predictions(numContexts),
  bases(numContexts),
  sums(BINS* numContexts* histogramsPerContext) {
  dt = DivisionTable::getDT();
}

void ResidualMap::setscale(const int scale) {
  this->scale = scale;
}

void ResidualMap::set(const short prediction, const uint32_t histogram_id) {
  assert(histogram_id < histogramsPerContext);
  assert(currentContextIndex < numContexts);
  predictions[currentContextIndex] = prediction;
  bases[currentContextIndex] = (currentContextIndex * histogramsPerContext + histogram_id) * BINS;
  currentContextIndex++;
}

void ResidualMap::skip() {
  assert(currentContextIndex < numContexts);
  bases[currentContextIndex] = UINT32_MAX;
  currentContextIndex++;
}

void ResidualMap::mix(Mixer& m) {
  INJECT_SHARED_bpos
  INJECT_SHARED_c0
  if (bpos == 7)
    shared->GetUpdateBroadcaster()->subscribe(this);
  assert(currentContextIndex == numContexts);

  for (size_t i = 0; i < currentContextIndex; i++) {
    const uint32_t base = bases[i];
    if (base == UINT32_MAX) { // skipped context
      m.add(0);
      m.add(0);
      continue;
    }

    const uint16_t* s = &sums[base];
    const uint8_t  c1 = c0 << (8 - bpos);
    const int range = 1 << (7 - bpos);  // 128 down to 1

    // n0: bins [offset, offset+range) mod 256
    // n1: bins [mid,    mid+range)    mod 256
    // s[k] = count[0]+...+count[k], k=0..BINS-1, s[BINS-1]=total.
    // Circular range [a, a+len): no wrap → s[a+len-1]-s[a-1]; wrap → s[BINS-1]-s[a-1]+s[a+len-BINS-1].
    // boundary cases:
    //   a==0 → s[a-1] treated as 0.
    //   offset+range == BINS is the exact-fit case: s[BINS-1] is the last valid entry, no wrap needed.
    const int bin0_offset = (192 + c1 - predictions[i]) & 255;
    const int bin1_offset = (bin0_offset + range) & 255;

    // n0: observation count in the "next bit = 0" residual window (lower half)
    // n1: observation count in the "next bit = 1" residual window (upper half)
    uint64_t n0, n1;
    // n0
    if (bin0_offset + range <= BINS)
      n0 = s[bin0_offset + range - 1] - (bin0_offset == 0 ? 0 : s[bin0_offset - 1]);
    else
      n0 = s[BINS - 1] - s[bin0_offset - 1] + s[bin0_offset + range - BINS - 1];

    // n1
    if (bin1_offset + range <= BINS)
      n1 = s[bin1_offset + range - 1] - (bin1_offset == 0 ? 0 : s[bin1_offset - 1]);
    else
      n1 = s[BINS - 1] - s[bin1_offset - 1] + s[bin1_offset + range - BINS - 1];

    // we need p1 = 4096*(n1+1)/(n0+n1+2)
    // to avoid the division:

    const uint64_t raw_sum = n0 + n1; // [0..131070] where 131070 = two times UINT16_MAX

    // to use dt, we need sum to be [0..1023] → we need to shift raw_sum by how much?
    // 131070 >> 7 = 1023
    //  65535 >> 6 = 1023
    //  32767 >> 5 = 1023
    //       ...
    //   4095 >> 2 = 1023
    //   2047 >> 1 = 1023
    //   1023 >> 0 = 1023
    // the shift needed is the number of bits in raw_sum - 10
    // the number of bits in raw_sum is 32 - clz(raw_sum)

    // clz is undefined for 0, but raw_sum==0 → shift=0 anyway,
    // so we OR with 1 to make clz safe; it doesn't affect the result when raw_sum>0.
    const int shift_raw = 32 - 10 - clz((uint32_t)(raw_sum | 1));
    const int shift = shift_raw & ~(shift_raw >> 31);  // max(shift_raw, 0) without branch

    const size_t sum_small = raw_sum >> shift;  // [0..1023]
    n1 = (n1 >> shift) + 1;

    const int p1 = (int)(((n1 << 12) * (uint64_t)dt[sum_small]) >> 30); //4096 * (n1+1) / (n0+n1+2)

    m.add((stretch(p1) * scale) >> 8);
    m.add(((p1 - 2048) * scale) >> 9);
  }
}

void ResidualMap::update() {
  assert(shared->State.bitPosition == 0);
  assert(currentContextIndex == numContexts);

  while (currentContextIndex > 0) {
    currentContextIndex--;
    const uint32_t base = bases[currentContextIndex];
    if (base == UINT32_MAX)
      continue;

    INJECT_SHARED_c1
    const int bin = (192 + c1 - predictions[currentContextIndex]) & 255; // moving the center to bin #192 gives better cache locality on read and on updates

    // Increment all prefix entries after bin: sums[bin .. BINS-1] += 1.
    uint16_t* s = &sums[base];
    for (int k = bin; k < BINS; k++)
      s[k]++;

    // Aging: when the total reaches UINT16_MAX halve all prefix entries.
    // A bin's count is s[k]-s[k-1] (s[-1] treated as 0); total is s[BINS-1].
    if (s[BINS - 1] == UINT16_MAX) {
      for (int k = 0; k < BINS; k++)
        s[k] >>= 1;
    }
  }
}

void ResidualMap::print() {
  for (size_t i = 0; i < numContexts; i++) {
    for (size_t j = 0; j < histogramsPerContext; j++) {
      const uint16_t* s = &sums[(i * histogramsPerContext + j) * BINS];
      printf("%zu\t%zu", i, j);
      for (int k = 0; k < BINS; k++)
        printf("\t%d", k == 0 ? s[0] : s[k] - s[k - 1]);
      printf("\n");
    }
  }
}

// ===== src/Shared.cpp =====

/*
 relationship between compression level, shared->mem and buf memory use

 level   shared->mem    buf memory use (shared->mem * 8)
 -----   -----------    --------------
   1      0.125 MB              1 MB
   2      0.25	MB              2 MB
   3      0.5   MB              4 MB
   4      1.0   MB              8 MB
   5      2.0   MB             16 MB
   6      4.0   MB             32 MB
   7      8.0   MB             64 MB
   8     16.0   MB            128 MB
   9     32.0   MB            256 MB
  10     64.0   MB            512 MB
  11    128.0   MB           1024 MB
  12    256.0   MB           1024 MB

*/

Shared::Shared() {
  toScreen = !isOutputRedirected();
}

void Shared::init(uint8_t level, uint32_t bufMem) {
  this->level = level;
  mem = UINT64_C(65536) << level;
  if (bufMem == 0) //to auto size
    bufMem = static_cast<uint32_t>(min(mem * 8, UINT64_C(1) << 28)); /**< no reason to go over 256 MB */
  assert(isPowerOf2(bufMem));
  buf.setSize(bufMem);
  toScreen = !isOutputRedirected();
}

void Shared::update(int y, uint32_t p, bool isMissed) {
  State.y = y;
  State.c0 += State.c0 + y;
  State.bitPosition = (State.bitPosition + 1) & 7;
  if(State.bitPosition == 0 ) {
    State.c1 = State.c0;
    buf.add(State.c1);
    State.c8 = (State.c8 << 8) | (State.c4 >> 24);
    State.c4 = (State.c4 << 8) | State.c0;
    State.c0 = 1;
  }
  
  State.misses = (State.misses << 1) | static_cast<uint32_t>(isMissed);

  constexpr uint32_t shift = ArithmeticEncoder::PRECISION - 6;
  constexpr uint32_t maxp = (1u << ArithmeticEncoder::PRECISION) - 1;

  State.loss = ((y == 0 ? p : maxp - p)) >> shift; //0..1023
  assert(State.loss >= 0 && State.loss <= 1023);

  // Broadcast to all current subscribers: y (and c0, c1, c4, etc) is known
  updateBroadcaster.broadcastUpdate();
}

void Shared::reset() {
  buf.reset();
  memset(&State, 0, sizeof(State));
  State.c0 = 1;
}

UpdateBroadcaster *Shared::GetUpdateBroadcaster() const {
  UpdateBroadcaster* updater = const_cast<UpdateBroadcaster*>(&updateBroadcaster);
  return updater;
}

bool Shared::isOutputRedirected() {
#ifdef WINDOWS
  DWORD FileType = GetFileType(GetStdHandle(STD_OUTPUT_HANDLE));
  return (FileType == FILE_TYPE_PIPE) || (FileType == FILE_TYPE_DISK);
#endif
#ifdef UNIX
  return isatty(fileno(stdout)) == 0;
#endif
}

// ===== src/SmallStationaryContextMap.cpp =====

SmallStationaryContextMap::SmallStationaryContextMap(const Shared* const sh, const int bitsOfContext, const int inputBits, const int rate, const int scale) : 
  shared(sh),
  data((UINT64_C(1) << bitsOfContext) * ((UINT64_C(1) << inputBits) - 1)),
  mask((1U << bitsOfContext) - 1), 
  stride((1U << inputBits) - 1),
  bTotal(inputBits), rate(rate), scale(scale) {
  assert(inputBits > 0 && inputBits <= 8);
  reset();
  set(0);
}

void SmallStationaryContextMap::set(uint32_t ctx) {
  context = (ctx & mask) * stride;
  bCount = b = 0;
}

void SmallStationaryContextMap::reset() {
  for( uint32_t i = 0; i < data.size(); ++i ) {
    data[i] = 0x7FFF;
  }
  cp = &data[0];
}

void SmallStationaryContextMap::update() {
  INJECT_SHARED_y
  *cp += ((y << 16) - (*cp) + (1 << (rate - 1))) >> rate;
  b += static_cast<uint32_t>((y != 0) && b > 0);
}

void SmallStationaryContextMap::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  cp = &data[context + b];
  const int prediction = (*cp) >> 4;
  m.add((stretch(prediction) * scale) >> 8);
  m.add(((prediction - 2048) * scale) >> 9);
  bCount++;
  b += b + 1;
  assert(bCount <= bTotal);
}


// ===== src/Squash.cpp =====
#include <cassert>
#include <cmath>
#include <cstdint>


class squash_table {
public:
  short t[4095];
  int squash(int d /*-2047..2047*/) {
    /**
     * return p = 1/(1 + exp(-d)), d scaled by 8 bits, p scaled by 12 bits
     * @param d
     * @return
     */
    if (d < -2047)return 1;
    if (d > 2047)return 4095;
    float p = 1.0f / (1.0f + exp(-d / 256.0f));
    p *= 4096.0;
    uint32_t pi = (uint32_t)round(p);
    if (pi > 4095)pi = 4095;
    if (pi < 1)pi = 1;
    return pi;
  }

  squash_table() {
    for (int i = -2047; i <= 2047; i++) {
      t[i + 2047] = squash(i);
    }
  }

} sqt;

int squash(int d /*-2047..2047*/) {
  if (d < -2047)return 1;
  if (d > 2047)return 4095;
  return sqt.t[d + 2047];
}



// ===== src/SSE.cpp =====

// ---------------------------------------------------------------------------
// SSE (Secondary Symbol Estimation)
// ---------------------------------------------------------------------------
//
// WHAT IT DOES
//
// The mixer outputs a probability that the next bit is 1. That probability is
// good, but not perfectly calibrated: in some situations the mixer is
// systematically too confident, in others not confident enough.
//
// The SSE stage acts as an adaptive probability correction layer. It corrects
// the systematic bias with lookup tables called APMs (Adaptive Probability Maps). 
// 
// Each table answers the question: "when the incoming probability was p, and
// we were in context X, how often was the bit actually 1?" After each bit,
// the table moves its entries a little toward what actually happened.
//
// No single context captures every kind of miscalibration, so we use several
// tables, each keyed by a different context (the partial byte c0, previous
// bytes, match state, neighbouring pixels, ...). That gives us several
// corrected estimates of the same bit, which then have to be combined into
// one.
//
// Throughout this comment, "cost" means the number of bits the arithmetic
// coder spends on one bit: -log2(the probability we gave to the value that
// actually occurred). 50% costs 1 bit, 90% costs 0.15 bits, 1% costs 6.64
// bits.
//
// THE BUILDING BLOCKS
//
// APM and APM1 (12-bit probability in, 12-bit probability out)
//
//   For each context, the table holds a row of points spread across the
//   probability range. The points are evenly spaced on the "stretch" scale,
//   stretch(p) = ln(p/(1-p)), which puts them closer together near 0% and
//   100%, where a small change in probability makes a big difference in
//   cost. The input probability falls between two points, and the output is
//   interpolated between their stored probabilities.
//
//   Every table starts out as (roughly) the identity map: output = input.
//
//   The two types differ in how fast they learn, which matters below:
//
//   APM:  each point counts its updates, and the update step is
//         1/(count+2). A point learns quickly at first, then settles down.
//         The count stops at a limit (1023 here), so in the long run a point
//         reflects roughly its last thousand updates. Long memory: stable,
//         precise statistics, but slow to react when the data changes.
//
//   APM1: the update step is fixed at 1/2^rate (1/32 to 1/128 here), so a
//         point reflects roughly its last 2^rate updates. Short memory:
//         reacts quickly to changes, but is noisier.
//
// APMPost (12-bit probability in, 31-bit probability out)
//
//   Everything before this stage works with 12-bit probabilities, in steps
//   of 1/4096, but the arithmetic coder works with 31 bits. The difference
//   matters for highly predictable data: a 12-bit probability can never go
//   above 4095/4096, so even a bit that is certain costs at least 0.00035
//   bits. Over a megabyte of zeros (8 million bits), that adds up to about
//   350 bytes.
//
//   APMPost removes that limit. It has one cell for each of the 4096
//   possible input values (per context), and each cell simply counts the 0s
//   and 1s it has seen. Its output is the ratio of those counts, computed
//   with 31-bit precision. So the cell for input 4095/4096 can discover that
//   the bit is really 1 in 99.999% of cases, and say so.
//
//   Each cell starts with about 2048 imaginary observations that agree with
//   its input value, so APMPost begins as the identity map and moves away
//   from it only after thousands of real observations. Its context is kept
//   small (bpos, model order, match mode, or none): the more contexts, the
//   fewer observations per cell, so the context mainly sets how fast it
//   learns.
//
//   Because it counts real outcomes, APMPost also recalibrates, but slowly,
//   and only in cells that see a lot of traffic.
//
// TWO WAYS TO AVERAGE PROBABILITIES
//
//   Linear: average the probabilities themselves.
//   Logit:  average the stretch(p) values, then convert the result back to a
//           probability with squash(), the inverse of stretch().
//
// The two behave very differently when the estimates disagree. Example: three
// tables say the bit is 1 with 70% probability; a fourth is very sure it is a
// 0 and says 0.1%.
//
//                     pooled p(1)   cost if bit=1   cost if bit=0
//   Linear average       52.5%        0.93 bits       1.07 bits
//   Logit average        25.1%        1.99 bits       0.42 bits
//
// On the stretch scale, extreme probabilities lie far out: stretch(0.001) is
// about -6.9, while stretch(0.7) is only about +0.85. So one very confident
// estimate pulls a logit average strongly toward itself. A linear average
// gives each estimate exactly its share and no more.
//
// A logit average therefore wins when confident outliers are usually right,
// and a linear average wins when they are often wrong.
//
// WHY LINEAR HERE
//
// The mixer combines its inputs on the stretch scale, and that is the right
// choice there: its weights are learned bit by bit, so it discovers which
// inputs deserve trust, and its weights may sum to more than 1, which lets
// several agreeing inputs reinforce each other.
//
// Here the situation is different:
//
//  - The weights are fixed by hand. Nothing lowers the weight of a table that
//    turns out to be unreliable.
//
//  - When one table is far more confident than the others, that is often a
//    symptom of a problem rather than a real insight:
//      * a context hashed down to 16 bits, sharing its row with unrelated
//        contexts,
//      * an APM1 row that has just adapted to a short burst of unusual data,
//      * an APM row still holding long-term statistics from data that has
//        since changed.
//    (A row that has seen little data is not a problem: it starts as the
//    identity map, so it simply passes its input through.)
//
//  - The estimates are strongly correlated. They all start from the same
//    mixer output, most of their contexts include c0 or bpos, and some tables
//    are chained: they refine another table's output instead of the mixer's.
//    Their agreement is weaker evidence than it looks.
//
// Linear averaging with weights that sum to 1 also comes with a guarantee:
// the result is never much worse than the best estimate for that bit. If an
// estimate has weight w, the averaged probability of the actual bit is at
// least w times that estimate's probability. Therefore:
//
//     cost(average) <= cost(that estimate) + log2(1/w) bits
//
// With 4 equal weights, the average costs at most 2 bits more than the best
// estimate, no matter how badly the other three fail. (In the example above,
// the best estimate costs 0.51 bits and the linear average 0.93 bits.)
// A logit average has no such bound: the more extreme the wrong estimates,
// the more it costs.
//
// THE PRICE, AND HOW IT IS PAID
//
// A linear average can never be more confident than its most confident
// input, and when the inputs disagree it comes out less confident than it
// should be. This is proven: if each input is well calibrated and they are
// not all identical, any average with positive weights summing to 1 is
// miscalibrated toward underconfidence, however strongly the inputs are
// correlated. The miscalibration grows with how much the inputs disagree.
// Recalibrating the average (replacing p by the actual frequency of 1s
// observed when the average said p) gives a strictly better forecast under
// every proper scoring rule, coding cost included.
// (Ranjan & Gneiting, "Combining probability forecasts", JRSS-B 72(1):71-91,
// 2010, doi:10.1111/j.1467-9868.2009.00726.x, Theorem 1.)
//
// The remedy the paper recommends has the same shape as this stage: a
// linear average followed by a learned recalibration map. Here, in several
// paths an APM1 takes an average (prA) as its input and learns, per context,
// how that average relates to the actual outcomes, including sharpening it
// when it is too timid. APMPost adds a slow, long-term correction on top.
//
// The final 50/50 average is, by the same theorem, slightly underconfident,
// and nothing corrects it afterwards. Its inputs rarely disagree much, so
// the loss is small, and by the bound above it can never exceed 1 bit
// more than the better of the two.
//
// LAYOUT
//
// Most paths build two estimates and average them at the end:
//
//   prA = average of the mixer output and APMs refining it
//   prB = average of pr0 and APM1s, most of them chained on refined estimates
//   pr  = ( APMPostA(prA) + APMPostB(prB) ) / 2
//
// In the paths that use APM1s (generic and 24/32-bit and
// palette images), the two branches learn at different speeds. prA comes
// only from APMs (long memory), while half to three quarters of prB's weight
// comes from APM1s (short memory). When the data changes, the fast branch
// catches up first; in stable data, the slow branch has the more precise
// statistics. Which branch is better changes over time, and the final linear
// average never loses more than 1 bit to whichever is currently better.

SSE::SSE(Shared* const sh) : shared(sh),
Image{
  // color:
    { { /*APM:*/ {sh,1 << 7,24,1023}, {sh,1 << 16,24,1023}, {sh,1 << 16,24,1023}, {sh,1 << 16,24,1023}},
      { /*APM1:*/ {sh,1 << 15,7}, {sh,1 << 16,7}} ,
      { /*ApmPostA: */ sh,8},
      { /*ApmPostB: */ sh,8}
    },
  // palette:
  { { /*APM:*/ {sh,1 << 11,24,1023}, {sh,1 << 16,24,1023}, {sh,1 << 16,24,1023}, {sh,1 << 16,24,1023}},
    { /*APM1:*/ {sh,1 << 16,5}, {sh,1 << 16,6}},
    { /*ApmPostA: */ sh,1},
    { /*ApmPostB: */ sh,1}
  },
  // gray:
  { { /*APM:*/ {sh,1 << 11,24,1023}, {sh,1 << 16,24,1023}, {sh,256 * 257,24,1023}} ,
    { /*ApmPostA: */ sh,8},
    { /*ApmPostB: */ sh,8}
  },
  // bilevel:
  { { /*APM:*/ {sh,1 << 10,20,1023}, {sh,1 << 16,20,1023}, {sh,41 * 8,20,1023}, {sh,64,20,1023} },
    { /*APM1:*/ {sh,1 << 16,6}, {sh,1 << 16,6}, {sh,1 << 16,6}},
    { /*ApmPostFinal: */ sh,16}
  }
},
Jpeg{
  { /*APM:*/ {sh,0x1000 + 1,24,1023} },
  { /*ApmPostA: */ sh,1},
  { /*ApmPostB: */ sh,1}
},
Generic{
  { /*APM:*/  {sh,1 << 8,20,1023}, {sh,1 << 8,24,1023}, {sh,1 << 16,24,1023}, {sh,1 << 16,24,1023}}, /* APM: contexts, steps */
  { /*APM1:*/ {sh,256 * 257,7}, {sh,1 << 16,7}, {sh,1 << 16,7}},
  { /*ApmPostA: */ sh,8},
  { /*ApmPostB: */ sh,8}
} {
}

uint32_t SSE::p(const uint32_t pr_orig) {

  INJECT_SHARED_c0
    INJECT_SHARED_bpos
    INJECT_SHARED_c4
    INJECT_SHARED_blockPos
    INJECT_SHARED_blockType

    assert(shared->State.NormalModel.order <= 7);
    assert(shared->State.JPEG.state <= 4095 + 1);

  assert(shared->State.Image1.ctx12 < (1u << 12));
  assert(shared->State.Image1.shape < (1u << 22));
  assert(shared->State.Image1.ink <= 40);
  assert(shared->State.Image1.match <= 32);

  assert(shared->State.Image.plane <= 3);
  assert(shared->State.Image.lossQ <= 639);

  //uint32_t misses4 = shared->State.misses & 15u;
  uint32_t misses = shared->State.misses << ((8 - bpos) & 7); //byte-aligned
  misses = (misses & 0xffffff00) | (misses & 0xff) >> ((8 - bpos) & 7);

  uint32_t misses3 =
    ((misses & 0x1) != 0) |
    ((misses & 0xfe) != 0) << 1 |
    ((misses & 0xff00) != 0) << 2;

  const BlockType normalizedBlockType =
    blockType == BlockType::JPEG && shared->State.JPEG.state == 0 ? BlockType::DEFAULT :
    blockType;

  switch (normalizedBlockType) {
  case BlockType::IMAGE24:
  case BlockType::IMAGE32: {
    uint32_t plane = shared->State.Image.plane; // 0..3
    uint32_t pr0 = Image.Color.APMs[0].p(pr_orig, plane << 5 | bpos << 2 | (bpos == 0 ? 0 : (misses3 & 3))); //7, even the plane could be enough
    uint32_t pr1 = Image.Color.APMs[1].p(pr_orig, finalize64(hash(c0, shared->State.Image.pixels.W, shared->State.Image.pixels.WW), 16)); //16
    uint32_t pr2 = Image.Color.APMs[2].p(pr_orig, finalize64(hash(c0, shared->State.Image.pixels.N, shared->State.Image.pixels.NN), 16)); //16
    uint32_t pr3 = Image.Color.APMs[3].p(pr_orig, (c0 << 8) | shared->State.Image.ctx); //16

    uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + 2) >> 2;

    uint32_t lossQ = shared->State.Image.lossQ; // 0 .. 639
    uint32_t pr4 = Image.Color.APM1s[0].p(pr0, (lossQ >> 2) << 5 | plane << 3 | bpos); //15
    uint32_t pr5 = Image.Color.APM1s[1].p(pr0, finalize64(hash(c0, min(lossQ, 255), plane), 16)); //16

    uint32_t prB = (pr0 * 2 + pr4 * 3 + pr5 * 3 + 4) >> 3;

    uint32_t pr = (Image.Color.APMPostA.p(prA, bpos) + Image.Color.APMPostB.p(prB, bpos) + 1) >> 1;
    return pr;
    break;
  }
  case BlockType::IMAGE8GRAY: {
    uint32_t pr0 = Image.Gray.APMs[0].p(pr_orig, static_cast<uint32_t>(c0) << 3 | (bpos == 0 ? 0 : (misses3 & 3))); //11
    uint32_t pr1 = Image.Gray.APMs[1].p(pr0, (c0 << 8) | shared->State.Image.ctx); //16
    uint32_t pr2 = Image.Gray.APMs[2].p(pr_orig, (bpos | (shared->State.Image.ctx & 0xF8)) * 257 + shared->State.Match.expectedByte); //256*257

    int prA = (2 * pr_orig + pr1 + pr2 + 2) >> 2;

    uint32_t pr = (Image.Gray.APMPostA.p(pr0, bpos) + Image.Gray.APMPostB.p(prA, bpos) + 1) >> 1;
    return pr;
    break;
  }
  case BlockType::IMAGE8: {
    uint32_t pr0 = Image.Palette.APMs[0].p(pr_orig, static_cast<uint32_t>(c0) << 3 | (bpos == 0 ? 0 : (misses3 & 3))); //11
    uint32_t pr1 = Image.Palette.APMs[1].p(pr_orig, finalize64(hash(c0 | shared->State.Image.pixels.W << 8 | shared->State.Image.pixels.N << 16), 16)); //16
    uint32_t pr2 = Image.Palette.APMs[2].p(pr_orig, finalize64(hash(c0 | shared->State.Image.pixels.N << 8 | shared->State.Image.pixels.NN << 16), 16)); //16
    uint32_t pr3 = Image.Palette.APMs[3].p(pr_orig, finalize64(hash(c0 | shared->State.Image.pixels.W << 8 | shared->State.Image.pixels.WW << 16), 16)); //16

    uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + 2) >> 2;

    uint32_t pr4 = Image.Palette.APM1s[0].p(prA, finalize64(hash(c0 | shared->State.Image.pixels.N << 8, shared->State.Match.expectedByte), 16)); //16
    uint32_t pr5 = Image.Palette.APM1s[1].p(pr0, finalize64(hash(c0 | shared->State.Image.pixels.W << 8, shared->State.NormalModel.order), 16)); //16

    uint32_t prB = (pr0 * 2 + pr4 + pr5 + 2) >> 2;

    uint32_t pr = (Image.Palette.APMPostA.p(prA, 0) + Image.Palette.APMPostB.p(prB, 0) + 1) >> 1;
    return pr;
    break;
  }
  case BlockType::IMAGE1: {
    const auto& im = shared->State.Image1;
    const uint32_t misses = shared->State.misses;
    const uint32_t misses3im1 = (misses & 1) | ((misses & 0x6) != 0) << 1 | ((misses & 0xfffffff8) != 0) << 2;

    uint32_t pr0 = Image.Bilevel.APMs[0].p(pr_orig, (im.ctx12 & 0x7f) << 3 | misses3im1); //10
    uint32_t pr1 = Image.Bilevel.APMs[1].p(pr_orig, finalize64(hash(im.rowAbove, im.column), 16)); //16
    uint32_t pr2 = Image.Bilevel.APMs[2].p(pr_orig, static_cast<uint32_t>(im.ink) << 3 | misses3im1); //41*8
    uint32_t pr3 = Image.Bilevel.APMs[3].p(pr_orig, im.ref); //64

    uint32_t pr4 = Image.Bilevel.APM1s[0].p(pr0, finalize64(hash(im.run, im.ink, im.ref, misses3im1), 16)); //16
    uint32_t pr5 = Image.Bilevel.APM1s[1].p(pr0, finalize64(hash(im.ctx12, im.shape), 16)); //16
    uint32_t pr6 = Image.Bilevel.APM1s[2].p(pr0, finalize64(hash(im.match, im.ctx12), 16)); //16

    const uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + pr4 + pr5 + pr6 + 3) / 7;

    uint32_t pr = Image.Bilevel.APMPostFinal.p(prA, im.run >> 1);
    return pr;
    break;
  }
//case BlockType::IMAGE4: //TODO
  case BlockType::JPEG: {
    uint32_t pr0 = Jpeg.APMs[0].p(pr_orig, shared->State.JPEG.state);
    uint32_t pr = (Jpeg.APMPostA.p(pr_orig, 0) + Jpeg.APMPostB.p(pr0, 0) + 1) >> 1;
    return pr;
    break;
  }
  default: {
    uint32_t pr0 = Generic.APMs[0].p(pr_orig, shared->State.Match.length2 << 6 | static_cast<uint32_t>(bpos) << 3 | misses3); //8
    uint32_t pr1 = Generic.APMs[1].p(pr_orig, shared->State.NormalModel.order << 5 | shared->State.Match.length2 << 3 | bpos); //8
    uint32_t pr2 = Generic.APMs[2].p(pr_orig, c0 | (c4 & 0xFF) << 8); //16
    uint32_t pr3 = Generic.APMs[3].p(pr_orig, c0 << 8 | (c4 & 0xF0) | ((c4 & 0xF000) >> 12)); //16

    uint32_t prA = (pr_orig + pr1 + pr2 + pr3 + 2) >> 2;

    uint32_t pr4 = Generic.APM1s[0].p(prA, shared->State.Match.expectedByte + (misses3 << 5 | shared->State.NormalModel.order << 2 | shared->State.Match.length2) * 257); //256*257
    uint32_t pr5 = Generic.APM1s[1].p(pr0, misses3 << 13 | shared->State.Match.length2 << 11 | (shared->State.NormalModel.order >> 2) << 8 | c0); //16
    uint32_t pr6 = Generic.APM1s[2].p(pr0, (misses3 & 3) << 14 | (bpos >> 1) << 12 | (c4 & 0xFF) << 4 | (static_cast<uint32_t>(shared->State.NormalModel.order) >> 1)); //16

    uint32_t prB = (pr0 + pr4 + pr5 + pr6 + 2) >> 2;

    uint32_t pr = (Generic.APMPostA.p(prA, shared->State.NormalModel.order) + Generic.APMPostB.p(prB, shared->State.Match.mode3) + 1) >> 1;
    return pr;
  }
  }

}

// ===== src/StateMap.cpp =====

StateMap::StateMap (const Shared* const sh, const int s, const int n, const int lim, const StateMapType mapType) :
  AdaptiveMap(sh, n * s), limit(lim), numContextSets(s), numContextsPerSet(n), currentContextSetIndex(0), cxt(s) {
  assert(numContextSets > 0 && numContextsPerSet > 0);
  if( mapType == StateMapType::BitHistory ) { // when the context is a bit history byte, we have a-priory for p
    assert((numContextsPerSet & 255) == 0);
    for (uint64_t cx = 0; cx < numContextsPerSet; ++cx) {
      uint8_t state = cx & 255;
      for (uint64_t s = 0; s < numContextSets; ++s) {
        uint32_t n0 = StateTable::getNextState(state, 2);
        uint32_t n1 = StateTable::getNextState(state, 3);
        uint32_t p;
        if (state < 205) {
          n0 = n0 * 3 + 1;
          n1 = n1 * 3 + 1;
          p = ((n1 << 20) / (n0 + n1)) << 12;
          //printf("%d %d\n", state, p >> 20); // verifying
        }
        else if(state < 253){
          int incremental_state = (state - 205)>>2;
          if (((state - 205) & 3) <= 1)
            n0 = 29 + (1 << incremental_state);
          else
            n1 = 29 + (1 << incremental_state);
          n0 = n0 * 3 + 1;
          n1 = n1 * 3 + 1;
          assert(n0 < 16384 && n1 < 16384);
          p = ((n1 << 18) / (n0 + n1)) << 14 | min((n0 + n1) >> 3, 1023);
          //printf("%d %d\n", state, p >> 20); // verifying
        }
        else { // 253, 254, 255
          p = 2048 << 20; //unused states: p=0.5
        }
        t[cx * numContextSets + s] = p;
      }
    }
  } else if( mapType == StateMapType::Run ) { // when the context is a run count: we have a-priory for p
    for( uint32_t cx = 0; cx < numContextsPerSet; ++cx ) {
      const int predictedBit = (cx) & 1;
      const int uncertainty = (cx >> 1) & 1;
      //const int bp = (cx>>2)&1; // unused in calculation - a-priory does not seem to depend on bitPosition for a RunMap
      const int runCount = (cx >> 4); // 0..254
      uint32_t n0 = uncertainty + 1;
      uint32_t n1 = (runCount + 1) * 12;
      if( predictedBit == 0 ) {
        std::swap(n0, n1);
      }
      assert(n0 < 4096 && n1 < 4096);
      for( uint64_t s = 0; s < numContextSets; ++s ) {
        t[cx * numContextSets + s] = ((n1 << 20) / (n0 + n1)) << 12 | limit;
      }
    }
  } else { // no a-priory in the general case
    for( uint32_t i = 0; i < numContextsPerSet * numContextSets; ++i ) {
      t[i] = 2048<<20 | 0; //initial p=0.5, initial count=0
    }
  }
}

void StateMap::update() {
  assert(currentContextSetIndex <= numContextSets);
  while( currentContextSetIndex > 0 ) {
    currentContextSetIndex--;
    const uint32_t idx = cxt[currentContextSetIndex];
    if( idx == UINT32_MAX) {
      continue; // skipped context
    }
    assert(idx % numContextSets == currentContextSetIndex && idx / numContextSets < numContextsPerSet);
    AdaptiveMap::update(&t[idx], limit);
  }
}

int StateMap::p1(const uint32_t cx) {
  assert(numContextSets == 1);
  assert(currentContextSetIndex == 0);
  assert(cx >= 0 && cx < numContextsPerSet);
  shared->GetUpdateBroadcaster()->subscribe(this);
  cxt[0] = cx;
  currentContextSetIndex++;
  return t[cx] >> 20;
}

int StateMap::p2(const uint32_t s, const uint32_t cx) {
  assert(s < numContextSets);
  assert(cx < numContextsPerSet);
  assert(s == currentContextSetIndex);
  const uint32_t idx = cx * numContextSets + currentContextSetIndex;
  cxt[currentContextSetIndex] = idx;
  currentContextSetIndex++;
  return t[idx] >> 20;
}

void StateMap::subscribe() {
  shared->GetUpdateBroadcaster()->subscribe(this);
}

void StateMap::skip(const uint32_t contextSetIndex) {
  assert(contextSetIndex < numContextSets);
  assert(contextSetIndex == currentContextSetIndex);
  cxt[currentContextSetIndex] = UINT32_MAX; // mark for skipping
  currentContextSetIndex++;
}

void StateMap::print() const {
  for (uint32_t cx = 0; cx < numContextsPerSet; cx++) {
    for (uint32_t s = 0; s < numContextSets; s++) {
      uint32_t p0 = t[cx * numContextSets + s] >> 10;
      printf("%d\t%d\t%d\n", cx, s, p0);
    }
  }
}

// ===== src/StateTable.cpp =====

uint8_t  StateTable::getNextState(uint8_t const state, const int y) {
  return stateTable[state][y];
}

uint8_t StateTable::getNextState(uint8_t const oldState, const int y, Random &rnd) {
  const uint8_t newState = stateTable[oldState][y];
  if (newState >= 205 && newState >= oldState + 4) {
    // Apply probabilistic increment for contexts with strong trend of 0s or 1s.
    // Applicable to states 149 and above, but we'll start applying it from states (205-208).
    // That means...
    // For states 205..208, 209..212, ... 249..252 a group of 4 states is not represented by
    // the counts indicated in the state table. An exponential scale is used instead.
    // The highest group (249..252) represents the top of this scale, where we can not increment anymore.
    // Reaching the highest states (249-252) from states (201-204) takes 12 steps.
    // thus the probability of reaching the highest states (249-252) is ~ 1/2048 
    if (rnd(1) != 0) { // random bit: p()=1/2 -> the probability to advance to a higher state is 1/2
      return oldState; // don't advance
    }
  }
  return newState;
}

void StateTable::update(uint8_t *const state, const int y, Random &rnd) {
  *state = getNextState(*state, y, rnd);
}

uint8_t StateTable::group(const uint8_t state) {
  return stateGroup[state];
}

uint8_t StateTable::prio(const uint8_t state) {
  return statePrio[state];
}


// ===== src/StationaryMap.cpp =====

StationaryMap::StationaryMap(const Shared* const sh, const int bitsOfContext, const int inputBits, const int scale) : 
  shared(sh),
  data((UINT64_C(1) << bitsOfContext) * ((UINT64_C(1) << inputBits) - 1)),
  mask((1 << bitsOfContext) - 1), 
  stride((1 << inputBits) - 1), bTotal(inputBits), scale(scale),
  context(0), bCount(0), b(0), cp(nullptr)
{
  assert(inputBits > 0 && inputBits <= 8);
  assert(bitsOfContext + inputBits <= 24);
}

void StationaryMap::set(uint32_t ctx) {
  context = (ctx & mask) * stride;
  bCount = b = 0;
}

void StationaryMap::setScale(int scale) {
  this->scale = scale;
}

void StationaryMap::reset() {
  for( uint32_t i = 0; i < data.size(); ++i ) {
    data[i] = 0;
  }
}

void StationaryMap::update() {
  INJECT_SHARED_y

  uint32_t counts, n0, n1;
  counts = *cp;
  n0 = counts >> 16;
  n1 = counts & 0xffff;

  n0 += 1 - y;
  n1 += y;
  int shift = (n0 | n1) >> 16; // shift: 0 or 1
  n0 >>= shift;
  n1 >>= shift;

  *cp = n0 << 16 | n1;

  b += static_cast<uint32_t>((y != 0) && b > 0);
}

void StationaryMap::mix(Mixer &m) {
  shared->GetUpdateBroadcaster()->subscribe(this);
  uint32_t counts, n0, n1, sum;
  int p1, st, bitIsUncertain;

  cp = &data[context + b];
  counts = *cp;
  n0 = counts >> 16;
  n1 = counts & 0xffff;

  sum = n0 + n1;
  p1 = ((n1 * 2 + 1) << 12) / (sum * 2 + 2);
  st = (stretch(p1) * scale) >> 8;
  m.add(st);
  m.add(((p1 - 2048) * scale) >> 9);
  bitIsUncertain = int(sum <= 1 || (n0 != 0 && n1 != 0));
  m.add((bitIsUncertain - 1) & st); // when both counts are nonzero add(0) otherwise add(st)

  bCount++;
  b += b + 1;
  assert(bCount <= bTotal);
}


// ===== src/StationaryMapForJpegModel.cpp =====

StationaryMapForJpegModel::StationaryMapForJpegModel(const Shared* const sh, const uint32_t numContexts, const int bitsOfMemory,
  const int inputBits, const int scale) :
  shared(sh),
  data(UINT64_C(1) << (bitsOfMemory - 8)), // 256 bytes per group
  activeContexts(numContexts),
  numContexts(numContexts),
  groupBits(bitsOfMemory - 8),
  scale(scale),
  bCount(0), b(0), subscribed(false) {
  assert(inputBits == static_cast<int>(InputBits)); // the memory layout depends on it
  static_cast<void>(inputBits); // only used by the assert
  assert(numContexts > 0);
  assert(groupBits >= 1); // at least two groups
  assert(groupBits + ChecksumBits <= 64); // the group index and the checksum must both fit in the 64 bit hash
}

void StationaryMapForJpegModel::set(const uint32_t i, const uint64_t ctxHash) {
  assert(i < numContexts);
  bCount = b = 0;
  ActiveContext& ctx = activeContexts[i];
  const uint64_t h = hash(i, ctxHash);
  // The group index is taken from the top groupBits bits of the hash and the
  // checksum from the 24 bits below them, so the two don't overlap.
  ctx.group = &data[finalize64(h, groupBits)];
  const uint32_t c = checksum24(h, groupBits);
  ctx.chk = c + static_cast<uint32_t>((c & 0xffff) == 0); // a low half of 0 is reserved for empty elements
  prefetch(ctx.group); // the lookup only reads this first cache line
}

void StationaryMapForJpegModel::claim(Group* const g, const uint32_t e, const uint32_t chk) {
  // Start from empty counters, so the new context does not inherit statistics
  // from the context that used this element before.
  g->root[e] = 0;
  memset(g->tail[e], 0, sizeof(g->tail[e]));
  g->chkLo[e] = static_cast<uint16_t>(chk);
  g->chkHi[e] = static_cast<uint8_t>(chk >> 16);
}

uint32_t StationaryMapForJpegModel::victimRank(const Group* const g, const uint64_t ranking) {
  // In 1 out of 64 replacements, ignore the use counts and simply take the least
  // recently used element. Otherwise an old element with a high count could stay
  // at the back of the group forever.
  if (rnd(6) == 0) {
    return ElementsPerGroup - 1;
  }
  uint32_t victim = ElementsPerGroup - 1;
  uint32_t victimUses = useCount(g->root[elementAt(ranking, ElementsPerGroup - 1)]);
  // On a tie, replace the less recently used one (only a strictly smaller count wins).
  for (uint32_t r = ElementsPerGroup - 2; r >= Protected; --r) {
    const uint32_t uses = useCount(g->root[elementAt(ranking, r)]);
    if (uses < victimUses) {
      victimUses = uses;
      victim = r;
    }
  }
  return victim;
}

void StationaryMapForJpegModel::lookup(ActiveContext& ctx) {
  Group* const g = ctx.group;
  const uint16_t chkLo = static_cast<uint16_t>(ctx.chk);
  const uint8_t chkHi = static_cast<uint8_t>(ctx.chk >> 16);
  const uint64_t ranking = g->order ^ Identity;

  uint32_t r = ElementsPerGroup; // stays ElementsPerGroup if the group is full and the context is not in it
  bool isNew = true; // true unless the context was found

  for (uint32_t rank = 0; rank < ElementsPerGroup; ++rank) {
    const uint32_t e = elementAt(ranking, rank);
    const uint16_t lo = g->chkLo[e];
    if (lo == chkLo && g->chkHi[e] == chkHi) { // found the context
      r = rank;
      isNew = false;
      break;
    }
    if (lo == 0) { // empty element; all later ranks are empty too
      r = rank;
      break;
    }
  }

  if (r == ElementsPerGroup) { // group is full and the context is not in it: replace an element
    r = victimRank(g, ranking);
  }

  const uint32_t e = elementAt(ranking, r);
  if (r != 0) { // is not at the front yet?
    g->order = moveToFront(ranking, r) ^ Identity;
  }
  if (isNew) {
    claim(g, e, ctx.chk);
  }

  ctx.counter = &g->root[e]; // the first bit of the chunk uses the root counter
  ctx.tail = &g->tail[e][0];
  // The tail is first needed on the second bit of the chunk, so fetch it now.
  // A tail is 24 bytes, so two of the eight tails cross a cache line boundary:
  // prefetch both its first and its last counter.
  prefetch(ctx.tail);
  prefetch(ctx.tail + (Counters - 2));
}

void StationaryMapForJpegModel::setScale(const int scale) {
  this->scale = scale;
}

void StationaryMapForJpegModel::reset() {
  // An all-zero group is valid: empty, with its elements ranked in index order
  // (see Identity for why).
  memset(&data[0], 0, data.size() * sizeof(Group));
  bCount = b = 0;
}

int StationaryMapForJpegModel::mix(Mixer& m, const uint32_t i) {
  if (!subscribed) { // subscribe once per bit, not once per context, so update() runs once per bit
    shared->GetUpdateBroadcaster()->subscribe(this);
    subscribed = true;
  }
  assert(i < numContexts);
  assert(b < Counters);

  ActiveContext& ctx = activeContexts[i];
  if (b == 0) { // first bit of the chunk: the group prefetched by set() should be in cache by now
    lookup(ctx); // sets ctx.counter and ctx.tail
  }
  else {
    ctx.counter = ctx.tail + (b - 1); // counters[b]; the tail starts at counters[1]
  }

  const uint32_t counts = *ctx.counter;
  const uint32_t n0 = counts >> 16;
  const uint32_t n1 = counts & 0xffff;
  const uint32_t sum = n0 + n1;

  const int p1 = static_cast<int>(((n1 * 2 + 1) << 12) / (sum * 2 + 2));
  const int st = (stretch(p1) * scale) >> 8;
  m.add(st);
  m.add(((p1 - 2048) * scale) >> 9);
  const int bitIsUncertain = static_cast<int>(sum <= 1 || (n0 != 0 && n1 != 0));
  // Adds st only if the bit has always had the same value (seen at least twice); otherwise adds 0.
  m.add((bitIsUncertain - 1) & st);

  return st;
}

void StationaryMapForJpegModel::update() {
  INJECT_SHARED_y

  // y == 0: add 1 to n0 (high half); y == 1: add 1 to n1 (low half).
  const uint32_t inc = 0x00010000u >> (y << 4);

  for (uint32_t i = 0; i < numContexts; ++i) {
    uint32_t* const counter = activeContexts[i].counter; // the counter that mix() just used
    uint32_t c = *counter + inc; // both counts are at most 0x7fff before this, so the increment cannot overflow into the other half
    if (c & 0x80008000u) { // rare: at most once per 16k updates of this counter
      // When either count reaches 0x8000, halve both counts. This keeps each
      // count within its 16 bits and lets the statistics keep adapting.
      // Packed version of:
      //   n0 = c >> 16;
      //   n1 = c & 0xffff;
      //   n0 >>= 1;
      //   n1 >>= 1;
      //   c = n0 << 16 | n1;
      // Shifting right moves the lowest bit of n0 into bit 15 (the top of n1);
      // the mask clears it. Bit 31 is always 0 after a right shift.
      c = (c >> 1) & 0x7fff7fffu;
    }
    *counter = c;
  }

  b = 2 * b + 1 + y;
  bCount++;
  assert(bCount <= InputBits);
  subscribed = false;
}

// ===== src/Stretch.cpp =====
#include <cassert>
#include <cmath>
#include <cstdint>

class stretch_table {
public:
  short t[4096];
  int stretch(int p) {
    /**
   * Inverse of @ref squash. d = ln(p/(1-p)), d scaled by 8 bits, p by 12 bits.
   * d has range -2047 to 2047 representing -8 to 8. p has range 0 to 4095.
   */
    assert(p >= 0 && p <= 4095);
    if (p == 0)p = 1;
    float f = p / 4096.0f;
    float d = log(f / (1.0f - f)) * 256.0f;
    int32_t di = (int32_t)round(d);
    if (di > 2047)di = 2047;
    if (di < -2047)di = -2047;
    return di;
  }

  stretch_table() {
    for (int i = 0; i <= 4095; i++) {
      t[i] = stretch(i);
    }
  }

} str;


int stretch(int p) {
  return str.t[p];
}



// ===== src/String.cpp =====
#include <string.h>

#ifndef NDEBUG
  void String::chk_consistency() const {
    for (uint32_t i = 0; i < size() - 1; i++) {
      if ((*this)[i] == 0) {
        quit("Internal error - string consistency check failed (1).");
      }
    }
    if (((*this)[size() - 1]) != 0) {
      quit("Internal error - string consistency check failed (2).");
    }
  }
#endif

void String::appendIntRecursive(uint64_t x) {
  if( x <= 9 ) {
    pushBack('0' + char(x));
  } else {
    const uint64_t rem = x % 10;
    x = x / 10;
    if( x != 0 ) {
      appendIntRecursive(x);
    }
    pushBack('0' + char(rem));
  }
}

const char* String::c_str() const {
  return &(*this)[0];
}

uint64_t String::strsize() const {
  chk_consistency();
  assert(size() > 0);
  return size() - 1;
}

void String::operator=(const char *s) {
  resize(strlen(s) + 1);
  memcpy(&(*this)[0], s, size());
  chk_consistency();
}

void String::operator+=(const char *s) {
  const uint64_t pos = size();
  const uint64_t len = strlen(s);
  resize(pos + len);
  memcpy(&(*this)[pos - 1], s, len + 1);
  chk_consistency();
}

void String::operator+=(char c) {
  popBack(); //Remove NUL
  pushBack(c);
  pushBack(0); //Append NUL
  chk_consistency();
}

void String::operator+=(uint64_t x) {
  popBack(); //Remove NUL
  if( x == 0 ) {
    pushBack('0');
  } else {
    appendIntRecursive(x);
  }
  pushBack(0); //Append NUL
  chk_consistency();
}

bool String::endsWith(const char *ending) const {
  const uint64_t endingSize = strlen(ending);
  if( endingSize > strsize()) {
    return false;
  }
  const int cmp = memcmp(ending, &(*this)[strsize() - endingSize], endingSize);
  return (cmp == 0);
}

void String::stripEnd(uint64_t count) {
  assert(strsize() >= count);
  const uint64_t newSize = strsize() - count;
  resize(newSize);
  pushBack(0); //Append NUL
  chk_consistency();
}

bool String::beginsWith(const char *beginning) const {
  const uint64_t beginningSize = strlen(beginning);
  if( beginningSize > strsize()) {
    return false;
  }
  const int cmp = memcmp(beginning, &(*this)[0], beginningSize);
  return (cmp == 0);
}

void String::stripStart(uint64_t count) {
  assert(strsize() >= count);
  const uint64_t newSize = strsize() - count;
  memmove(&(*this)[0], &(*this)[count], newSize);
  resize(newSize);
  pushBack(0); //Append NUL
  chk_consistency();
}

int String::findLast(char c) const{
  uint64_t i = strsize();
  while( i-- > 0 ) {
    if((*this)[i] == c ) {
      return static_cast<int>(i);
    }
  }
  return -1; //not found
}

String::String(const char *s) : Array<char>(strlen(s) + 1) {
  memcpy(&(*this)[0], s, size());
  chk_consistency();
}


// ===== src/UpdateBroadcaster.cpp =====

void UpdateBroadcaster::subscribe(IPredictor *subscriber) {
  subscribers[n] = subscriber;
  n++;
  assert(n < 1024);
}

void UpdateBroadcaster::broadcastUpdate() {
  for( int i = 0; i < n; i++ ) {
    IPredictor *subscriber = subscribers[i];
    subscriber->update();
  }
  n = 0;
}

