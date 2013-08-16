//  loopwriter.h
//  Created by Carl Pillot on 6/22/13.

#ifndef LOOPWRITER_H
#define LOOPWRITER_H

#include <QtCore>
#include <sndfile.h>

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

  signals:
    void samplesRecorded(int);

  private:

    bool m_isRecording;
    QString m_path;

};

#endif
