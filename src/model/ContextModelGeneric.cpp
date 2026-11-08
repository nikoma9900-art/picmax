#include "../MixerFactory.hpp"
#include "../Models.hpp"

class ContextModelGeneric: public IContextModel {

private:
  Shared* const shared;
  Models* const models;
  Mixer* m;

public:
  ContextModelGeneric(Shared* const sh, Models* const models, const MixerFactory* const mf) : shared(sh), models(models) {
    const bool useLSTM = shared->GetOptionUseLSTM();
    m = mf->createMixer(
      1 +  //bias
      MatchModel::MIXERINPUTS + NormalModel::MIXERINPUTS +
      (useLSTM ? LstmModelContainer::MIXERINPUTS : 0)
      ,
      MatchModel::MIXERCONTEXTS + NormalModel::MIXERCONTEXTS_PRE + NormalModel::MIXERCONTEXTS_POST +
      (useLSTM ? LstmModelContainer::MIXERCONTEXTS : 0)
      ,
      MatchModel::MIXERCONTEXTSETS + NormalModel::MIXERCONTEXTSETS_PRE + NormalModel::MIXERCONTEXTSETS_POST +
      (useLSTM ? LstmModelContainer::MIXERCONTEXTSETS : 0)
      ,
      (useLSTM ? 1 : 0)
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

    const bool useLSTM = shared->GetOptionUseLSTM();
    if (useLSTM) {
      LstmModelContainer& lstmModel = models->lstmModelGeneric();
      lstmModel.mix(*m);
    }

    return m->p();
  }

  ~ContextModelGeneric() {
    delete m;
  }

};
