//  loopbuffer.cpp
//  Created by Carl Pillot on 6/27/13.

#include "loopbuffer.h"

LoopBuffer::LoopBuffer(ConfigObject<ConfigValue>* _config)
: m_config(_config),
m_pMainBuffer(new CSAMPLE[LOOP_BUFFER_LENGTH]) {
    //m_bIsRecording = false;
    //m_recReady = new ControlObjectThread(LOOP_RECORDING_PREF_KEY, "rec_status");
    //m_samplerate = new ControlObjectThread("[Master]", "samplerate");
}

LoopBuffer::~LoopBuffer() {
    delete m_pMainBuffer;
}

LoopBuffer::writeSampleBuffer(const CSAMPLE* pBuffer, const int iBufferSize) {
    return;
}

LoopBuffer::getBuffer() {
    return m_pMainBuffer;
}

LoopBuffer::resetBuffer() {
    return;
}