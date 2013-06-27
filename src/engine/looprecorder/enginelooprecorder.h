//  enginelooprecorder.h
//  Created by Carl Pillot on 6/22/13.
//  Adapted from EngineLoopRecorder.h

#ifndef ENGINELOOPRECORDER_H
#define ENGINELOOPRECORDER_H

#include <sndfile.h>

#include "configobject.h"
#include "trackinfoobject.h"

class ConfigKey;
class ControlObjectThread;

class EngineLoopRecorder : public QObject {
    Q_OBJECT
public:
    EngineLoopRecorder(ConfigObject<ConfigValue>* _config);
    virtual ~EngineLoopRecorder();
    
    void process(const CSAMPLE* pBuffer, const int iBufferSize);
    void shutdown() {}
    
    // writes uncompressed audio to file
    void write(unsigned char *header, unsigned char *body, int headerLen, int bodyLen);
    // creates or opens an audio file
    bool openFile();
    // closes the audio file
    void closeFile();
    void updateFromPreferences();
    bool fileOpen();
    bool isRecording();
    
signals:
    // emitted to notify LoopRecordingManager
    //void bytesRecorded(int);
    //void isLoopRecording(bool);
    
private:
    //int getActiveTracks();
    
    // Check if the metadata has changed since the previous check. We also check
    // when was the last check performed to avoid using too much CPU and as well
    // to avoid changing the metadata during scratches.
    //bool metaDataHasChanged();
    
    //void writeCueLine();
    
    ConfigObject<ConfigValue>* m_config;
    QByteArray m_Encoding;
    QString m_filename;
    QByteArray m_baTitle;
    QByteArray m_baAuthor;
    QByteArray m_baAlbum;
    
    //QFile m_file;
    //QFile m_cuefile;
    //QDataStream m_datastream;
    SNDFILE *m_sndfile;
    SF_INFO m_sfInfo;
    
    ControlObjectThread* m_recReady;
    ControlObjectThread* m_samplerate;
    
    //int m_iMetaDataLife;
    TrackPointer m_pCurrentTrack;
    
    //QByteArray m_cuefilename;
    //quint64 m_cuesamplepos;
    //quint64 m_cuetrack;
    //bool m_bCueIsEnabled;
    bool m_bIsRecording;
};

#endif