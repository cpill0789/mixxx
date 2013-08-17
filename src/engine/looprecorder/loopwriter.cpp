//  loopwriter.cpp
//  Created by Carl Pillot on 8/15/13.
//  Writes audio to a file for loop recording.

#include "engine/looprecorder/loopwriter.h"

#include "configobject.h"
#include "defs.h"
#include "sampleutil.h"

#define LOOP_BUFFER_SIZE 16384

LoopWriter::LoopWriter(ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig),
        m_sampleFifo(LOOP_BUFFER_SIZE),
        m_pWorkBuffer(SampleUtil::alloc(LOOP_BUFFER_SIZE)),
        m_isRecording(false),
        m_path("") {

}

LoopWriter::~LoopWriter() {
    qDebug() << "!~!~!~!~!~! Loop writer deleted !~!~!~!~!~!~!";
    SampleUtil::free(m_pWorkBuffer);
    emit(finished());
}

void LoopWriter::process(const CSAMPLE* pBuffer, const int iBufferSize) {
    
}

void LoopWriter::slotSetPath(QString) {

}

void LoopWriter::slotStartWriter() {
    qDebug() << "!~!~!~!~!~! Loop writer started !~!~!~!~!~!~!";
    emit(writerStarted());
}

void LoopWriter::slotStartRecording(int samples) {
    qDebug() << "!~!~!~!~!~! LoopWriter::slotStartRecording Length: " << samples << " !~!~!~!~!~!~!";
}

void LoopWriter::slotStopRecording() {

}
