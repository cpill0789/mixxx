/***************************************************************************
                          mixxx.cpp  -  description
                             -------------------
    begin                : Mon Feb 18 09:48:17 CET 2002
    copyright            : (C) 2002 by Tue and Ken Haste Andersen
    email                :
***************************************************************************/

/***************************************************************************
*                                                                         *
*   This program is free software; you can redistribute it and/or modify  *
*   it under the terms of the GNU General Public License as published by  *
*   the Free Software Foundation; either version 2 of the License, or     *
*   (at your option) any later version.                                   *
*                                                                         *
***************************************************************************/

#include <QtDebug>
#include <QtCore>
#include <QtGui>
#include <QTranslator>
#include <QMenu>
#include <QMenuBar>
#include <QFileDialog>
#include <QDesktopWidget>

#include "mixxx.h"

#include "analyserqueue.h"
#include "controlobjectthreadmain.h"
#include "controlpotmeter.h"
#include "deck.h"
#include "defs_urls.h"
#include "dlgabout.h"
#include "dlgpreferences.h"
#include "engine/enginemaster.h"
#include "engine/enginemicrophone.h"
#include "engine/enginepassthrough.h"
#include "library/library.h"
#include "library/libraryscanner.h"
#include "library/librarytablemodel.h"
#include "controllers/controllermanager.h"
#include "mixxxkeyboard.h"
#include "playermanager.h"
#include "recording/defs_recording.h"
#include "recording/recordingmanager.h"
#include "shoutcast/shoutcastmanager.h"
#include "looprecording/looprecordingmanager.h"
#include "skin/legacyskinparser.h"
#include "skin/skinloader.h"
#include "soundmanager.h"
#include "soundmanagerutil.h"
#include "soundsourceproxy.h"
#include "trackinfoobject.h"
#include "upgrade.h"
#include "waveform/waveformwidgetfactory.h"
#include "widget/wwaveformviewer.h"
#include "widget/wwidget.h"
#include "widget/wspinny.h"
#include "sharedglcontext.h"
#include "util/debug.h"
#include "util/statsmanager.h"
#include "util/timer.h"
#include "util/version.h"
#include "util/compatibility.h"
#include "playerinfo.h"

#ifdef __VINYLCONTROL__
#include "vinylcontrol/defs_vinylcontrol.h"
#include "vinylcontrol/vinylcontrolmanager.h"
#endif

#ifdef __MODPLUG__
#include "dlgprefmodplug.h"
#endif

extern "C" void crashDlg()
{
    QMessageBox::critical(0, "Mixxx",
        "Mixxx has encountered a serious error and needs to close.");
}


bool loadTranslations(const QLocale& systemLocale, QString userLocale,
                      const QString& translation, const QString& prefix,
                      const QString& translationPath, QTranslator* pTranslator) {
    if (userLocale.size() == 0) {
#if QT_VERSION >= 0x040800
        QStringList uiLanguages = systemLocale.uiLanguages();
        if (uiLanguages.size() > 0 && uiLanguages.first() == "en") {
            // Don't bother loading a translation if the first ui-langauge is
            // English because the interface is already in English. This fixes
            // the case where the user's install of Qt doesn't have an explicit
            // English translation file and the fact that we don't ship a
            // mixxx_en.qm.
            return false;
        }
        return pTranslator->load(systemLocale, translation, prefix, translationPath);
#else
        userLocale = systemLocale.name();
#endif  // QT_VERSION
    }
    return pTranslator->load(translation + prefix + userLocale, translationPath);
}

void MixxxApp::logBuildDetails() {
    QString version = Version::version();
    QString buildBranch = Version::developmentBranch();
    QString buildRevision = Version::developmentRevision();
    QString buildFlags = Version::buildFlags();

    QStringList buildInfo;
    if (!buildBranch.isEmpty() && !buildRevision.isEmpty()) {
        buildInfo.append(
            QString("git %1 r%2").arg(buildBranch, buildRevision));
    } else if (!buildRevision.isEmpty()) {
        buildInfo.append(
            QString("git r%2").arg(buildRevision));
    }
    buildInfo.append("built on: " __DATE__ " @ " __TIME__);
    if (!buildFlags.isEmpty()) {
        buildInfo.append(QString("flags: %1").arg(buildFlags.trimmed()));
    }
    QString buildInfoFormatted = QString("(%1)").arg(buildInfo.join("; "));

    // This is the first line in mixxx.log
    qDebug() << "Mixxx" << version << buildInfoFormatted << "is starting...";
    qDebug() << "Qt version is:" << qVersion();
}

void MixxxApp::initializeWindow() {
    QString version = Version::version();
#ifdef __APPLE__
    setWindowTitle(tr("Mixxx")); //App Store
#elif defined(AMD64) || defined(EM64T) || defined(x86_64)
    setWindowTitle(tr("Mixxx %1 x64").arg(version));
#elif defined(IA64)
    setWindowTitle(tr("Mixxx %1 Itanium").arg(version));
#else
    setWindowTitle(tr("Mixxx %1").arg(version));
#endif
    setWindowIcon(QIcon(":/images/ic_mixxx_window.png"));
}

void MixxxApp::initializeTranslations(QApplication* pApp) {
    QString resourcePath = m_pConfig->getResourcePath();
    QString translationsFolder = resourcePath + "translations/";

    // Load Qt base translations
    QString userLocale = m_cmdLineArgs.getLocale();
    QLocale systemLocale = QLocale::system();

    // Attempt to load user locale from config
    if (userLocale == "") {
        userLocale = m_pConfig->getValueString(ConfigKey("[Config]","Locale"));
    }

    // Load Qt translations for this locale from the system translation
    // path. This is the lowest precedence QTranslator.
    QTranslator* qtTranslator = new QTranslator(pApp);
    if (loadTranslations(systemLocale, userLocale, "qt", "_",
                         QLibraryInfo::location(QLibraryInfo::TranslationsPath),
                         qtTranslator)) {
        pApp->installTranslator(qtTranslator);
    } else {
        delete qtTranslator;
    }

    // Load Qt translations for this locale from the Mixxx translations
    // folder.
    QTranslator* mixxxQtTranslator = new QTranslator(pApp);
    if (loadTranslations(systemLocale, userLocale, "qt", "_",
                         translationsFolder,
                         mixxxQtTranslator)) {
        pApp->installTranslator(mixxxQtTranslator);
    } else {
        delete mixxxQtTranslator;
    }

    // Load Mixxx specific translations for this locale from the Mixxx
    // translations folder.
    QTranslator* mixxxTranslator = new QTranslator(pApp);
    bool mixxxLoaded = loadTranslations(systemLocale, userLocale, "mixxx", "_",
                                        translationsFolder, mixxxTranslator);
    qDebug() << "Loading translations for locale"
             << (userLocale.size() > 0 ? userLocale : systemLocale.name())
             << "from translations folder" << translationsFolder << ":"
             << (mixxxLoaded ? "success" : "fail");
    if (mixxxLoaded) {
        pApp->installTranslator(mixxxTranslator);
    } else {
        delete mixxxTranslator;
    }
}

void MixxxApp::initializeKeyboard() {
    QString resourcePath = m_pConfig->getResourcePath();

    // Set the default value in settings file
    if (m_pConfig->getValueString(ConfigKey("[Keyboard]","Enabled")).length() == 0)
        m_pConfig->set(ConfigKey("[Keyboard]","Enabled"), ConfigValue(1));

    // Read keyboard configuration and set kdbConfig object in WWidget
    // Check first in user's Mixxx directory
    QString userKeyboard = m_cmdLineArgs.getSettingsPath() + "Custom.kbd.cfg";

    //Empty keyboard configuration
    m_pKbdConfigEmpty = new ConfigObject<ConfigValueKbd>("");

    if (QFile::exists(userKeyboard)) {
        qDebug() << "Found and will use custom keyboard preset" << userKeyboard;
        m_pKbdConfig = new ConfigObject<ConfigValueKbd>(userKeyboard);
    } else {
        // Default to the locale for the main input method (e.g. keyboard).
        QLocale locale = inputLocale();

        // check if a default keyboard exists
        QString defaultKeyboard = QString(resourcePath).append("keyboard/");
        defaultKeyboard += locale.name();
        defaultKeyboard += ".kbd.cfg";

        if (!QFile::exists(defaultKeyboard)) {
            qDebug() << defaultKeyboard << " not found, using en_US.kbd.cfg";
            defaultKeyboard = QString(resourcePath).append("keyboard/").append("en_US.kbd.cfg");
            if (!QFile::exists(defaultKeyboard)) {
                qDebug() << defaultKeyboard << " not found, starting without shortcuts";
                defaultKeyboard = "";
            }
        }
        m_pKbdConfig = new ConfigObject<ConfigValueKbd>(defaultKeyboard);
    }

    // TODO(XXX) leak pKbdConfig, MixxxKeyboard owns it? Maybe roll all keyboard
    // initialization into MixxxKeyboard
    // Workaround for today: MixxxKeyboard calls delete
    bool keyboardShortcutsEnabled = m_pConfig->getValueString(
        ConfigKey("[Keyboard]", "Enabled")) == "1";
    m_pKeyboard = new MixxxKeyboard(keyboardShortcutsEnabled ? m_pKbdConfig : m_pKbdConfigEmpty);
}

