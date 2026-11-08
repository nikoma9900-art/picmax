#include "ContextModel.hpp"
#include "ContextModelGeneric.cpp"
#include "ContextModelImage1.cpp"
#include "ContextModelImage4.cpp"
#include "ContextModelImage8.cpp"
#include "ContextModelImage24.cpp"
#include "ContextModelJpeg.cpp"

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
