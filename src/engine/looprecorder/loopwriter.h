//  loopwriter.h
//  Created by Carl Pillot on 8/15/13.

#ifndef LOOPWRITER_H
#define LOOPWRITER_H

#include <QtCore>
#include <sndfile.h>

#include "configobject.h"
#include "defs.h"
#include "util/fifo.h"

class LoopWriter : public QObject {
    Q_OBJECT
  public:
    LoopWriter(ConfigObject<ConfigValue>* _config);
    virtual ~LoopWriter();

    void process(const CSAMPLE* pBuffer, const int iBufferSize);

  public slots:
    void slotSetPath(QString);
    void slotStartRecording(int);
    void slotStopRecording();
    void slotStartWriter();

  signals:
    void samplesRecorded(int);
    void finished();

  private:

    ConfigObject<ConfigValue>* m_pConfig;

    FIFO<CSAMPLE> m_sampleFifo;
    CSAMPLE* m_pWorkBuffer;

    // Provides thread safety around the wait condition below.
    QMutex m_waitLock;
    // Allows sleeping until we have samples to process.
    QWaitCondition m_waitForSamples;

    bool m_isRecording;
    QString m_path;

};

#endif