MixxxApp::MixxxApp(QApplication *pApp, const CmdlineArgs& args)
        : m_runtime_timer("MixxxApp::runtime"),
          m_cmdLineArgs(args),
          m_pVinylcontrol1Enabled(NULL),
          m_pVinylcontrol2Enabled(NULL) {
    logBuildDetails();
    ScopedTimer t("MixxxApp::MixxxApp");
    m_runtime_timer.start();
    initializeWindow();

    // Only record stats in developer mode.
    if (m_cmdLineArgs.getDeveloper()) {
        StatsManager::create();
    }

    //Reset pointer to players
    m_pSoundManager = NULL;
    m_pPrefDlg = NULL;
    m_pControllerManager = NULL;
    m_pRecordingManager = NULL;
#ifdef __SHOUTCAST__
    m_pShoutcastManager = NULL;
#endif
    m_pLoopRecordingManager = NULL;
    bool bLoopRecordEnabled = m_cmdLineArgs.getLoopRecorder();

    qDebug() << "!~!~!~!~!~!~!~!~! Loop Recorder Command Line Arg: " << bLoopRecordEnabled;

    // Check to see if this is the first time this version of Mixxx is run
    // after an upgrade and make any needed changes.
    Upgrade upgrader;
    m_pConfig = upgrader.versionUpgrade(args.getSettingsPath());

    QString resourcePath = m_pConfig->getResourcePath();

    initializeTranslations(pApp);

    // Set the visibility of tooltips, default "1" = ON
    m_toolTipsCfg = m_pConfig->getValueString(ConfigKey("[Controls]", "Tooltips"), "1").toInt();

    // Store the path in the config database
    m_pConfig->set(ConfigKey("[Config]", "Path"), ConfigValue(resourcePath));

    initializeKeyboard();

    // Starting the master (mixing of the channels and effects):
    m_pEngine = new EngineMaster(m_pConfig, "[Master]", true, true, bLoopRecordEnabled);

    m_pRecordingManager = new RecordingManager(m_pConfig, m_pEngine);
#ifdef __SHOUTCAST__
    m_pShoutcastManager = new ShoutcastManager(m_pConfig, m_pEngine);
#endif

    // Initialize player device
    // while this is created here, setupDevices needs to be called sometime
    // after the players are added to the engine (as is done currently) -- bkgood
    m_pSoundManager = new SoundManager(m_pConfig, m_pEngine);

    // TODO(rryan): Fold microphone and passthrough creation into a manager
    // (e.g. PlayerManager, though they aren't players).

    EngineMicrophone* pMicrophone = new EngineMicrophone("[Microphone]");
    // What should channelbase be?
    AudioInput micInput = AudioInput(AudioPath::MICROPHONE, 0, 0);
    m_pEngine->addChannel(pMicrophone);
    m_pSoundManager->registerInput(micInput, pMicrophone);

    EnginePassthrough* pPassthrough1 = new EnginePassthrough("[Passthrough1]");
    // What should channelbase be?
    AudioInput passthroughInput1 = AudioInput(AudioPath::EXTPASSTHROUGH, 0, 0);
    m_pEngine->addChannel(pPassthrough1);
    m_pSoundManager->registerInput(passthroughInput1, pPassthrough1);

    EnginePassthrough* pPassthrough2 = new EnginePassthrough("[Passthrough2]");
    // What should channelbase be?
    AudioInput passthroughInput2 = AudioInput(AudioPath::EXTPASSTHROUGH, 0, 1);
    m_pEngine->addChannel(pPassthrough2);
    m_pSoundManager->registerInput(passthroughInput2, pPassthrough2);

    // Get Music dir
    bool hasChanged_MusicDir = false;
    QDir dir(m_pConfig->getValueString(ConfigKey("[Playlist]","Directory")));
    if (m_pConfig->getValueString(
        ConfigKey("[Playlist]","Directory")).length() < 1 || !dir.exists())
    {
        // TODO this needs to be smarter, we can't distinguish between an empty
        // path return value (not sure if this is normally possible, but it is
        // possible with the Windows 7 "Music" library, which is what
        // QDesktopServices::storageLocation(QDesktopServices::MusicLocation)
        // resolves to) and a user hitting 'cancel'. If we get a blank return
        // but the user didn't hit cancel, we need to know this and let the
        // user take some course of action -- bkgood
        QString fd = QFileDialog::getExistingDirectory(
            this, tr("Choose music library directory"),
            QDesktopServices::storageLocation(QDesktopServices::MusicLocation));

        if (fd != "")
        {
            m_pConfig->set(ConfigKey("[Playlist]","Directory"), fd);
            m_pConfig->Save();
            hasChanged_MusicDir = true;
        }
    }
    // Do not write meta data back to ID3 when meta data has changed
    // Because multiple TrackDao objects can exists for a particular track
    // writing meta data may ruine your MP3 file if done simultaneously.
    // see Bug #728197
    // For safety reasons, we deactivate this feature.
    m_pConfig->set(ConfigKey("[Library]","WriteAudioTags"), ConfigValue(0));


    // library dies in seemingly unrelated qtsql error about not having a
    // sqlite driver if this path doesn't exist. Normally config->Save()
    // above would make it but if it doesn't get run for whatever reason
    // we get hosed -- bkgood
    if (!QDir(args.getSettingsPath()).exists()) {
        QDir().mkpath(args.getSettingsPath());
    }

    // Register TrackPointer as a metatype since we use it in signals/slots
    // regularly.
    qRegisterMetaType<TrackPointer>("TrackPointer");

#ifdef __VINYLCONTROL__
    m_pVCManager = new VinylControlManager(this, m_pConfig, m_pSoundManager);
#else
    m_pVCManager = NULL;
#endif

    // Create the player manager.
    m_pPlayerManager = new PlayerManager(m_pConfig, m_pSoundManager, m_pEngine);
    m_pPlayerManager->addDeck();
    m_pPlayerManager->addDeck();
    m_pPlayerManager->addSampler();
    m_pPlayerManager->addSampler();
    m_pPlayerManager->addSampler();
    m_pPlayerManager->addSampler();
    m_pPlayerManager->addPreviewDeck();
    m_pPlayerManager->addLoopRecorderDeck();
    m_pPlayerManager->addLoopRecorderDeck();

#ifdef __VINYLCONTROL__
    m_pVCManager->init();
#endif

#ifdef __MODPLUG__
    // restore the configuration for the modplug library before trying to load a module
    DlgPrefModplug* pModplugPrefs = new DlgPrefModplug(0, m_pConfig);
    pModplugPrefs->loadSettings();
    pModplugPrefs->applySettings();
    delete pModplugPrefs; // not needed anymore
#endif

    m_pLibrary = new Library(this, m_pConfig,
                             m_pRecordingManager);
    m_pPlayerManager->bindToLibrary(m_pLibrary);

    // Initialize loop recording manager.
    if (bLoopRecordEnabled) {
        m_pLoopRecordingManager = new LoopRecordingManager(m_pConfig, m_pEngine);
        m_pPlayerManager->bindToLoopRecorder(m_pLoopRecordingManager);
    }

    // Call inits to invoke all other construction parts

    // Intialize default BPM system values
    if (m_pConfig->getValueString(ConfigKey("[BPM]", "BPMRangeStart"))
            .length() < 1)
    {
        m_pConfig->set(ConfigKey("[BPM]", "BPMRangeStart"),ConfigValue(65));
    }

    if (m_pConfig->getValueString(ConfigKey("[BPM]", "BPMRangeEnd"))
            .length() < 1)
    {
        m_pConfig->set(ConfigKey("[BPM]", "BPMRangeEnd"),ConfigValue(135));
    }

    if (m_pConfig->getValueString(ConfigKey("[BPM]", "AnalyzeEntireSong"))
            .length() < 1)
    {
        m_pConfig->set(ConfigKey("[BPM]", "AnalyzeEntireSong"),ConfigValue(1));
    }

    // Initialize controller sub-system,
    //  but do not set up controllers until the end of the application startup
    qDebug() << "Creating ControllerManager";
    m_pControllerManager = new ControllerManager(m_pConfig);

    WaveformWidgetFactory::create();
    WaveformWidgetFactory::instance()->setConfig(m_pConfig);

    m_pSkinLoader = new SkinLoader(m_pConfig);
    connect(this, SIGNAL(newSkinLoaded()),
            this, SLOT(onNewSkinLoaded()));
    connect(this, SIGNAL(newSkinLoaded()),
            m_pLibrary, SLOT(onSkinLoadFinished()));

    // Initialize preference dialog
    m_pPrefDlg = new DlgPreferences(this, m_pSkinLoader, m_pSoundManager, m_pPlayerManager,
                                    m_pControllerManager, m_pVCManager, m_pConfig);
    m_pPrefDlg->setWindowIcon(QIcon(":/images/ic_mixxx_window.png"));
    m_pPrefDlg->setHidden(true);

    // Try open player device If that fails, the preference panel is opened.
    int setupDevices = m_pSoundManager->setupDevices();
    unsigned int numDevices = m_pSoundManager->getConfig().getOutputs().count();
    // test for at least one out device, if none, display another dlg that
    // says "mixxx will barely work with no outs"
    while (setupDevices != OK || numDevices == 0)
    {
        // Exit when we press the Exit button in the noSoundDlg dialog
        // only call it if setupDevices != OK
        if (setupDevices != OK) {
            if (noSoundDlg() != 0) {
                exit(0);
            }
        } else if (numDevices == 0) {
            bool continueClicked = false;
            int noOutput = noOutputDlg(&continueClicked);
            if (continueClicked) break;
            if (noOutput != 0) {
                exit(0);
            }
        }
        setupDevices = m_pSoundManager->setupDevices();
        numDevices = m_pSoundManager->getConfig().getOutputs().count();
    }

    // Load tracks in args.qlMusicFiles (command line arguments) into player
    // 1 and 2:
    for (int i = 0; i < (int)m_pPlayerManager->numDecks()
            && i < args.getMusicFiles().count(); ++i) {
        if ( SoundSourceProxy::isFilenameSupported(args.getMusicFiles().at(i))) {
            m_pPlayerManager->slotLoadToDeck(args.getMusicFiles().at(i), i+1);
        }
    }

    initActions();
    initMenuBar();

    // Before creating the first skin we need to create a QGLWidget so that all
    // the QGLWidget's we create can use it as a shared QGLContext.
    QGLWidget* pContextWidget = new QGLWidget(this);
    pContextWidget->hide();
    SharedGLContext::setWidget(pContextWidget);

    // Use frame as container for view, needed for fullscreen display
    m_pView = new QFrame();

    m_pWidgetParent = NULL;
    // Loads the skin as a child of m_pView
    // assignment intentional in next line
    if (!(m_pWidgetParent = m_pSkinLoader->loadDefaultSkin(
        m_pView, m_pKeyboard, m_pPlayerManager, m_pControllerManager, m_pLibrary, m_pLoopRecordingManager, m_pVCManager))) {
        reportCriticalErrorAndQuit("default skin cannot be loaded see <b>mixxx</b> trace for more information.");

        //TODO (XXX) add dialog to warn user and launch skin choice page
        resize(640,480);
    } else {
        // keep gui centered (esp for fullscreen)
        m_pView->setLayout( new QHBoxLayout(m_pView));
        m_pView->layout()->setContentsMargins(0,0,0,0);
        m_pView->layout()->addWidget(m_pWidgetParent);

        // this has to be after the OpenGL widgets are created or depending on a
        // million different variables the first waveform may be horribly
        // corrupted. See bug 521509 -- bkgood ?? -- vrince
        setCentralWidget(m_pView);
    }

    //move the app in the center of the primary screen
    slotToCenterOfPrimaryScreen();

    // Check direct rendering and warn user if they don't have it
    checkDirectRendering();

    //Install an event filter to catch certain QT events, such as tooltips.
    //This allows us to turn off tooltips.
    pApp->installEventFilter(this); // The eventfilter is located in this
                                      // Mixxx class as a callback.

    // If we were told to start in fullscreen mode on the command-line,
    // then turn on fullscreen mode.
    if (args.getStartInFullscreen()) {
        slotViewFullScreen(true);
    }
    emit(newSkinLoaded());

    // Refresh the GUI (workaround for Qt 4.6 display bug)
    /* // TODO(bkgood) delete this block if the moving of setCentralWidget
     * //              totally fixes this first-wavefore-fubar issue for
     * //              everyone
    QString QtVersion = qVersion();
    if (QtVersion>="4.6.0") {
        qDebug() << "Qt v4.6.0 or higher detected. Using rebootMixxxView() "
            "workaround.\n    (See bug https://bugs.launchpad.net/mixxx/"
            "+bug/521509)";
        rebootMixxxView();
    } */

    // Wait until all other ControlObjects are set up
    //  before initializing controllers
    m_pControllerManager->setUpDevices();

    // Scan the library for new files and directories
    bool rescan = (bool)m_pConfig->getValueString(ConfigKey("[Library]","RescanOnStartup")).toInt();
    // rescan the library if we get a new plugin
    QSet<QString> prev_plugins = QSet<QString>::fromList(m_pConfig->getValueString(
        ConfigKey("[Library]", "SupportedFileExtensions")).split(",", QString::SkipEmptyParts));
    QSet<QString> curr_plugins = QSet<QString>::fromList(
        SoundSourceProxy::supportedFileExtensions());
    rescan = rescan || (prev_plugins != curr_plugins);
    m_pConfig->set(ConfigKey("[Library]", "SupportedFileExtensions"),
        QStringList(SoundSourceProxy::supportedFileExtensions()).join(","));

    // Scan the library directory. Initialize this after the skinloader has
    // loaded a skin, see Bug #1047435
    m_pLibraryScanner = new LibraryScanner(m_pLibrary->getTrackCollection());
    connect(m_pLibraryScanner, SIGNAL(scanFinished()),
            this, SLOT(slotEnableRescanLibraryAction()));

    //Refresh the library models when the library (re)scan is finished.
    connect(m_pLibraryScanner, SIGNAL(scanFinished()),
            m_pLibrary, SLOT(slotRefreshLibraryModels()));

    if (rescan || hasChanged_MusicDir) {
        m_pLibraryScanner->scan(
            m_pConfig->getValueString(ConfigKey("[Playlist]", "Directory")), this);
    }
}

