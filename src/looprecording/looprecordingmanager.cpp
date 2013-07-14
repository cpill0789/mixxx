// looprecordingmanager.cpp
// Created by Carl Pillot on 6/22/13.
// adapted from recordingmanager.cpp

#include <QMutex>
#include <QDir>
#include <QDebug>
#include <QFile>

#include "looprecording/looprecordingmanager.h"
#include "looprecording/defs_looprecording.h"
#include "engine/looprecorder/enginelooprecorder.h"
#include "controlpushbutton.h"
#include "engine/enginemaster.h"
#include "trackinfoobject.h"


LoopRecordingManager::LoopRecordingManager(ConfigObject<ConfigValue>* pConfig, EngineMaster* pEngine)
        : m_pConfig(pConfig),
        m_recordingDir(""),
        m_recording_base_file(""),
        m_recordingFile(""),
        m_recordingLocation(""),
        m_isRecording(false),
        m_iNumberOfBytesRecored(0),
        pTrackToPlay(NULL) {
    m_pToggleLoopRecording = new ControlPushButton(ConfigKey(LOOP_RECORDING_PREF_KEY, "toggle_loop_recording"));
    connect(m_pToggleLoopRecording, SIGNAL(valueChanged(double)),
            this, SLOT(slotToggleLoopRecording(double)));
    
    m_pClearRecorder = new ControlPushButton(ConfigKey(LOOP_RECORDING_PREF_KEY, "clear_recorder"));
    
    connect(m_pClearRecorder, SIGNAL(valueChanged(double)),
            this, SLOT(slotToggleClear(double)));
    
    m_pExportLoop = new ControlPushButton(ConfigKey(LOOP_RECORDING_PREF_KEY, "export_loop"));
    connect(m_pExportLoop, SIGNAL(valueChanged(double)),
            this, SLOT(slotToggleExport(double)));
    
    //m_recReadyCO = new ControlObject(ConfigKey(LOOP_RECORDING_PREF_KEY, "rec_status"));
    //m_recReady = new ControlObjectThread(m_recReadyCO->getKey());
    m_recReady = new ControlObjectThread(LOOP_RECORDING_PREF_KEY, "rec_status");
    
    
    
    m_loopPlayReadyCO = new ControlObject(
                                ConfigKey(LOOP_RECORDING_PREF_KEY, "play_status"));
    m_loopPlayReady = new ControlObjectThread(m_loopPlayReadyCO->getKey());
    
    m_pConfig->set(ConfigKey(LOOP_RECORDING_PREF_KEY, "Encoding"),QString("WAV"));
    
    // Connect with EngineLoopRecorder
    EngineLoopRecorder* pLoopRecorder = pEngine->getLoopRecorder();
    if (pLoopRecorder) {
        connect(pLoopRecorder, SIGNAL(isLoopRecording(bool)),this, SLOT(slotIsLoopRecording(bool)));
        connect(pLoopRecorder, SIGNAL(clearRecorder()),this, SLOT(slotClearRecorder()));
        connect(pLoopRecorder, SIGNAL(loadToLoopDeck()),this, SLOT(slotLoadToLoopDeck()));
    }
}

LoopRecordingManager::~LoopRecordingManager()
{
    qDebug() << "~LoopRecordingManager";
    // TODO(carl) delete temporary loop recorder files.
    
    //delete m_recReadyCO;
    delete m_recReady;
    delete m_loopPlayReadyCO;
    delete m_loopPlayReady;
    delete m_pToggleLoopRecording;
    delete m_pClearRecorder;
    delete m_pExportLoop;
}

QString LoopRecordingManager::formatDateTimeForFilename(QDateTime dateTime) const {
    // Use a format based on ISO 8601. Windows does not support colons in
    // filenames so we can't use them anywhere.
    QString formatted = dateTime.toString("yyyy-MM-dd_hh'h'mm'm'ss's'");
    return formatted;
}

void LoopRecordingManager::slotSetLoopRecording(bool recording) {
    //qDebug() << "LoopRecordingManager slotSetLoopRecording";
    if (recording && !isLoopRecordingActive()) {
        startRecording();
    } else if (!recording && isLoopRecordingActive()) {
        stopRecording();
    }
}

void LoopRecordingManager::slotToggleLoopRecording(double v) {
    //qDebug() << "LoopRecordingManager::slotToggleLoopRecording v: " << v;
    if (v == 0.) {
        if (isLoopRecordingActive()) {
            stopRecording();
        }
    } else {
        // TODO: Prevent duplicate start call.  Currently startRecording is called twice.
        startRecording();
    }
}

