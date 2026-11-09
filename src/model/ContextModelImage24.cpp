#include "../MixerFactory.hpp"
#include "../Models.hpp"

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