MixxxApp::~MixxxApp() {
    // TODO(rryan): Get rid of QTime here.
    QTime qTime;
    qTime.start();
    Timer t("MixxxApp::~MixxxApp");
    t.start();

    qDebug() << "Destroying MixxxApp";

    qDebug() << "save config " << qTime.elapsed();
    m_pConfig->Save();

    // SoundManager depend on Engine and Config
    qDebug() << "delete soundmanager " << qTime.elapsed();
    delete m_pSoundManager;

    // View depends on MixxxKeyboard, PlayerManager, Library
    qDebug() << "delete view " << qTime.elapsed();
    delete m_pView;

    // SkinLoader depends on Config
    qDebug() << "delete SkinLoader " << qTime.elapsed();
    delete m_pSkinLoader;

    // ControllerManager depends on Config
    qDebug() << "delete ControllerManager " << qTime.elapsed();
    delete m_pControllerManager;

#ifdef __VINYLCONTROL__
    // VinylControlManager depends on a CO the engine owns
    // (vinylcontrol_enabled in VinylControlControl)
    qDebug() << "delete vinylcontrolmanager " << qTime.elapsed();
    delete m_pVCManager;
    delete m_pVinylcontrol1Enabled;
    delete m_pVinylcontrol2Enabled;
#endif
    // PlayerManager depends on Engine, SoundManager, VinylControlManager, and Config
    qDebug() << "delete playerManager " << qTime.elapsed();
    delete m_pPlayerManager;

    // LibraryScanner depends on Library
    qDebug() << "delete library scanner " <<  qTime.elapsed();
    delete m_pLibraryScanner;

    // Delete the library after the view so there are no dangling pointers to
    // Depends on RecordingManager
    // the data models.
    qDebug() << "delete library " << qTime.elapsed();
    delete m_pLibrary;

    // RecordingManager depends on config, engine
    qDebug() << "delete RecordingManager " << qTime.elapsed();
    delete m_pRecordingManager;

#ifdef __SHOUTCAST__
    // ShoutcastManager depends on config, engine
    qDebug() << "delete ShoutcastManager " << qTime.elapsed();
    delete m_pShoutcastManager;
#endif

    // LoopRecordingManager depends on config, engine
    qDebug() << "delete LoopRecordingManager " << qTime.elapsed();
    delete m_pLoopRecordingManager;
    
    // EngineMaster depends on Config
    qDebug() << "delete m_pEngine " << qTime.elapsed();
    delete m_pEngine;

    // HACK: Save config again. We saved it once before doing some dangerous
    // stuff. We only really want to save it here, but the first one was just
    // a precaution. The earlier one can be removed when stuff is more stable
    // at exit.
    m_pConfig->Save();

    delete m_pPrefDlg;

    qDebug() << "delete config " << qTime.elapsed();
    delete m_pConfig;

    PlayerInfo::destroy();

    // Check for leaked ControlObjects and give warnings.
    QList<ControlDoublePrivate*> leakedControls;
    QList<ConfigKey> leakedConfigKeys;

    ControlDoublePrivate::getControls(&leakedControls);

    if (leakedControls.size() > 0) {
        qDebug() << "WARNING: The following" << leakedControls.size() << "controls were leaked:";
        foreach (ControlDoublePrivate* pCOP, leakedControls) {
            ConfigKey key = pCOP->getKey();
            qDebug() << key.group << key.item << pCOP->getCreatorCO();
            leakedConfigKeys.append(key);
        }

       foreach (ConfigKey key, leakedConfigKeys) {
           // delete just to satisfy valgrind:
           // check if the pointer is still valid, the control object may have bin already
           // deleted by its parent in this loop
           ControlObject* pCo = ControlObject::getControl(key, false);
           if (pCo) {
               // it might happens that a control is deleted as child from an other control
               delete pCo;
           }
       }
   }
   qDebug() << "~MixxxApp: All leaking controls deleted.";

    delete m_pKeyboard;
    delete m_pKbdConfig;
    delete m_pKbdConfigEmpty;

    WaveformWidgetFactory::destroy();
    t.elapsed(true);
    // Report the total time we have been running.
    m_runtime_timer.elapsed(true);
    StatsManager::destroy();
}