void LoopRecordingManager::slotToggleClear(double v) {
    //qDebug() << "LoopRecordingManager::slotClearRecorder v: " << v;
    if (v > 0.) {
        m_recReady->slotSet(LOOP_RECORD_CLEAR);
        m_pToggleLoopRecording->set(0.);
    }
}

void LoopRecordingManager::slotToggleExport(double v) {
    qDebug() << "LoopRecordingManager::slotToggleExport v: " << v;
    if (v > 0.) {
        exportLoopToPlayer("[Sampler2]");
    }
}

void LoopRecordingManager::slotClearRecorder() {
    
    foreach(QString location, m_filesRecorded) {
        qDebug() << "LoopRecordingManager::slotClearRecorder deleteing: " << location;
        QFile file(location);

        if (file.exists()) {
            file.remove();
        }
    }
    m_filesRecorded.clear();
}

void LoopRecordingManager::slotLoadToLoopDeck(){
    loadToLoopDeck();
}

void LoopRecordingManager::startRecording() {
    qDebug() << "LoopRecordingManager startRecording";
    
    m_iNumberOfBytesRecored = 0;
    QString encodingType = m_pConfig->getValueString(ConfigKey(LOOP_RECORDING_PREF_KEY, "Encoding"));
    //Append file extension
    QString date_time_str = formatDateTimeForFilename(QDateTime::currentDateTime());
    m_recordingFile = QString("%1_%2.%3")
    .arg("loop",date_time_str, encodingType.toLower());
        
    // Storing the absolutePath of the recording file without file extension
    m_recording_base_file = getRecordingDir();
    m_recording_base_file.append("/loop_").append(date_time_str);
    // appending file extension to get the filelocation
    m_recordingLocation = m_recording_base_file + "."+ encodingType.toLower();
    m_filesRecorded << m_recordingLocation;
    m_pConfig->set(ConfigKey(LOOP_RECORDING_PREF_KEY, "Path"), m_recordingLocation);
    m_recReady->slotSet(LOOP_RECORD_READY);
}

void LoopRecordingManager::stopRecording()
{
    qDebug() << "LoopRecordingManager::stopRecording";
    m_recReady->slotSet(LOOP_RECORD_OFF);
    m_recordingFile = "";
    m_recordingLocation = "";
    m_iNumberOfBytesRecored = 0;
}

void LoopRecordingManager::setRecordingDir() {
    QDir recordDir(m_pConfig->getValueString(
                                             ConfigKey("[Recording]", "Directory")).append("/temp"));
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

void LoopRecordingManager::slotIsLoopRecording(bool isRecordingActive) {
    
    //Notify the GUI controls, see dlgrecording.cpp
    m_isRecording = isRecordingActive;
    emit(isLoopRecording(isRecordingActive));
}

bool LoopRecordingManager::isLoopRecordingActive() {
    return m_isRecording;
}

QString& LoopRecordingManager::getRecordingFile() {
    return m_recordingFile;
}

QString& LoopRecordingManager::getRecordingLocation() {
    return m_recordingLocation;
}

void LoopRecordingManager::exportLoopToPlayer(QString group) {
    //qDebug() << "LoopRecordingManager::exportLoopToPlayer m_filesRecorded: " << m_filesRecorded;
    
    // TODO(carl) handle multi-layered loops.
    QString dir = getRecordingDir();
    QString encodingType = m_pConfig->getValueString(ConfigKey(LOOP_RECORDING_PREF_KEY, "Encoding"));
    //Append file extension
    QString date_time_str = formatDateTimeForFilename(QDateTime::currentDateTime());
    
    QString newFileLocation = QString("%1%2_%3.%4")
    .arg(dir,"loop",date_time_str, encodingType.toLower());
    
    
    if (saveLoop(newFileLocation)) {
        emit(exportToPlayer(newFileLocation, group));
    } else {
        qDebug () << "LoopRecordingManager::exportLoopToPlayer Error Saving File: " << newFileLocation;
    }
}
void LoopRecordingManager::loadToLoopDeck() {
    //qDebug() << "LoopRecordingManager::loadToLoopDeck m_filesRecorded: " << m_filesRecorded;
    if (!m_filesRecorded.isEmpty()) {
        
        pTrackToPlay = TrackPointer(new TrackInfoObject(m_filesRecorded.last()), &QObject::deleteLater);
        emit(loadToLoopDeck(pTrackToPlay, QString("[LoopRecorderDeck1]"), true));
    }
}

bool LoopRecordingManager::saveLoop(QString newFileLocation) {
    
    // Right now just save the last recording created.
    // In the future this will be more complex.
    if (!m_filesRecorded.isEmpty()) {
        
        QString oldFileLocation = m_filesRecorded.last();
        QFile file(oldFileLocation);
        
        if (file.exists()) {
            return file.copy(newFileLocation);
        }

    }
    return false;
}