#ifndef LOOPRECORDERDECK_H
#define LOOPRECORDERDECK_H

#include "basetrackplayer.h"

class LoopRecorderDeck : public BaseTrackPlayer {
    Q_OBJECT
  public:
    LoopRecorderDeck(QObject* pParent,
                ConfigObject<ConfigValue> *pConfig,
                EngineMaster* pMixingEngine,
                EngineChannel::ChannelOrientation defaultOrientation,
                QString group);
    virtual ~LoopRecorderDeck();
};

#endif /* LoopRecorderDeck_H */
