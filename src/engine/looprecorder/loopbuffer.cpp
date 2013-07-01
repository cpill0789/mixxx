//  loopbuffer.cpp
//  Created by Carl Pillot on 6/27/13.

#include "loopbuffer.h"

#include "looprecording/defs_looprecording.h"
#include "sampleutil.h"

LoopBuffer::LoopBuffer(ConfigObject<ConfigValue>* _config)
: m_config(_config){
    m_bIsRecording = false;
    m_iSamplesRecorded = 0;
    //m_recReady = new ControlObjectThread(LOOP_RECORDING_PREF_KEY, "rec_status");
    //m_samplerate = new ControlObjectThread("[Master]", "samplerate");
    m_pMainBuffer = SampleUtil::alloc(LOOP_BUFFER_LENGTH);
}

LoopBuffer::~LoopBuffer() {
    SampleUtil::free(m_pMainBuffer);
}

void LoopBuffer::writeSampleBuffer(const CSAMPLE* pBuffer, const int iBufferSize) {
    qDebug() << "LoopBuffer::writeSampleBuffer " << *pBuffer << " " << iBufferSize;
}

CSAMPLE* LoopBuffer::getBuffer() {
    return m_pMainBuffer;
}

void LoopBuffer::resetBuffer() {

}

bool LoopBuffer::isReady() {
    qDebug() << "LoopBuffer::isReady";
    return true;
}

bool LoopBuffer::isRecording() {
    //qDebug() << "LoopBuffer::isRecording";
    return m_bIsRecording;
}

void LoopBuffer::open() {
    qDebug() << "LoopBuffer::open";
    m_bIsRecording = true;
}

void LoopBuffer::endRecording() {
    qDebug() << "LoopBuffer::endRecording";
    m_bIsRecording = false;
}