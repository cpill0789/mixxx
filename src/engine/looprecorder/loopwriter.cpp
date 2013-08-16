//  loopwriter.cpp
//  Created by Carl Pillot on 6/22/13.
//  Writes audio to a file for loop recording.

#include "engine/looprecorder/loopwriter.h"

LoopWriter::LoopWriter(ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig),
        m_isRecording(false),
        m_path("") {

}

LoopWriter::~LoopWriter() {
    
}

LoopWriter::process(const CSAMPLE* pBuffer, const int iBufferSize) {
    
}

LoopWriter::slotSetPath(QString) {

}
LoopWriter::slotStartRecording(int) {

}
LoopWriter::slotStopRecording() {

}