void toggleVisibility(ConfigKey key, bool enable) {
    qDebug() << "Setting visibility for" << key.group << key.item << enable;
    ControlObject::set(key, enable ? 1.0 : 0.0);
}

void MixxxApp::slotViewShowSamplers(bool enable) {
    toggleVisibility(ConfigKey("[Samplers]", "show_samplers"), enable);
}

void MixxxApp::slotViewShowVinylControl(bool enable) {
    toggleVisibility(ConfigKey(VINYL_PREF_KEY, "show_vinylcontrol"), enable);
}

void MixxxApp::slotViewShowMicrophone(bool enable) {
    toggleVisibility(ConfigKey("[Microphone]", "show_microphone"), enable);
}

void MixxxApp::slotViewShowPreviewDeck(bool enable) {
    toggleVisibility(ConfigKey("[PreviewDeck]", "show_previewdeck"), enable);
}

void setVisibilityOptionState(QAction* pAction, ConfigKey key) {
    ControlObject* pVisibilityControl = ControlObject::getControl(key);
    pAction->setEnabled(pVisibilityControl != NULL);
    pAction->setChecked(pVisibilityControl != NULL ? pVisibilityControl->get() > 0.0 : false);
}

void MixxxApp::onNewSkinLoaded() {
    setVisibilityOptionState(m_pViewVinylControl,
                             ConfigKey(VINYL_PREF_KEY, "show_vinylcontrol"));
    setVisibilityOptionState(m_pViewShowSamplers,
                             ConfigKey("[Samplers]", "show_samplers"));
    setVisibilityOptionState(m_pViewShowMicrophone,
                             ConfigKey("[Microphone]", "show_microphone"));
    setVisibilityOptionState(m_pViewShowPreviewDeck,
                             ConfigKey("[PreviewDeck]", "show_previewdeck"));
}

int MixxxApp::noSoundDlg(void)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("Sound Device Busy"));
    msgBox.setText(
        "<html>" +
        tr("Mixxx was unable to access all the configured sound devices. "
        "Another application is using a sound device Mixxx is configured to "
        "use or a device is not plugged in.") +
        "<ul>"
            "<li>" +
                tr("<b>Retry</b> after closing the other application "
                "or reconnecting a sound device") +
            "</li>"
            "<li>" +
                tr("<b>Reconfigure</b> Mixxx's sound device settings.") +
            "</li>"
            "<li>" +
                tr("Get <b>Help</b> from the Mixxx Wiki.") +
            "</li>"
            "<li>" +
                tr("<b>Exit</b> Mixxx.") +
            "</li>"
        "</ul></html>"
    );

    QPushButton *retryButton = msgBox.addButton(tr("Retry"),
        QMessageBox::ActionRole);
    QPushButton *reconfigureButton = msgBox.addButton(tr("Reconfigure"),
        QMessageBox::ActionRole);
    QPushButton *wikiButton = msgBox.addButton(tr("Help"),
        QMessageBox::ActionRole);
    QPushButton *exitButton = msgBox.addButton(tr("Exit"),
        QMessageBox::ActionRole);

    while (true)
    {
        msgBox.exec();

        if (msgBox.clickedButton() == retryButton) {
            m_pSoundManager->queryDevices();
            return 0;
        } else if (msgBox.clickedButton() == wikiButton) {
            QDesktopServices::openUrl(QUrl(
                "http://mixxx.org/wiki/doku.php/troubleshooting"
                "#no_or_too_few_sound_cards_appear_in_the_preferences_dialog")
            );
            wikiButton->setEnabled(false);
        } else if (msgBox.clickedButton() == reconfigureButton) {
            msgBox.hide();
            m_pSoundManager->queryDevices();

            // This way of opening the dialog allows us to use it synchronously
            m_pPrefDlg->setWindowModality(Qt::ApplicationModal);
            m_pPrefDlg->exec();
            if (m_pPrefDlg->result() == QDialog::Accepted) {
                m_pSoundManager->queryDevices();
                return 0;
            }

            msgBox.show();

        } else if (msgBox.clickedButton() == exitButton) {
            return 1;
        }
    }
}

int MixxxApp::noOutputDlg(bool *continueClicked)
{
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setWindowTitle(tr("No Output Devices"));
    msgBox.setText( "<html>" + tr("Mixxx was configured without any output sound devices. "
                    "Audio processing will be disabled without a configured output device.") +
                    "<ul>"
                        "<li>" +
                            tr("<b>Continue</b> without any outputs.") +
                        "</li>"
                        "<li>" +
                            tr("<b>Reconfigure</b> Mixxx's sound device settings.") +
                        "</li>"
                        "<li>" +
                            tr("<b>Exit</b> Mixxx.") +
                        "</li>"
                    "</ul></html>"
    );

    QPushButton *continueButton = msgBox.addButton(tr("Continue"), QMessageBox::ActionRole);
    QPushButton *reconfigureButton = msgBox.addButton(tr("Reconfigure"), QMessageBox::ActionRole);
    QPushButton *exitButton = msgBox.addButton(tr("Exit"), QMessageBox::ActionRole);

    while (true)
    {
        msgBox.exec();

        if (msgBox.clickedButton() == continueButton) {
            *continueClicked = true;
            return 0;
        } else if (msgBox.clickedButton() == reconfigureButton) {
            msgBox.hide();
            m_pSoundManager->queryDevices();

            // This way of opening the dialog allows us to use it synchronously
            m_pPrefDlg->setWindowModality(Qt::ApplicationModal);
            m_pPrefDlg->exec();
            if ( m_pPrefDlg->result() == QDialog::Accepted) {
                m_pSoundManager->queryDevices();
                return 0;
            }

            msgBox.show();

        } else if (msgBox.clickedButton() == exitButton) {
            return 1;
        }
    }
}

QString buildWhatsThis(const QString& title, const QString& text) {
    QString preparedTitle = title;
    return QString("%1\n\n%2").arg(preparedTitle.replace("&", ""), text);
}

