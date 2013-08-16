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

    // Should this be process instead?
    void writeSamples(const CSAMPLE* pBuffer, const int iBufferSize);

    // Moves the recorder object to the new thread and starts execution.
    void startThread();

    // Returns a pointer to the LoopWriter object.
    // Used to connect signals and slots with the LoopRecordingManager.
    LoopWriter* getLoopWriter() {
        return m_pLoopWriter;
    }

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
