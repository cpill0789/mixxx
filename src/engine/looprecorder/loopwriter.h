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
    void slotClearWriter();
    void slotSetFile(SNDFILE*);
    void slotStartRecording(int);
    void slotStopRecording(bool);


  signals:
    void clearRecorder();
    void finished();
    void isRecording(bool);
    void samplesAvailable();
    void samplesRecorded(int);
    void loadAudio();

  private slots:
    void slotProcessSamples();

  private:
    void closeFile();
    void writeBuffer(const CSAMPLE* pBuffer, const int iBufferSize);

    FIFO<CSAMPLE> m_sampleFifo;
    CSAMPLE* m_pWorkBuffer;

    bool m_bIsFileAvailable;
    bool m_bIsRecording;
    //unsigned int m_iLoopBreak;
    unsigned int m_iLoopLength;
    //unsigned int m_iLoopRemainder;
    quint64 m_iSamplesRecorded;
    SNDFILE* m_pSndfile;
};

#endif