// initializes all QActions of the application
void MixxxApp::initActions()
{
    QString loadTrackText = tr("Load Track to Deck %1");
    QString loadTrackStatusText = tr("Loads a track in deck %1");
    QString openText = tr("Open");

    QString player1LoadStatusText = loadTrackStatusText.arg(QString::number(1));
    m_pFileLoadSongPlayer1 = new QAction(loadTrackText.arg(QString::number(1)), this);
    m_pFileLoadSongPlayer1->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "FileMenu_LoadDeck1"),
                                                  tr("Ctrl+o"))));
    m_pFileLoadSongPlayer1->setShortcutContext(Qt::ApplicationShortcut);
    m_pFileLoadSongPlayer1->setStatusTip(player1LoadStatusText);
    m_pFileLoadSongPlayer1->setWhatsThis(
        buildWhatsThis(openText, player1LoadStatusText));
    connect(m_pFileLoadSongPlayer1, SIGNAL(triggered()),
            this, SLOT(slotFileLoadSongPlayer1()));

    QString player2LoadStatusText = loadTrackStatusText.arg(QString::number(2));
    m_pFileLoadSongPlayer2 = new QAction(loadTrackText.arg(QString::number(2)), this);
    m_pFileLoadSongPlayer2->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "FileMenu_LoadDeck2"),
                                                  tr("Ctrl+Shift+O"))));
    m_pFileLoadSongPlayer2->setShortcutContext(Qt::ApplicationShortcut);
    m_pFileLoadSongPlayer2->setStatusTip(player2LoadStatusText);
    m_pFileLoadSongPlayer2->setWhatsThis(
        buildWhatsThis(openText, player2LoadStatusText));
    connect(m_pFileLoadSongPlayer2, SIGNAL(triggered()),
            this, SLOT(slotFileLoadSongPlayer2()));

    QString quitTitle = tr("&Exit");
    QString quitText = tr("Quits Mixxx");
    m_pFileQuit = new QAction(quitTitle, this);
    m_pFileQuit->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]", "FileMenu_Quit"),
                                                  tr("Ctrl+q"))));
    m_pFileQuit->setShortcutContext(Qt::ApplicationShortcut);
    m_pFileQuit->setStatusTip(quitText);
    m_pFileQuit->setWhatsThis(buildWhatsThis(quitTitle, quitText));
    connect(m_pFileQuit, SIGNAL(triggered()), this, SLOT(slotFileQuit()));

    QString rescanTitle = tr("&Rescan Library");
    QString rescanText = tr("Rescans library folders for changes to tracks.");
    m_pLibraryRescan = new QAction(rescanTitle, this);
    m_pLibraryRescan->setStatusTip(rescanText);
    m_pLibraryRescan->setWhatsThis(buildWhatsThis(rescanTitle, rescanText));
    m_pLibraryRescan->setCheckable(false);
    connect(m_pLibraryRescan, SIGNAL(triggered()),
            this, SLOT(slotScanLibrary()));

    QString createPlaylistTitle = tr("Add &New Playlist");
    QString createPlaylistText = tr("Create a new playlist");
    m_pPlaylistsNew = new QAction(createPlaylistTitle, this);
    m_pPlaylistsNew->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "LibraryMenu_NewPlaylist"),
                                                  tr("Ctrl+n"))));
    m_pPlaylistsNew->setShortcutContext(Qt::ApplicationShortcut);
    m_pPlaylistsNew->setStatusTip(createPlaylistText);
    m_pPlaylistsNew->setWhatsThis(buildWhatsThis(createPlaylistTitle, createPlaylistText));
    connect(m_pPlaylistsNew, SIGNAL(triggered()),
            m_pLibrary, SLOT(slotCreatePlaylist()));

    QString createCrateTitle = tr("Add New &Crate");
    QString createCrateText = tr("Create a new crate");
    m_pCratesNew = new QAction(createCrateTitle, this);
    m_pCratesNew->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "LibraryMenu_NewCrate"),
                                                  tr("Ctrl+Shift+N"))));
    m_pCratesNew->setShortcutContext(Qt::ApplicationShortcut);
    m_pCratesNew->setStatusTip(createCrateText);
    m_pCratesNew->setWhatsThis(buildWhatsThis(createCrateTitle, createCrateText));
    connect(m_pCratesNew, SIGNAL(triggered()),
            m_pLibrary, SLOT(slotCreateCrate()));

    QString fullScreenTitle = tr("&Full Screen");
    QString fullScreenText = tr("Display Mixxx using the full screen");
    m_pViewFullScreen = new QAction(fullScreenTitle, this);
#ifdef __APPLE__
    QString fullscreen_key = tr("Ctrl+Shift+F");
#else
    QString fullscreen_key = tr("F11");
#endif
    m_pViewFullScreen->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "ViewMenu_Fullscreen"),
                                                  fullscreen_key)));
    m_pViewFullScreen->setShortcutContext(Qt::ApplicationShortcut);
    // QShortcut * shortcut = new QShortcut(QKeySequence(tr("Esc")),  this);
    // connect(shortcut, SIGNAL(triggered()), this, SLOT(slotQuitFullScreen()));
    m_pViewFullScreen->setCheckable(true);
    m_pViewFullScreen->setChecked(false);
    m_pViewFullScreen->setStatusTip(fullScreenText);
    m_pViewFullScreen->setWhatsThis(buildWhatsThis(fullScreenTitle, fullScreenText));
    connect(m_pViewFullScreen, SIGNAL(toggled(bool)),
            this, SLOT(slotViewFullScreen(bool)));

    QString keyboardShortcutTitle = tr("Enable &Keyboard Shortcuts");
    QString keyboardShortcutText = tr("Toggles keyboard shortcuts on or off");
    bool keyboardShortcutsEnabled = m_pConfig->getValueString(
        ConfigKey("[Keyboard]", "Enabled")) == "1";
    m_pOptionsKeyboard = new QAction(keyboardShortcutTitle, this);
    m_pOptionsKeyboard->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_EnableShortcuts"),
                                                  tr("Ctrl+`"))));
    m_pOptionsKeyboard->setShortcutContext(Qt::ApplicationShortcut);
    m_pOptionsKeyboard->setCheckable(true);
    m_pOptionsKeyboard->setChecked(keyboardShortcutsEnabled);
    m_pOptionsKeyboard->setStatusTip(keyboardShortcutText);
    m_pOptionsKeyboard->setWhatsThis(buildWhatsThis(keyboardShortcutTitle, keyboardShortcutText));
    connect(m_pOptionsKeyboard, SIGNAL(toggled(bool)),
            this, SLOT(slotOptionsKeyboard(bool)));

    QString preferencesTitle = tr("&Preferences");
    QString preferencesText = tr("Change Mixxx settings (e.g. playback, MIDI, controls)");
    m_pOptionsPreferences = new QAction(preferencesTitle, this);
    m_pOptionsPreferences->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_Preferences"),
                                                  tr("Ctrl+P"))));
    m_pOptionsPreferences->setShortcutContext(Qt::ApplicationShortcut);
    m_pOptionsPreferences->setStatusTip(preferencesText);
    m_pOptionsPreferences->setWhatsThis(buildWhatsThis(preferencesTitle, preferencesText));
    connect(m_pOptionsPreferences, SIGNAL(triggered()),
            this, SLOT(slotOptionsPreferences()));

    QString aboutTitle = tr("&About");
    QString aboutText = tr("About the application");
    m_pHelpAboutApp = new QAction(aboutTitle, this);
    m_pHelpAboutApp->setStatusTip(aboutText);
    m_pHelpAboutApp->setWhatsThis(buildWhatsThis(aboutTitle, aboutText));
    connect(m_pHelpAboutApp, SIGNAL(triggered()),
            this, SLOT(slotHelpAbout()));

    QString supportTitle = tr("&Community Support");
    QString supportText = tr("Get help with Mixxx");
    m_pHelpSupport = new QAction(supportTitle, this);
    m_pHelpSupport->setStatusTip(supportText);
    m_pHelpSupport->setWhatsThis(buildWhatsThis(supportTitle, supportText));
    connect(m_pHelpSupport, SIGNAL(triggered()), this, SLOT(slotHelpSupport()));

    QString manualTitle = tr("&User Manual");
    QString manualText = tr("Read the Mixxx user manual.");
    m_pHelpManual = new QAction(manualTitle, this);
    m_pHelpManual->setStatusTip(manualText);
    m_pHelpManual->setWhatsThis(buildWhatsThis(manualTitle, manualText));
    connect(m_pHelpManual, SIGNAL(triggered()), this, SLOT(slotHelpManual()));

    QString feedbackTitle = tr("Send Us &Feedback");
    QString feedbackText = tr("Send feedback to the Mixxx team.");
    m_pHelpFeedback = new QAction(feedbackTitle, this);
    m_pHelpFeedback->setStatusTip(feedbackText);
    m_pHelpFeedback->setWhatsThis(buildWhatsThis(feedbackTitle, feedbackText));
    connect(m_pHelpFeedback, SIGNAL(triggered()), this, SLOT(slotHelpFeedback()));

    QString translateTitle = tr("&Translate This Application");
    QString translateText = tr("Help translate this application into your language.");
    m_pHelpTranslation = new QAction(translateTitle, this);
    m_pHelpTranslation->setStatusTip(translateText);
    m_pHelpTranslation->setWhatsThis(buildWhatsThis(translateTitle, translateText));
    connect(m_pHelpTranslation, SIGNAL(triggered()), this, SLOT(slotHelpTranslation()));

#ifdef __VINYLCONTROL__
    QString vinylControlText = tr("Use timecoded vinyls on external turntables to control Mixxx");
    QString vinylControlTitle1 = tr("Enable Vinyl Control &1");
    QString vinylControlTitle2 = tr("Enable Vinyl Control &2");

    m_pOptionsVinylControl = new QAction(vinylControlTitle1, this);
    m_pOptionsVinylControl->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_EnableVinyl1"),
                                                  tr("Ctrl+y"))));
    m_pOptionsVinylControl->setShortcutContext(Qt::ApplicationShortcut);
    // Either check or uncheck the vinyl control menu item depending on what
    // it was saved as.
    m_pOptionsVinylControl->setCheckable(true);
    m_pOptionsVinylControl->setChecked(false);
    m_pOptionsVinylControl->setStatusTip(vinylControlText);
    m_pOptionsVinylControl->setWhatsThis(buildWhatsThis(vinylControlTitle1, vinylControlText));
    connect(m_pOptionsVinylControl, SIGNAL(toggled(bool)), this,
            SLOT(slotCheckboxVinylControl(bool)));
    m_pVinylcontrol1Enabled = new ControlObjectThread(
            "[Channel1]", "vinylcontrol_enabled");
    connect(m_pVinylcontrol1Enabled, SIGNAL(valueChanged(double)), this,
            SLOT(slotControlVinylControl(double)));

    m_pOptionsVinylControl2 = new QAction(vinylControlTitle2, this);
    m_pOptionsVinylControl2->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_EnableVinyl2"),
                                                  tr("Ctrl+U"))));
    m_pOptionsVinylControl2->setShortcutContext(Qt::ApplicationShortcut);
    m_pOptionsVinylControl2->setCheckable(true);
    m_pOptionsVinylControl2->setChecked(false);
    m_pOptionsVinylControl2->setStatusTip(vinylControlText);
    m_pOptionsVinylControl2->setWhatsThis(buildWhatsThis(vinylControlTitle2, vinylControlText));
    connect(m_pOptionsVinylControl2, SIGNAL(toggled(bool)), this,
            SLOT(slotCheckboxVinylControl2(bool)));

    m_pVinylcontrol2Enabled = new ControlObjectThread(
            "[Channel2]", "vinylcontrol_enabled");
    connect(m_pVinylcontrol2Enabled, SIGNAL(valueChanged(double)), this,
            SLOT(slotControlVinylControl2(double)));
