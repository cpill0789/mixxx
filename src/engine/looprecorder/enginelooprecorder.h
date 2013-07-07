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
    
private:
    void run();
    void process(const CSAMPLE* pBuffer, const int iBufferSize);
    //int getActiveTracks();
    
    // Check if the metadata has changed since the previous check. We also check
    // when was the last check performed to avoid using too much CPU and as well
    // to avoid changing the metadata during scratches.
    //bool metaDataHasChanged();
    
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
    
    ControlObjectThread* m_recReady;
    ControlObjectThread* m_samplerate;
    
    //int m_iMetaDataLife;
    TrackPointer m_pCurrentTrack;
    
    bool m_bIsRecording;
};

#endif