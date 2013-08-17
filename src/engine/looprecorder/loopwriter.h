//  loopwriter.h
//  Created by Carl Pillot on 8/15/13.

#ifndef LOOPWRITER_H
#define LOOPWRITER_H

#include <QtCore>
#include <sndfile.h>

#include "defs.h"
#include "util/fifo.h"

class LoopWriter : public QObject {
    Q_OBJECT
  public:
    LoopWriter();
    virtual ~LoopWriter();

    void process(const CSAMPLE* pBuffer, const int iBufferSize);

  public slots:
    void slotStartRecording(int);
    void slotStopRecording();

  signals:
    void samplesRecorded(int);
    void finished();
    void samplesAvailable();

  private slots:
    void slotProcessSamples();

  private:
    void closeFile();
    void recordBuffer();


    ConfigObject<ConfigValue>* m_pConfig;

    FIFO<CSAMPLE> m_sampleFifo;
    CSAMPLE* m_pWorkBuffer;

    bool m_bIsRecording;
    bool m_bIsFileAvailable;
    unsigned int m_iLoopLength;
    SNDFILE* m_pSndfile;
};

#endif
