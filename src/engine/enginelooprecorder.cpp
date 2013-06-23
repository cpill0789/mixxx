//  enginelooprecorder.cpp
//  Created by Carl Pillot on 6/22/13.
//  Adapted from EngineLoopRecorder.cpp

#include "enginelooprecorder.h"

#include "configobject.h"
#include "controlobject.h"
#include "controlobjectthread.h"
#include "errordialoghandler.h"
#include "playerinfo.h"

//const int kMetaDataLifeTimeout = 16;

EngineLoopRecorder::EngineLoopRecorder(ConfigObject<ConfigValue>* _config)
: m_config(_config),
m_sndfile(NULL),
m_iMetaDataLife(0) {
    m_recReady = new ControlObjectThread(RECORDING_PREF_KEY, "status");
    m_samplerate = new ControlObjectThread("[Master]", "samplerate");
}

EngineLoopRecorder::~EngineLoopRecorder() {
    closeFile();
    delete m_recReady;
    delete m_samplerate;
}

void EngineLoopRecorder::updateFromPreferences() {
    m_Encoding = m_config->getValueString(ConfigKey(RECORDING_PREF_KEY,"Encoding")).toLatin1();
    //returns a number from 1 .. 10
    //m_OGGquality = m_config->getValueString(ConfigKey(RECORDING_PREF_KEY,"OGG_Quality")).toLatin1();
    //m_MP3quality = m_config->getValueString(ConfigKey(RECORDING_PREF_KEY,"MP3_Quality")).toLatin1();
    m_filename = m_config->getValueString(ConfigKey(RECORDING_PREF_KEY,"Path"));
}


void EngineLoopRecorder::process(const CSAMPLE* pBuffer, const int iBufferSize) {
    // if recording is disabled
    //if (m_recReady->get() == RECORD_OFF) {
    //    //qDebug("Setting record flag to: OFF");
    //    if (fileOpen()) {
    //        closeFile();    //close file and free encoder
    //        emit(isRecording(false));
    //    }
    //}
    
    // if we are ready for recording, i.e, the output file has been selected, we
    // open a new file
    if (m_recReady->get() == RECORD_READY) {
        updateFromPreferences();	//update file location from pref
        if (openFile()) {
            qDebug("Setting record flag to: ON");
            m_recReady->slotSet(RECORD_ON);
            emit(isLoopRecording(true)); //will notify the RecordingManager
            
            // Since we just started recording, timeout and clear the metadata.
            m_iMetaDataLife = kMetaDataLifeTimeout;
            m_pCurrentTrack = TrackPointer();
            
            if (m_bCueIsEnabled) {
                openCueFile();
                m_cuesamplepos = 0;
                m_cuetrack = 0;
            }
        } else { // Maybe the encoder could not be initialized
            qDebug("Setting record flag to: OFF");
            m_recReady->slotSet(RECORD_OFF);
            emit(isLoopRecording(false));
        }
    }
    
    // If recording is enabled process audio to uncompressed data.
    if (m_recReady->get() == RECORD_ON) {
        if (m_Encoding == ENCODING_WAVE || m_Encoding == ENCODING_AIFF) {
            if (m_sndfile != NULL) {
                sf_write_float(m_sndfile, pBuffer, iBufferSize);
                emit(bytesRecorded(iBufferSize));
            }
        }
  	}
}

bool EngineLoopRecorder::fileOpen() {
    // Both encoder and file must be initalized
    
    if (m_Encoding == ENCODING_WAVE || m_Encoding == ENCODING_AIFF) {
        return (m_sndfile != NULL);
    }
}

bool EngineLoopRecorder::openFile() {
    // Unfortunately, we cannot use QFile for writing WAV and AIFF audio
    if (m_Encoding == ENCODING_WAVE || m_Encoding == ENCODING_AIFF){
        unsigned long samplerate = m_samplerate->get();
        // set sfInfo
        m_sfInfo.samplerate = samplerate;
        m_sfInfo.channels = 2;
        
        if (m_Encoding == ENCODING_WAVE)
            m_sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
        else
            m_sfInfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_16;
        
        // creates a new WAVE or AIFF file and write header information
        m_sndfile = sf_open(m_filename.toLocal8Bit(), SFM_WRITE, &m_sfInfo);
        if (m_sndfile) {
            sf_command(m_sndfile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE) ;
            //set meta data
            int ret;
            
            ret = sf_set_string(m_sndfile, SF_STR_TITLE, m_baTitle.data());
            if(ret != 0)
                qDebug("libsndfile: %s", sf_error_number(ret));
            
            ret = sf_set_string(m_sndfile, SF_STR_ARTIST, m_baAuthor.data());
            if(ret != 0)
                qDebug("libsndfile: %s", sf_error_number(ret));
            
            ret = sf_set_string(m_sndfile, SF_STR_COMMENT, m_baAlbum.data());
            if(ret != 0)
                qDebug("libsndfile: %s", sf_error_number(ret));
            
        }
    }
    
    // check if file is really open
    if (!fileOpen()) {
        ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
        props->setType(DLG_WARNING);
        props->setTitle(tr("Recording"));
        props->setText(tr("<html>Could not create audio file for recording!<p><br>Maybe you do not have enough free disk space or file permissions.</html>"));
        ErrorDialogHandler::instance()->requestErrorDialog(props);
        return false;
    }
    return true;
}

void EngineLoopRecorder::closeFile() {
    if (m_Encoding == ENCODING_WAVE || m_Encoding == ENCODING_AIFF) {
        if (m_sndfile != NULL) {
            sf_close(m_sndfile);
            m_sndfile = NULL;
        }
    }
}
