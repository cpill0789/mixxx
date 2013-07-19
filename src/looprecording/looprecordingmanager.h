#ifndef LOOPRECORDINGMANAGER_H
#define LOOPRECORDINGMANAGER_H

#include <QDesktopServices>
#include <QDateTime>
#include <QObject>


#include "configobject.h"
#include "controlobject.h"
#include "controlobjectthread.h"
#include "controlobjectthreadmain.h"
#include "looprecording/defs_looprecording.h"
#include "trackinfoobject.h"

class EngineMaster;
class ControlPushButton;
class ControlObject;
class ControlObjectThread;
//class TrackPointer;
class TrackInfoObject;

class LoopRecordingManager : public QObject
{
    Q_OBJECT
  public:
    LoopRecordingManager(ConfigObject<ConfigValue>* pConfig, EngineMaster* pEngine);
    virtual ~LoopRecordingManager();
    
    
    // This will try to start recording. If successful, slotIsRecording will be
    // called and a signal isRecording will be emitted.
    void startRecording();
    void stopRecording();
    bool isLoopRecordingActive();
    void setRecordingDir();
    QString& getRecordingDir();
    // Returns the currently recording file
    QString& getRecordingFile();
    QString& getRecordingLocation();
    
  signals:
    // emits the cummulative number of bytes being recorded
    //void bytesRecorded(long);
    
    void isLoopRecording(bool);
    void exportToPlayer(QString, QString);
    void loadToLoopDeck(TrackPointer, QString, bool);
    
  public slots:
    void slotIsLoopRecording(bool);
    void slotClearRecorder();
    void slotLoadToLoopDeck();
    //void slotBytesRecorded(int);
    
  private slots:
    void slotSetLoopRecording(bool recording);
    void slotToggleLoopRecording(double v);
    void slotToggleClear(double v);
    void slotToggleExport(double v);
    void slotChangeLoopSource(double v);
    void slotChangeExportDestination(double v);
    void slotNumDecksChanged(double v);
    void slotNumSamplersChanged(double v);
    
  private:
    void exportLoopToPlayer(QString group);
    void loadToLoopDeck();
    QString formatDateTimeForFilename(QDateTime dateTime) const;
    bool saveLoop(QString newFileLocation);

    ControlObjectThread* m_pRecReady;
    ControlPushButton* m_pToggleLoopRecording;
    ControlPushButton* m_pClearRecorder;
    ControlPushButton* m_pExportLoop;

    ControlPushButton* m_pChangeLoopSource;
    ControlObjectThread* m_pLoopSource;

    ControlObjectThread* m_pLoopPlayReady;
    ControlObject* m_pCOLoopPlayReady;

    ControlObjectThread* m_pNumDecks;
    ControlObjectThread* m_pNumSamplers;

    ControlPushButton* m_pChangeExportDestination;
    ControlObject* m_pCOExportDestination;


    ConfigObject<ConfigValue>* m_pConfig;
    QString m_recordingDir;
    //the base file
    QString m_recording_base_file;
    //filename without path
    QString m_recordingFile;
    //Absolute file
    QString m_recordingLocation;
    
    bool m_isRecording;
    //will be a very large number
    quint64 m_iNumberOfBytesRecored;
    
    QList<QString> m_filesRecorded;
    TrackPointer pTrackToPlay;

    // New filename code.
    QString date_time_str;
    QString encodingType;
    unsigned int m_iLoopNumber;
    unsigned int m_iNumDecks;
    unsigned int m_iNumSamplers;
};

#endif // LOOPRECORDINGMANAGER_H
