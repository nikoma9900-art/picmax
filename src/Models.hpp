#pragma once

#include "model/Image1BitModel.hpp"
#include "model/Image24BitModel.hpp"
#include "model/Image4BitModel.hpp"
#include "model/Image8BitModel.hpp"
#include "model/JpegModel.hpp"
#include "model/MatchModel.hpp"
#include "model/NormalModel.hpp"
#include "model/IContextModel.hpp"
#include "MixerFactory.hpp"

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
