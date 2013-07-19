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
class ControlObject;
class ControlObjectThread;

class EngineLoopRecorder : public QThread {
    Q_OBJECT
  public:
    EngineLoopRecorder(ConfigObject<ConfigValue>* _config);
    virtual ~EngineLoopRecorder();
    
    void writeSamples(const CSAMPLE* pBuffer, const int iBufferSize);
        
    // creates or opens an audio file
    bool openFile();
    // closes the audio file
    void closeFile();
    void updateFromPreferences();
    bool fileOpen();
    
  signals:
    // emitted to notify LoopRecordingManager
    //void bytesRecorded(int);
    void isLoopRecording(bool);
    void clearRecorder();
    void loadToLoopDeck();
    
  private:
    void run();
    
    void process(const CSAMPLE* pBuffer, const int iBufferSize);
    //int getActiveTracks();
    
    ConfigObject<ConfigValue>* m_config;
    
    // Indicates that the thread should exit.
    volatile bool m_bStopThread;
    
    FIFO<CSAMPLE> m_sampleFifo;
    CSAMPLE* m_pWorkBuffer;
    
    // Provides thread safety around the wait condition below.
    QMutex m_waitLock;
    // Allows sleeping until we have samples to process.
    QWaitCondition m_waitForSamples;
    
    QByteArray m_Encoding;
    QString m_filename;
    QByteArray m_baTitle;
    QByteArray m_baAuthor;
    QByteArray m_baAlbum;
        
    SNDFILE *m_sndfile;
    SF_INFO m_sfInfo;
    
    ControlObject* m_recReadyCO;
    ControlObjectThread* m_recReady;
    ControlObjectThread* m_samplerate;
    
    TrackPointer m_pCurrentTrack;
    
    bool m_bIsRecording;
};

#endif