#endif

#ifdef __SHOUTCAST__
    QString shoutcastTitle = tr("Enable Live &Broadcasting");
    QString shoutcastText = tr("Stream your mixes to a shoutcast or icecast server");
    m_pOptionsShoutcast = new QAction(shoutcastTitle, this);
    m_pOptionsShoutcast->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_EnableLiveBroadcasting"),
                                                  tr("Ctrl+L"))));
    m_pOptionsShoutcast->setShortcutContext(Qt::ApplicationShortcut);
    m_pOptionsShoutcast->setCheckable(true);
    m_pOptionsShoutcast->setChecked(m_pShoutcastManager->isEnabled());
    m_pOptionsShoutcast->setStatusTip(shoutcastText);
    m_pOptionsShoutcast->setWhatsThis(buildWhatsThis(shoutcastTitle, shoutcastText));

    connect(m_pOptionsShoutcast, SIGNAL(triggered(bool)),
            m_pShoutcastManager, SLOT(setEnabled(bool)));
#endif

    QString mayNotBeSupported = tr("May not be supported on all skins.");
    QString showSamplersTitle = tr("Show Samplers");
    QString showSamplersText = tr("Show the sample deck section of the Mixxx interface.") +
            " " + mayNotBeSupported;
    m_pViewShowSamplers = new QAction(showSamplersTitle, this);
    m_pViewShowSamplers->setCheckable(true);
    m_pViewShowSamplers->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "ViewMenu_ShowSamplers"),
                                                  tr("Ctrl+1", "Menubar|View|Show Samplers"))));
    m_pViewShowSamplers->setStatusTip(showSamplersText);
    m_pViewShowSamplers->setWhatsThis(buildWhatsThis(showSamplersTitle, showSamplersText));
    connect(m_pViewShowSamplers, SIGNAL(toggled(bool)),
            this, SLOT(slotViewShowSamplers(bool)));

    QString showVinylControlTitle = tr("Show Vinyl Control Section");
    QString showVinylControlText = tr("Show the vinyl control section of the Mixxx interface.") +
            " " + mayNotBeSupported;
    m_pViewVinylControl = new QAction(showVinylControlTitle, this);
    m_pViewVinylControl->setCheckable(true);
    m_pViewVinylControl->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(
            ConfigKey("[KeyboardShortcuts]", "ViewMenu_ShowVinylControl"),
            tr("Ctrl+3", "Menubar|View|Show Vinyl Control Section"))));
    m_pViewVinylControl->setStatusTip(showVinylControlText);
    m_pViewVinylControl->setWhatsThis(buildWhatsThis(showVinylControlTitle, showVinylControlText));
    connect(m_pViewVinylControl, SIGNAL(toggled(bool)),
            this, SLOT(slotViewShowVinylControl(bool)));

    QString showMicrophoneTitle = tr("Show Microphone Section");
    QString showMicrophoneText = tr("Show the microphone section of the Mixxx interface.") +
            " " + mayNotBeSupported;
    m_pViewShowMicrophone = new QAction(showMicrophoneTitle, this);
    m_pViewShowMicrophone->setCheckable(true);
    m_pViewShowMicrophone->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(
            ConfigKey("[KeyboardShortcuts]", "ViewMenu_ShowMicrophone"),
            tr("Ctrl+2", "Menubar|View|Show Microphone Section"))));
    m_pViewShowMicrophone->setStatusTip(showMicrophoneText);
    m_pViewShowMicrophone->setWhatsThis(buildWhatsThis(showMicrophoneTitle, showMicrophoneText));
    connect(m_pViewShowMicrophone, SIGNAL(toggled(bool)),
            this, SLOT(slotViewShowMicrophone(bool)));

    QString showPreviewDeckTitle = tr("Show Preview Deck");
    QString showPreviewDeckText = tr("Show the preview deck in the Mixxx interface.") +
    " " + mayNotBeSupported;
    m_pViewShowPreviewDeck = new QAction(showPreviewDeckTitle, this);
    m_pViewShowPreviewDeck->setCheckable(true);
    m_pViewShowPreviewDeck->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "ViewMenu_ShowPreviewDeck"),
                                                  tr("Ctrl+4", "Menubar|View|Show Preview Deck"))));
    m_pViewShowPreviewDeck->setStatusTip(showPreviewDeckText);
    m_pViewShowPreviewDeck->setWhatsThis(buildWhatsThis(showPreviewDeckTitle, showPreviewDeckText));
    connect(m_pViewShowPreviewDeck, SIGNAL(toggled(bool)),
            this, SLOT(slotViewShowPreviewDeck(bool)));


    QString recordTitle = tr("&Record Mix");
    QString recordText = tr("Record your mix to a file");
    m_pOptionsRecord = new QAction(recordTitle, this);
    m_pOptionsRecord->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_RecordMix"),
                                                  tr("Ctrl+R"))));
    m_pOptionsRecord->setShortcutContext(Qt::ApplicationShortcut);
    m_pOptionsRecord->setCheckable(true);
    m_pOptionsRecord->setStatusTip(recordText);
    m_pOptionsRecord->setWhatsThis(buildWhatsThis(recordTitle, recordText));
    connect(m_pOptionsRecord, SIGNAL(toggled(bool)),
            m_pRecordingManager, SLOT(slotSetRecording(bool)));

    QString reloadSkinTitle = tr("&Reload Skin");
    QString reloadSkinText = tr("Reload the skin");
    m_pDeveloperReloadSkin = new QAction(reloadSkinTitle, this);
    m_pDeveloperReloadSkin->setShortcut(
        QKeySequence(m_pKbdConfig->getValueString(ConfigKey("[KeyboardShortcuts]",
                                                  "OptionsMenu_ReloadSkin"),
                                                  tr("Ctrl+Shift+R"))));
    m_pDeveloperReloadSkin->setShortcutContext(Qt::ApplicationShortcut);
    m_pDeveloperReloadSkin->setCheckable(true);
    m_pDeveloperReloadSkin->setChecked(false);
    m_pDeveloperReloadSkin->setStatusTip(reloadSkinText);
    m_pDeveloperReloadSkin->setWhatsThis(buildWhatsThis(reloadSkinTitle, reloadSkinText));
    connect(m_pDeveloperReloadSkin, SIGNAL(toggled(bool)),
            this, SLOT(slotDeveloperReloadSkin(bool)));
}

void MixxxApp::initMenuBar()
{
    // MENUBAR
    m_pFileMenu = new QMenu(tr("&File"), menuBar());
    m_pOptionsMenu = new QMenu(tr("&Options"), menuBar());
    m_pLibraryMenu = new QMenu(tr("&Library"),menuBar());
    m_pViewMenu = new QMenu(tr("&View"), menuBar());
    m_pHelpMenu = new QMenu(tr("&Help"), menuBar());
    m_pDeveloperMenu = new QMenu(tr("Developer"), menuBar());
    connect(m_pOptionsMenu, SIGNAL(aboutToShow()),
            this, SLOT(slotOptionsMenuShow()));
    // menuBar entry fileMenu
    m_pFileMenu->addAction(m_pFileLoadSongPlayer1);
    m_pFileMenu->addAction(m_pFileLoadSongPlayer2);
    m_pFileMenu->addSeparator();
    m_pFileMenu->addAction(m_pFileQuit);

    // menuBar entry optionsMenu
    //optionsMenu->setCheckable(true);
#ifdef __VINYLCONTROL__
    m_pVinylControlMenu = new QMenu(tr("&Vinyl Control"), menuBar());
    m_pVinylControlMenu->addAction(m_pOptionsVinylControl);
    m_pVinylControlMenu->addAction(m_pOptionsVinylControl2);

    m_pOptionsMenu->addMenu(m_pVinylControlMenu);
    m_pOptionsMenu->addSeparator();
#endif

    m_pOptionsMenu->addAction(m_pOptionsRecord);
#ifdef __SHOUTCAST__
    m_pOptionsMenu->addAction(m_pOptionsShoutcast);
#endif
    m_pOptionsMenu->addSeparator();
    m_pOptionsMenu->addAction(m_pOptionsKeyboard);
    m_pOptionsMenu->addSeparator();
    m_pOptionsMenu->addAction(m_pOptionsPreferences);

    m_pLibraryMenu->addAction(m_pLibraryRescan);
    m_pLibraryMenu->addSeparator();
    m_pLibraryMenu->addAction(m_pPlaylistsNew);
    m_pLibraryMenu->addAction(m_pCratesNew);

    // menuBar entry viewMenu
    //viewMenu->setCheckable(true);
    m_pViewMenu->addAction(m_pViewShowSamplers);
    m_pViewMenu->addAction(m_pViewShowMicrophone);
    m_pViewMenu->addAction(m_pViewVinylControl);
    m_pViewMenu->addAction(m_pViewShowPreviewDeck);
    m_pViewMenu->addSeparator();
    m_pViewMenu->addAction(m_pViewFullScreen);

    // Developer Menu
    m_pDeveloperMenu->addAction(m_pDeveloperReloadSkin);

    // menuBar entry helpMenu
    m_pHelpMenu->addAction(m_pHelpSupport);
    m_pHelpMenu->addAction(m_pHelpManual);
    m_pHelpMenu->addAction(m_pHelpFeedback);
    m_pHelpMenu->addAction(m_pHelpTranslation);
    m_pHelpMenu->addSeparator();
    m_pHelpMenu->addAction(m_pHelpAboutApp);

    menuBar()->addMenu(m_pFileMenu);
    menuBar()->addMenu(m_pLibraryMenu);
    menuBar()->addMenu(m_pViewMenu);
    menuBar()->addMenu(m_pOptionsMenu);

    if (m_cmdLineArgs.getDeveloper()) {
        menuBar()->addMenu(m_pDeveloperMenu);
    }

    menuBar()->addSeparator();
    menuBar()->addMenu(m_pHelpMenu);
}

