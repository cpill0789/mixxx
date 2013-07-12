#include "LoopRecorderDeck.h"

LoopRecorderDeck::LoopRecorderDeck(QObject* pParent,
                         ConfigObject<ConfigValue> *pConfig,
                         EngineMaster* pMixingEngine,
                         EngineChannel::ChannelOrientation defaultOrientation,
                         QString group) :
        BaseTrackPlayer(pParent, pConfig, pMixingEngine, defaultOrientation,
                group, false, true) {
}

LoopRecorderDeck::~LoopRecorderDeck() {
}
