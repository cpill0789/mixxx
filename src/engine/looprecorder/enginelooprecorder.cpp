//  enginelooprecorder.cpp
//  Created by Carl Pillot on 6/22/13.
//  Adapted from EngineLoopRecorder.cpp

#include "engine/looprecorder/enginelooprecorder.h"

#include "configobject.h"
//#include "controlobject.h"
//#include "controlobjectthread.h"
#include "errordialoghandler.h"
//#include "playerinfo.h"
#include "looprecording/defs_looprecording.h"
#include "util/timer.h"
#include "util/counter.h"
//#include "sampleutil.h"
#include "engine/looprecorder/loopwriter.h"

#define LOOP_BUFFER_SIZE 16384

EngineLoopRecorder::EngineLoopRecorder(ConfigObject<ConfigValue>* _config)
        : m_config(_config),
        m_bIsWriterReady(false) {

    m_pLoopWriter = new LoopWriter(_config);

    LoopRecorderThread = new QThread;
    LoopRecorderThread->setObjectName(QString("LoopRecorder"));

    // TODO(carl) make sure Thread exits properly.
    //connect(m_pLoopWriter, SIGNAL(error(QString)), this, SLOT(errorString(QString)));
    connect(LoopRecorderThread, SIGNAL(started()), m_pLoopWriter, SLOT(slotStartWriter()));
    connect(m_pLoopWriter, SIGNAL(writerStarted()), this, SLOT(slotWriterStarted()));
    connect(m_pLoopWriter, SIGNAL(finished()), LoopRecorderThread, SLOT(quit()));
    //connect(m_pLoopWriter, SIGNAL(finished()), m_pLoopWriter, SLOT(deleteLater()));
    connect(LoopRecorderThread, SIGNAL(finished()), LoopRecorderThread, SLOT(deleteLater()));

}

EngineLoopRecorder::~EngineLoopRecorder() {
    delete m_pLoopWriter;
}

void EngineLoopRecorder::writeSamples(const CSAMPLE* pBuffer, const int iBufferSize) {
    //ScopedTimer t("EngineLoopRecorder::writeSamples");
    //int samples_written = m_sampleFifo.write(newBuffer, buffer_size);
    
//    if (samples_written != buffer_size) {
//        Counter("EngineLoopRecorder::writeSamples buffer overrun").increment();
//    }
//    
//    if (m_sampleFifo.writeAvailable() < LOOP_BUFFER_SIZE/5) {
//        // Signal to the loop recorder that samples are available.
//        //m_waitForSamples.wakeAll();
//    }
}

void EngineLoopRecorder::startThread() {
    qDebug() << "!~!~!~! EngineLoopRecorder::startThread() !~!~!~!";
    m_pLoopWriter->moveToThread(LoopRecorderThread);
    LoopRecorderThread->start();
}

void EngineLoopRecorder::slotWriterStarted() {
    qDebug() << "!~!~!~! EngineLoopRecorder::slotWriterStarted() !~!~!~!";
    m_bIsWriterReady = true;
}

//void EngineLoopRecorder::run() {
//    
//    QThread::currentThread()->setObjectName(QString("EngineLoopRecorder"));
//    
//    while (!m_bStopThread) {
//
//        // Sleep until samples are available.
//        m_waitLock.lock();
//        m_waitForSamples.wait(&m_waitLock);
//        m_waitLock.unlock();
//
//        int samples_read;
//        while ((samples_read = m_sampleFifo.read(
//                                                 m_pWorkBuffer, LOOP_BUFFER_SIZE))) {
//
//            float currentRecStatus = m_recReady->get();
//
//            // if recording is disabled
//            if (currentRecStatus == LOOP_RECORD_OFF) {
//                //qDebug("Setting record flag to: OFF");
//                if (isFileOpen()) {
//                    closeFile();
//                    emit(isLoopRecording(false));
//                    emit(loadToLoopDeck());
//                }
//
//            } else if (currentRecStatus == LOOP_RECORD_CLEAR) {
//                if (isFileOpen()) {
//                    closeFile();    //close file and free encoder
//                    emit(isLoopRecording(false));
//                }
//                emit(clearRecorder());
//                m_recReady->slotSet(LOOP_RECORD_OFF);
//
//                // if we are ready for recording, i.e, the output file has been selected, we
//                // open a new file
//            } else if (currentRecStatus == LOOP_RECORD_READY) {
//                updateFromPreferences();	//update file location from pref
//
//                if (openFile()) {
//                    qDebug("Setting loop record flag to: ON");
//                    m_recReady->slotSet(LOOP_RECORD_ON);
//                    emit(isLoopRecording(true)); //will notify the LoopRecordingManager
//                } else { // Maybe the encoder could not be initialized
//                    qDebug("Setting loop record flag to: OFF");
//                    m_recReady->slotSet(LOOP_RECORD_OFF);
//                    emit(isLoopRecording(false));
//                }
//                
//                // If recording is enabled process audio to uncompressed data.
//            } else if (currentRecStatus == LOOP_RECORD_ON) {
//                process(m_pWorkBuffer, samples_read);
//            }
//        }
//        
//        // Check to see if we're supposed to exit/stop this thread.
//        if (m_bStopThread) {
//            return;
//        }
//    }
//}