void MixxxApp::slotFileLoadSongPlayer(int deck) {
    QString group = m_pPlayerManager->groupForDeck(deck-1);

    QString loadTrackText = tr("Load track to Deck %1").arg(QString::number(deck));
    QString deckWarningMessage = tr("Deck %1 is currently playing a track.")
            .arg(QString::number(deck));
    QString areYouSure = tr("Are you sure you want to load a new track?");

    if (ControlObject::get(ConfigKey(group, "play")) > 0.0) {
        int ret = QMessageBox::warning(this, tr("Mixxx"),
            deckWarningMessage + "\n" + areYouSure,
            QMessageBox::Yes | QMessageBox::No,
            QMessageBox::No);

        if (ret != QMessageBox::Yes)
            return;
    }

    QString s =
        QFileDialog::getOpenFileName(
            this,
            loadTrackText,
            m_pConfig->getValueString(ConfigKey("[Playlist]", "Directory")),
            QString("Audio (%1)")
                .arg(SoundSourceProxy::supportedFileExtensionsString()));

    if (!s.isNull()) {
        m_pPlayerManager->slotLoadToDeck(s, deck);
    }
}

void MixxxApp::slotFileLoadSongPlayer1() {
    slotFileLoadSongPlayer(1);
}

void MixxxApp::slotFileLoadSongPlayer2() {
    slotFileLoadSongPlayer(2);
}

void MixxxApp::slotFileQuit()
{
    if (!confirmExit()) {
        return;
    }
    hide();
    qApp->quit();
}

void MixxxApp::slotOptionsKeyboard(bool toggle) {
    if (toggle) {
        //qDebug() << "Enable keyboard shortcuts/mappings";
        m_pKeyboard->setKeyboardConfig(m_pKbdConfig);
        m_pConfig->set(ConfigKey("[Keyboard]","Enabled"), ConfigValue(1));
    } else {
        //qDebug() << "Disable keyboard shortcuts/mappings";
        m_pKeyboard->setKeyboardConfig(m_pKbdConfigEmpty);
        m_pConfig->set(ConfigKey("[Keyboard]","Enabled"), ConfigValue(0));
    }
}

void MixxxApp::slotDeveloperReloadSkin(bool toggle) {
    Q_UNUSED(toggle);
    rebootMixxxView();
}

void MixxxApp::slotViewFullScreen(bool toggle)
{
    if (m_pViewFullScreen)
        m_pViewFullScreen->setChecked(toggle);

    if (isFullScreen() == toggle) {
        return;
    }

    if (toggle) {
#if defined(__LINUX__) || defined(__APPLE__)
         // this and the later move(m_winpos) doesn't seem necessary
         // here on kwin, if it's necessary with some other x11 wm, re-enable
         // it, I guess -bkgood
         //m_winpos = pos();
         // fix some x11 silliness -- for some reason the move(m_winpos)
         // is moving the currentWindow to (0, 0), not the frame (as it's
         // supposed to, I might add)
         // if this messes stuff up on your distro yell at me -bkgood
         //m_winpos.setX(m_winpos.x() + (geometry().x() - x()));
         //m_winpos.setY(m_winpos.y() + (geometry().y() - y()));
#endif
        showFullScreen();
#ifdef __LINUX__
        // Fix for "No menu bar with ubuntu unity in full screen mode" Bug #885890
        // Not for Mac OS because the native menu bar will unhide when moving
        // the mouse to the top of screen

        //menuBar()->setNativeMenuBar(false);
        // ^ This leaves a broken native Menu Bar with Ubuntu Unity Bug #1076789#
        // it is only allowed to change this prior initMenuBar()

        m_NativeMenuBarSupport = menuBar()->isNativeMenuBar();
        if (m_NativeMenuBarSupport) {
            setMenuBar(new QMenuBar(this));
            menuBar()->setNativeMenuBar(false);
            initMenuBar();
        }
#endif
    } else {
#ifdef __LINUX__
        if (m_NativeMenuBarSupport) {
            setMenuBar(new QMenuBar(this));
            menuBar()->setNativeMenuBar(m_NativeMenuBarSupport);
            initMenuBar();
        }
        //move(m_winpos);
#endif
        showNormal();
    }
}

void MixxxApp::slotOptionsPreferences()
{
    m_pPrefDlg->setHidden(false);
    m_pPrefDlg->activateWindow();
}

void MixxxApp::slotControlVinylControl(double toggle)
{
#ifdef __VINYLCONTROL__
    if (m_pPlayerManager->hasVinylInput(0)) {
        m_pOptionsVinylControl->setChecked((bool)toggle);
    } else {
        m_pOptionsVinylControl->setChecked(false);
        if (toggle) {
            QMessageBox::warning(this, tr("Mixxx"),
                tr("No input device(s) select.\nPlease select your soundcard(s) "
                    "in the sound hardware preferences."),
                QMessageBox::Ok,
                QMessageBox::Ok);
            m_pPrefDlg->show();
            m_pPrefDlg->showSoundHardwarePage();
            ControlObject::set(ConfigKey(
                    "[Channel1]", "vinylcontrol_status"), (double)VINYL_STATUS_DISABLED);
            m_pVinylcontrol1Enabled->set(0.0);
        }
    }
#endif
}

void MixxxApp::slotCheckboxVinylControl(bool toggle)
{
#ifdef __VINYLCONTROL__
    m_pVinylcontrol1Enabled->set((double)toggle);
#endif
}

void MixxxApp::slotControlVinylControl2(double toggle)
{
#ifdef __VINYLCONTROL__
    if (m_pPlayerManager->hasVinylInput(1)) {
        m_pOptionsVinylControl2->setChecked((bool)toggle);
    } else {
        m_pOptionsVinylControl2->setChecked(false);
        if (toggle) {
            QMessageBox::warning(this, tr("Mixxx"),
                tr("No input device(s) select.\nPlease select your soundcard(s) "
                    "in the sound hardware preferences."),
                QMessageBox::Ok,
                QMessageBox::Ok);
            m_pPrefDlg->show();
            m_pPrefDlg->showSoundHardwarePage();
            ControlObject::set(ConfigKey(
                    "[Channel2]", "vinylcontrol_status"), (double)VINYL_STATUS_DISABLED);
            m_pVinylcontrol2Enabled->set(0.0);
          }
    }
#endif
}

void MixxxApp::slotCheckboxVinylControl2(bool toggle)
{
#ifdef __VINYLCONTROL__
    m_pVinylcontrol2Enabled->set((double)toggle);
#endif
}

void MixxxApp::slotHelpAbout() {
    DlgAbout *about = new DlgAbout(this);
    about->show();
}

void MixxxApp::slotHelpSupport() {
    QUrl qSupportURL;
    qSupportURL.setUrl(MIXXX_SUPPORT_URL);
    QDesktopServices::openUrl(qSupportURL);
}

void MixxxApp::slotHelpFeedback() {
    QUrl qFeedbackUrl;
    qFeedbackUrl.setUrl(MIXXX_FEEDBACK_URL);
    QDesktopServices::openUrl(qFeedbackUrl);
}

void MixxxApp::slotHelpTranslation() {
    QUrl qTranslationUrl;
    qTranslationUrl.setUrl(MIXXX_TRANSLATION_URL);
    QDesktopServices::openUrl(qTranslationUrl);
}

