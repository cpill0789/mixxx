// looprecordingmanager.cpp
// Created by Carl Pillot on 6/22/13.
// adapted from recordingmanager.cpp

#include <QMutex>
#include <QDir>
#include <QDebug>

#include "recording/looprecordingmanager.h"
#include "recording/defs_looprecording.h"
#include "engine/looprecorder/enginelooprecorder.h"
#include "controlpushbutton.h"
#include "engine/enginemaster.h"

LoopRecordingManager::LoopRecordingManager(ConfigObject<ConfigValue>* pConfig, EngineMaster* pEngine)
: m_pConfig(pConfig),
m_recordingDir(""),
m_recording_base_file(""),
m_recordingFile(""),
m_recordingLocation(""),
m_isRecording(false),
m_iNumberOfBytesRecored(0) {
    m_pToggleRecording = new ControlPushButton(ConfigKey(RECORDING_PREF_KEY, "toggle_recording"));
    connect(m_pToggleRecording, SIGNAL(valueChanged(double)),
            this, SLOT(slotToggleRecording(double)));
    m_recReadyCO = new ControlObject(ConfigKey(RECORDING_PREF_KEY, "status"));
    m_recReady = new ControlObjectThread(m_recReadyCO->getKey());
    
    
    // Register EngineRecord with the engine sidechain.
    // TODO: connect with engine master instead.?
    // EngineSideChain* pSidechain = pEngine->getSideChain();
    //if (pSidechain) {
    //    EngineRecord* pEngineRecord = new EngineRecord(m_pConfig);
    //    connect(pEngineRecord, SIGNAL(isRecording(bool)),
    //            this, SLOT(slotIsRecording(bool)));
    //    connect(pEngineRecord, SIGNAL(bytesRecorded(int)),
    //            this, SLOT(slotBytesRecorded(int)));
    //    pSidechain->addSideChainWorker(pEngineRecord);
    //}
}

LoopRecordingManager::~LoopRecordingManager()
{
    qDebug() << "Delete LoopRecordingManager";
    delete m_recReadyCO;
    delete m_recReady;
}

QString LoopRecordingManager::formatDateTimeForFilename(QDateTime dateTime) const {
    // Use a format based on ISO 8601. Windows does not support colons in
    // filenames so we can't use them anywhere.
    QString formatted = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");
    return formatted;
}

void LoopRecordingManager::slotSetRecording(bool recording) {
    if (recording && !isRecordingActive()) {
        startRecording();
    } else if (!recording && isRecordingActive()) {
        stopRecording();
    }
}

void LoopRecordingManager::slotToggleRecording(double v) {
    if (v > 0) {
        if (isRecordingActive()) {
            stopRecording();
        } else {
            startRecording();
        }
    }
}

void LoopRecordingManager::startRecording() {
    m_iNumberOfBytesRecored = 0;
    //QString encodingType = m_pConfig->getValueString(ConfigKey(RECORDING_PREF_KEY, "Encoding"));
    QString encodingType = "WAV";
    //Append file extension
    QString date_time_str = formatDateTimeForFilename(QDateTime::currentDateTime());
    m_recordingFile = QString("%1.%2")
    .arg(date_time_str, encodingType.toLower());
        
    // Storing the absolutePath of the recording file without file extension
    m_recording_base_file = getRecordingDir();
    m_recording_base_file.append("/").append(date_time_str);
    //appending file extension to get the filelocation
    m_recordingLocation = m_recording_base_file + "."+ encodingType.toLower();
    m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "Path"), m_recordingLocation);
    //m_pConfig->set(ConfigKey(RECORDING_PREF_KEY, "CuePath"), m_recording_base_file +".cue");
    m_recReady->slotSet(RECORD_READY);
}

void LoopRecordingManager::stopRecording()
{
    qDebug() << "Loop Recording stopped";
    m_recReady->slotSet(RECORD_OFF);
    m_recordingFile = "";
    m_recordingLocation = "";
    m_iNumberOfBytesRecored = 0;
}

void LoopRecordingManager::setRecordingDir() {
    // TODO(CARL) get rid of absolute paths;
    QDir recordDir("/Users/carlpillot/mixxx_temp");
    // Note: the default ConfigKey for recordDir is set in DlgPrefRecord::DlgPrefRecord
    
    if (!recordDir.exists()) {
        if (recordDir.mkpath(recordDir.absolutePath())) {
            qDebug() << "Created folder" << recordDir.absolutePath() << "for recordings";
        } else {
            qDebug() << "Failed to create folder" << recordDir.absolutePath() << "for recordings";
        }
    }
    m_recordingDir = recordDir.absolutePath();
    qDebug() << "Loop Recordings folder set to" << m_recordingDir;
}

QString& LoopRecordingManager::getRecordingDir() {
    // Update current recording dir from preferences
    setRecordingDir();
    return m_recordingDir;
}

//Only called when recording is active
//void LoopRecordingManager::slotBytesRecorded(int bytes)
//{
//    //auto conversion to long
//    m_iNumberOfBytesRecored += bytes;
//    if(m_iNumberOfBytesRecored >= m_split_size)
//   {
//        //stop and start recording
//        stopRecording();
//        //Dont generate a new filename
//        //This will reuse the previous filename but appends a suffix
//        startRecording(false);
//    }
//    emit(bytesRecorded(m_iNumberOfBytesRecored));
//}

void LoopRecordingManager::slotIsRecording(bool isRecordingActive)
{
    //qDebug() << "SlotIsRecording " << isRecording;
    
    //Notify the GUI controls, see dlgrecording.cpp
    m_isRecording = isRecordingActive;
    emit(isRecording(isRecordingActive));
}

bool LoopRecordingManager::isRecordingActive() {
    return m_isRecording;
}

QString& LoopRecordingManager::getRecordingFile() {
    return m_recordingFile;
}

QString& LoopRecordingManager::getRecordingLocation() {
    return m_recordingLocation;
}