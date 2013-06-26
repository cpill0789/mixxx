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


//class EngineMaster;
class ControlPushButton;
class ControlObject;
class ControlObjectThread;

class LoopRecordingManager : public QObject
{
    Q_OBJECT
public:
    LoopRecordingManager(ConfigObject<ConfigValue>* pConfig);
    virtual ~LoopRecordingManager();
    
    
    // This will try to start recording If successfuly, slotIsRecording will be
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
    //emits the commulated number of bytes being recorded
    void bytesRecorded(long);
    void isRecording(bool);
    
    //public slots:
    //void slotIsLoopRecording(bool);
    //void slotBytesRecorded(int);
    
    private slots:
    //void slotSetLoopRecording(bool recording);
    void slotToggleLoopRecording(double v);
    
private:
    QString formatDateTimeForFilename(QDateTime dateTime) const;
    ControlObjectThread* m_recReady;
    ControlObject* m_recReadyCO;
    ControlObjectThread* pToggleRecording;
    
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
};

#endif // LOOPRECORDINGMANAGER_H