void MixxxApp::slotHelpManual() {
    QDir resourceDir(m_pConfig->getResourcePath());
    // Default to the mixxx.org hosted version of the manual.
    QUrl qManualUrl(MIXXX_MANUAL_URL);
#if defined(__APPLE__)
    // We don't include the PDF manual in the bundle on OSX. Default to the
    // web-hosted version.
#elif defined(__WINDOWS__)
    // On Windows, the manual PDF sits in the same folder as the 'skins' folder.
    if (resourceDir.exists(MIXXX_MANUAL_FILENAME)) {
        qManualUrl = QUrl::fromLocalFile(
                resourceDir.absoluteFilePath(MIXXX_MANUAL_FILENAME));
    }
#elif defined(__LINUX__)
    // On GNU/Linux, the manual is installed to e.g. /usr/share/mixxx/doc/
    resourceDir.cd("doc");
    if (resourceDir.exists(MIXXX_MANUAL_FILENAME)) {
        qManualUrl = QUrl::fromLocalFile(
                resourceDir.absoluteFilePath(MIXXX_MANUAL_FILENAME));
    }
#else
    // No idea, default to the mixxx.org hosted version.
#endif
    QDesktopServices::openUrl(qManualUrl);
}

void MixxxApp::setToolTipsCfg(int tt) {
    m_pConfig->set(ConfigKey("[Controls]","Tooltips"),
                   ConfigValue(tt));
    m_toolTipsCfg = tt;
}

void MixxxApp::rebootMixxxView() {

    if (!m_pWidgetParent || !m_pView)
        return;

    qDebug() << "Now in rebootMixxxView...";

    QPoint initPosition = pos();
    QSize initSize = size();

    m_pView->hide();

    WaveformWidgetFactory::instance()->stop();
    WaveformWidgetFactory::instance()->destroyWidgets();

    // Workaround for changing skins while fullscreen, just go out of fullscreen
    // mode. If you change skins while in fullscreen (on Linux, at least) the
    // window returns to 0,0 but and the backdrop disappears so it looks as if
    // it is not fullscreen, but acts as if it is.
    bool wasFullScreen = m_pViewFullScreen->isChecked();
    slotViewFullScreen(false);

    //delete the view cause swaping central widget do not remove the old one !
    if (m_pView) {
        delete m_pView;
    }
    m_pView = new QFrame();

    // assignment in next line intentional
    if (!(m_pWidgetParent = m_pSkinLoader->loadDefaultSkin(m_pView,
                                                           m_pKeyboard,
                                                           m_pPlayerManager,
                                                           m_pControllerManager,
                                                           m_pLibrary,
                                                           m_pLoopRecordingManager,
                                                           m_pVCManager))) {

        QMessageBox::critical(this,
                              tr("Error in skin file"),
                              tr("The selected skin cannot be loaded."));
    }
    else {
        // keep gui centered (esp for fullscreen)
        m_pView->setLayout( new QHBoxLayout(m_pView));
        m_pView->layout()->setContentsMargins(0,0,0,0);
        m_pView->layout()->addWidget(m_pWidgetParent);

        // don't move this before loadDefaultSkin above. bug 521509 --bkgood
        // NOTE: (vrince) I don't know this comment is relevant now ...
        setCentralWidget(m_pView);
        update();
        adjustSize();
        //qDebug() << "view size" << m_pView->size() << size();
    }

    if( wasFullScreen) {
        slotViewFullScreen(true);
    } else {
        move(initPosition.x() + (initSize.width() - m_pView->width()) / 2,
             initPosition.y() + (initSize.height() - m_pView->height()) / 2);
    }

    WaveformWidgetFactory::instance()->start();

#ifdef __APPLE__
    // Original the following line fixes issue on OSX where menu bar went away
    // after a skin change. It was original surrounded by #if __OSX__
    // Now it seems it causes the opposite see Bug #1000187
    //menuBar()->setNativeMenuBar(m_NativeMenuBarSupport);
#endif

    qDebug() << "rebootMixxxView DONE";
    emit(newSkinLoaded());
}

/** Event filter to block certain events. For example, this function is used
  * to disable tooltips if the user specifies in the preferences that they
  * want them off. This is a callback function.
  */
bool MixxxApp::eventFilter(QObject *obj, QEvent *event)
{
    if (event->type() == QEvent::ToolTip) {
        // return true for no tool tips
        if (m_toolTipsCfg == 2) {
            // ON (only in Library)
            WWidget* pWidget = dynamic_cast<WWidget*>(obj);
            WWaveformViewer* pWfViewer = dynamic_cast<WWaveformViewer*>(obj);
            WSpinny* pSpinny = dynamic_cast<WSpinny*>(obj);
            QLabel* pLabel = dynamic_cast<QLabel*>(obj);
            return (pWidget || pWfViewer || pSpinny || pLabel);
        } else if (m_toolTipsCfg == 1) {
            // ON
            return false;
        } else {
            // OFF
            return true;
        }
    } else {
        // standard event processing
        return QObject::eventFilter(obj, event);
    }
}

void MixxxApp::closeEvent(QCloseEvent *event) {
    if (!confirmExit()) {
        event->ignore();
    }
}

void MixxxApp::slotScanLibrary() {
    m_pLibraryRescan->setEnabled(false);
    m_pLibraryScanner->scan(
        m_pConfig->getValueString(ConfigKey("[Playlist]", "Directory")),this);
}

void MixxxApp::slotEnableRescanLibraryAction() {
    m_pLibraryRescan->setEnabled(true);
}

void MixxxApp::slotOptionsMenuShow() {
    // Check recording if it is active.
    m_pOptionsRecord->setChecked(m_pRecordingManager->isRecordingActive());
#ifdef __SHOUTCAST__
    m_pOptionsShoutcast->setChecked(m_pShoutcastManager->isEnabled());
#endif
}

void MixxxApp::slotToCenterOfPrimaryScreen() {
    if (!m_pView)
        return;

    QDesktopWidget* desktop = QApplication::desktop();
    int primaryScreen = desktop->primaryScreen();
    QRect primaryScreenRect = desktop->availableGeometry(primaryScreen);

    move(primaryScreenRect.left() + (primaryScreenRect.width() - m_pView->width()) / 2,
         primaryScreenRect.top() + (primaryScreenRect.height() - m_pView->height()) / 2);
}

void MixxxApp::checkDirectRendering() {
    // IF
    //  * A waveform viewer exists
    // AND
    //  * The waveform viewer is an OpenGL waveform viewer
    // AND
    //  * The waveform viewer does not have direct rendering enabled.
    // THEN
    //  * Warn user

    WaveformWidgetFactory* factory = WaveformWidgetFactory::instance();
    if (!factory)
        return;

    if (!factory->isOpenGLAvailable() &&
        m_pConfig->getValueString(ConfigKey("[Direct Rendering]", "Warned")) != QString("yes")) {
        QMessageBox::warning(
            0, tr("OpenGL Direct Rendering"),
            tr("Direct rendering is not enabled on your machine.<br><br>"
               "This means that the waveform displays will be very<br>"
               "<b>slow and may tax your CPU heavily</b>. Either update your<br>"
               "configuration to enable direct rendering, or disable<br>"
               "the waveform displays in the Mixxx preferences by selecting<br>"
               "\"Empty\" as the waveform display in the 'Interface' section.<br><br>"
               "NOTE: If you use NVIDIA hardware,<br>"
               "direct rendering may not be present, but you should<br>"
               "not experience degraded performance."));
        m_pConfig->set(ConfigKey("[Direct Rendering]", "Warned"), QString("yes"));
    }
}

bool MixxxApp::confirmExit() {
    bool playing(false);
    bool playingSampler(false);
    unsigned int deckCount = m_pPlayerManager->numDecks();
    unsigned int samplerCount = m_pPlayerManager->numSamplers();
    for (unsigned int i = 0; i < deckCount; ++i) {
        if (ControlObject::get(
                ConfigKey(PlayerManager::groupForDeck(i), "play"))) {
            playing = true;
            break;
        }
    }
    for (unsigned int i = 0; i < samplerCount; ++i) {
        if (ControlObject::get(
                ConfigKey(PlayerManager::groupForSampler(i), "play"))) {
            playingSampler = true;
            break;
        }
    }
    if (playing) {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            tr("Confirm Exit"),
            tr("A deck is currently playing. Exit Mixxx?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn == QMessageBox::No) {
            return false;
        }
    } else if (playingSampler) {
        QMessageBox::StandardButton btn = QMessageBox::question(this,
            tr("Confirm Exit"),
            tr("A sampler is currently playing. Exit Mixxx?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn == QMessageBox::No) {
            return false;
        }
    }
    if (m_pPrefDlg->isVisible()) {
        QMessageBox::StandardButton btn = QMessageBox::question(
            this, tr("Confirm Exit"),
            tr("The preferences window is still open.") + "<br>" +
            tr("Discard any changes and exit Mixxx?"),
            QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
        if (btn == QMessageBox::No) {
            return false;
        }
        else {
            m_pPrefDlg->close();
        }
    }
    return true;
}
