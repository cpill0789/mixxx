//  loopbuffer.h
//  Created by Carl Pillot on 6/27/13.

// class for recording loop samples to a buffer

#ifndef LOOPBUFFER_H
#define LOOPBUFFER_H

#include "configobject.h"
#include "defs.h"

class ConfigKey;
class ControlObjectThread;

class LoopBuffer : public QObject {
    Q_OBJECT
public:
    LoopBuffer(ConfigObject<ConfigValue>* _config);
    ~LoopBuffer();
    
    void writeSampleBuffer(const CSAMPLE* pBuffer, const int iBufferSize);
    CSAMPLE* getBuffer();
    void resetBuffer();
    
private:
    ConfigObject<ConfigValue>* m_config;
    CSAMPLE* m_pMainBuffer;
    int samplesRecorder;
    
};

#endif