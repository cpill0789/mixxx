//  enginelooprecorder.h
//  Created by Carl Pillot on 6/22/13.
//  Adapted from EngineLoopRecorder.h

#ifndef ENGINELOOPRECORDER_H
#define ENGINELOOPRECORDER_H

#include <QtCore>
#include <sndfile.h>

#include "configobject.h"
#include "trackinfoobject.h"
#include "util/fifo.h"

class ConfigKey;
//class ControlObject;
//class ControlObjectThread;
class LoopWriter;

class EngineLoopRecorder : public QObject {
    Q_OBJECT
  public:
    EngineLoopRecorder(ConfigObject<ConfigValue>* _config);
    virtual ~EngineLoopRecorder();
    
    void writeSamples(const CSAMPLE* pBuffer, const int iBufferSize);
        
    // creates or opens an audio file
//    bool openFile();
    // closes the audio file
//    void closeFile();
//    void updateFromPreferences();
//    bool isFileOpen();

  signals:
    // emitted to notify LoopRecordingManager of number of samples recorded
    void samplesRecorded(int);
    void isLoopRecording(bool);
    void clearRecorder();
    void loadToLoopDeck();
    
  private:
//    void run();

//    void process(const CSAMPLE* pBuffer, const int iBufferSize);
    //int getActiveTracks();
    
    ConfigObject<ConfigValue>* m_config;

    QThread* LoopRecorderThread;
    LoopWriter* m_pLoopWriter;
};

#endif
