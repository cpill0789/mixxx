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
    
  public slots:
    void slotClearRecorder();
    void slotIsLoopRecording(bool);
    void slotLoadToLoopDeck();

  signals:
    void exportToPlayer(QString, QString);
    void isLoopRecording(bool);
    void loadToLoopDeck(TrackPointer, QString, bool);

  private slots:
    void slotChangeExportDestination(double v);
    void slotChangeLoopSource(double v);
    void slotNumDecksChanged(double v);
    void slotNumSamplersChanged(double v);
    void slotSetLoopRecording(bool recording);
    void slotToggleClear(double v);
    void slotToggleExport(double v);
    void slotToggleLoopRecording(double v);
    void slotTogglePlay(double v);
    void slotToggleStopPlayback(double v);

  private:
    void exportLoopToPlayer(QString group);
    QString formatDateTimeForFilename(QDateTime dateTime) const;
    bool saveLoop(QString newFileLocation);
    void setRecordingDir();

    ConfigObject<ConfigValue>* m_pConfig;
    
    ControlObject* m_pCOExportDestination;
    ControlObject* m_pCOLoopPlayReady;

    ControlObjectThread* m_pLoopPlayReady;
    ControlObjectThread* m_pLoopSource;
    ControlObjectThread* m_pNumDecks;
    ControlObjectThread* m_pNumSamplers;
    ControlObjectThread* m_pRecReady;

    ControlObjectThread* m_pLoopDeck1Play;
    ControlObjectThread* m_pLoopDeck1Stop;
    ControlObjectThread* m_pLoopDeck1Eject;

    ControlPushButton* m_pChangeExportDestination;
    ControlPushButton* m_pChangeLoopSource;
    ControlPushButton* m_pClearRecorder;
    ControlPushButton* m_pExportLoop;
    ControlPushButton* m_pToggleLoopRecording;

    // Stores paths of all files recorded.
    QList<QString> m_filesRecorded;

    QString date_time_str;
    QString encodingType;
    QString m_recordingDir;

    //the base file
    QString m_recording_base_file;
    //filename without path
    QString m_recordingFile;
    //Absolute file
    QString m_recordingLocation;
    
    bool m_isRecording;

    unsigned int m_iLoopNumber;
    unsigned int m_iNumDecks;
    unsigned int m_iNumSamplers;
};

#endif // LOOPRECORDINGMANAGER_H
