#include <cstdint>
#include "BlockType.hpp"

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