//void EngineLoopRecorder::updateFromPreferences() {
//    m_encoding = m_config->getValueString(ConfigKey(LOOP_RECORDING_PREF_KEY,"Encoding")).toLatin1();
//    m_filename = m_config->getValueString(ConfigKey(LOOP_RECORDING_PREF_KEY,"Path"));
//    // TODO(carl):remove dummy strings and put something intelligent for these values.
//    m_baTitle = QByteArray("No title");
//    m_baAlbum = QByteArray("No album");
//    m_baAuthor = QByteArray("No artist");
//
//    //qDebug() << "Update from preferences: " << m_filename;
//}

//void EngineLoopRecorder::process(const CSAMPLE* pBuffer, const int iBufferSize) {
//
//    // TODO(carl) Start recording to a buffer immediately before a file is opened,
//    // then flush to file once it is open.
//
//    if (m_sndfile != NULL) {
//        sf_write_float(m_sndfile, pBuffer, iBufferSize);
//        emit(samplesRecorded(iBufferSize));
//    }
//}

//bool EngineLoopRecorder::isFileOpen() {
//    // File must be initalized
//    return (m_sndfile != NULL);
//}

//bool EngineLoopRecorder::openFile() {
//    
//    unsigned long samplerate = m_samplerate->get();
//    // set sfInfo
//    m_sfInfo.samplerate = samplerate;
//    m_sfInfo.channels = 2;
//    
//    //if (m_encoding == ENCODING_WAVE)
//        m_sfInfo.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
//    //else
//    //    m_sfInfo.format = SF_FORMAT_AIFF | SF_FORMAT_PCM_16;
//
//    // creates a new WAVE or AIFF file and write header information
//    m_sndfile = sf_open(m_filename.toLocal8Bit(), SFM_WRITE, &m_sfInfo);
//    if (m_sndfile) {
//        sf_command(m_sndfile, SFC_SET_NORM_FLOAT, NULL, SF_FALSE) ;
//        //set meta data
//        int ret;
//    
//        ret = sf_set_string(m_sndfile, SF_STR_TITLE, m_baTitle.data());
//        if(ret != 0)
//            qDebug("libsndfile: %s", sf_error_number(ret));
//        
//        ret = sf_set_string(m_sndfile, SF_STR_ARTIST, m_baAuthor.data());
//        if(ret != 0)
//            qDebug("libsndfile: %s", sf_error_number(ret));
//        
//        ret = sf_set_string(m_sndfile, SF_STR_COMMENT, m_baAlbum.data());
//        if(ret != 0)
//            qDebug("libsndfile: %s", sf_error_number(ret));
//        
//    }
//    
//    // check if file is really open
//    if (!isFileOpen()) {
//        ErrorDialogProperties* props = ErrorDialogHandler::instance()->newDialogProperties();
//        props->setType(DLG_WARNING);
//        props->setTitle(tr("Loop Recording"));
//        props->setText(tr("<html>Could not create audio file for loop recording!<p><br>Maybe you do not have enough free disk space or file permissions.</html>"));
//        ErrorDialogHandler::instance()->requestErrorDialog(props);
//        return false;
//    }
//    return true;
//
//}

//void EngineLoopRecorder::closeFile() {
//    if (m_sndfile != NULL) {
//        sf_close(m_sndfile);
//        m_sndfile = NULL;
//    }
//}
