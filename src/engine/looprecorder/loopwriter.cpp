//  loopwriter.cpp
//  Created by Carl Pillot on 6/22/13.
//  Writes audio to a file for loop recording.

#include "engine/looprecorder/loopwriter.h"

LoopWriter::LoopWriter(ConfigObject<ConfigValue>* pConfig)
        : m_pConfig(pConfig) {

}

LoopWriter::~LoopWriter() {
    
}