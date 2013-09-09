// looptracker.h
// Created by Carl Pillot on 8/23/13.
// Responsible for tracking the playback status of loop layers and mixing them into
// final loops.

#ifndef __LOOPTRACKER_H__
#define __LOOPTRACKER_H__

#include <QtCore>

#include "trackinfoobject.h"

class ControlObjectThread;

class LoopTracker : public QObject {
    Q_OBJECT
  public:
    LoopTracker();
    virtual ~LoopTracker();
    
    void addLoopLayer(QString path, unsigned int length);
    void clear();
    bool finalizeLoop(QString newPath);
    QString getCurrentPath();
    void play();
    void stop(bool clearDeck);
    void setCurrentLength(unsigned int length);

  public slots:
    void slotLoadToLoopDeck();

  signals:
    void loadToLoopDeck(TrackPointer, QString, bool);
    
  private:
    struct LayerInfo {
        QString path;
        unsigned int length;
    };
    QList<LayerInfo*> m_layers;
    int m_iCurrentLayer;
    bool m_bIsUndoAvailable;
    bool m_bIsRedoAvailable;

    ControlObjectThread* m_pLoopDeck1Play;
    ControlObjectThread* m_pLoopDeck1Stop;
    ControlObjectThread* m_pLoopDeck1Eject;
    ControlObjectThread* m_pLoopDeck2Play;
    ControlObjectThread* m_pLoopDeck2Stop;
    ControlObjectThread* m_pLoopDeck2Eject;
    ControlObjectThread* m_pTogglePlayback;
};

#endif