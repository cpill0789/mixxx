//  loopwriter.cpp
//  Created by Carl Pillot on 8/15/13.
//  Writes audio to a file for loop recording.

#include "engine/looprecorder/loopwriter.h"

#include "defs.h"
#include "sampleutil.h"

#define LOOP_BUFFER_SIZE 16384

LoopWriter::LoopWriter()
        : m_sampleFifo(LOOP_BUFFER_SIZE),
        m_pWorkBuffer(SampleUtil::alloc(LOOP_BUFFER_SIZE)),
        m_bIsFileAvailable(false),
        m_bIsRecording(false),
        m_iLoopLength(0),
        m_pSndfile(NULL) {
    connect(this, SIGNAL(samplesAvailable()), this, SLOT(slotProcessSamples()));
}

LoopWriter::~LoopWriter() {
    qDebug() << "!~!~!~!~!~! Loop writer deleted !~!~!~!~!~!~!";
    SampleUtil::free(m_pWorkBuffer);
    emit(finished());
}

void LoopWriter::process(const CSAMPLE* pBuffer, const int iBufferSize) {
    //ScopedTimer t("EngineLoopRecorder::writeSamples");
    int samples_written = m_sampleFifo.write(newBuffer, buffer_size);

    if (samples_written != buffer_size) {
        Counter("EngineLoopRecorder::writeSamples buffer overrun").increment();
    }

    if (m_sampleFifo.writeAvailable() < LOOP_BUFFER_SIZE/5) {
        Signal to the loop recorder that samples are available.
        emit(samplesAvailable());
    }
}

void LoopWriter::slotSetFile(SNDFILE* pFile) {
    if (!m_bIsFileOpen) {
        if (pFile != NULL) {
            m_pSndfile = pFile;
            m_bIsFileAvailable = true;
        }
    }
}

void LoopWriter::slotStartRecording(int samples) {
    qDebug() << "!~!~!~!~!~! LoopWriter::slotStartRecording Length: " << samples << " !~!~!~!~!~!~!";
    m_iLoopLength = samples;
    m_bIsRecording = true;
}

void LoopWriter::slotStopRecording() {
    m_bIsRecording = false;
    else {
        // TODO(carl) check if temp buffers are open and clear them.

        if (m_bIsFileAvailable) {
            closeFile();
        }
    }

}

void LoopWriter::slotProcessSamples() {
    int samples_read;
    while ((samples_read = m_sampleFifo.read(m_pWorkBuffer, LOOP_BUFFER_SIZE))) {

        // States for recording:
        // 1. not recording
        // 2. Recording, but file not yet open
        // 3. Recording, file open
        // 4. Stopping recording (closing file, sending to deck)

        if (m_bIsRecording) {
            writeBuffer(m_pWorkBuffer, samples_read);
        }
    }
}

void LoopWriter::closeFile() {
    if(m_pSndfile != NULL) {
        sf_close(m_pSndfile);
        // is this NULL assignment necessary?
        m_pSndfile = NULL;
    }
}

void LoopWriter::writeBuffer(const CSAMPLE* pBuffer, const int iBufferSize) {
    if (m_bIsFileOpen) {
        // TODO(carl) check for buffers to flush
        sf_write_float(m_pSndfile, pBuffer, iBufferSize);
    } else {
        // TODO(carl) write to temporary buffer.
    }

}
