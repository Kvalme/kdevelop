#include <qlayout.h>
#include <qmultilineedit.h>
#include <qvbox.h>
#include <qcheckbox.h>


#include <kapplication.h>
#include <kstdaction.h>
#include <kdebug.h>
#include <kaction.h>
#include <klocale.h>
#include <kconfig.h>
#include <kstatusbar.h>
#include <kdialogbase.h>

#include "widgets/ktabzoomwidget.h"


#include "projectmanager.h"
#include "partcontroller.h"
#include "partselectwidget.h"
#include "api.h"
#include "core.h"
#include "settingswidget.h"
#include "statusbar.h"


#include "toplevel_sdi.h"


TopLevelSDI::TopLevelSDI(QWidget *parent, const char *name)
  : KParts::MainWindow(parent, name), m_closing(false)
{
}


void TopLevelSDI::init()
{
  setXMLFile("gideonui.rc");

  createFramework();
  createActions();
  createStatusBar();

  createGUI(0);
}


TopLevelSDI::~TopLevelSDI()
{
}


bool TopLevelSDI::queryClose()
{
  if (m_closing)
    return true;

  emit wantsToQuit();
  return false;
}


void TopLevelSDI::realClose()
{
  saveSettings();
  
  m_closing = true;
  close();
}


KMainWindow *TopLevelSDI::main()
{
  return this;
}


void TopLevelSDI::createStatusBar()
{
  (void) new StatusBar(this);
}


void TopLevelSDI::createFramework()
{
  m_leftBar = new KTabZoomWidget(this, KTabZoomPosition::Left);
  setCentralWidget(m_leftBar);

  m_bottomBar = new KTabZoomWidget(m_leftBar, KTabZoomPosition::Bottom);
  m_leftBar->addContent(m_bottomBar);

  m_tabWidget = new QTabWidget(m_bottomBar);
  m_tabWidget->setMargin(2);

  PartController::createInstance(m_tabWidget);

  m_bottomBar->addContent(m_tabWidget);

  connect(m_tabWidget, SIGNAL(currentChanged(QWidget*)), 
	  PartController::getInstance(), SLOT(slotCurrentChanged(QWidget*)));

  connect(PartController::getInstance(), SIGNAL(activePartChanged(KParts::Part*)),
	  this, SLOT(createGUI(KParts::Part*)));
}


void TopLevelSDI::createActions()
{
  ProjectManager::getInstance()->createActions( actionCollection() );
  
  KStdAction::quit(this, SLOT(slotQuit()), actionCollection());
    
  KAction* action;
  action = KStdAction::preferences(this, SLOT(slotSettings()),
                actionCollection(), "settings_configure" );
  action->setStatusText( i18n("Lets you customize KDevelop") );
}


void TopLevelSDI::slotQuit()
{
  (void) queryClose();
}


void TopLevelSDI::embedPartView(QWidget *view, const QString &name)
{
  m_tabWidget->addTab(view, name);
  m_tabWidget->showPage(view);
}


void TopLevelSDI::embedSelectView(QWidget *view, const QString &name)
{
  m_leftBar->addTab(view, name);
}


void TopLevelSDI::embedOutputView(QWidget *view, const QString &name)
{
  m_bottomBar->addTab(view, name);
}


void TopLevelSDI::removeView(QWidget *)
{
}


void TopLevelSDI::raiseView(QWidget *view)
{
  m_leftBar->raiseWidget(view);
  m_bottomBar->raiseWidget(view);
  m_tabWidget->showPage(view);
}


void TopLevelSDI::lowerView(QWidget *view)
{
  m_leftBar->lowerWidget(view);
  m_bottomBar->lowerWidget(view);
}


void TopLevelSDI::lowerAllViews()
{
  m_leftBar->lowerAllWidgets();
  m_bottomBar->lowerAllWidgets();
}


void TopLevelSDI::createGUI(KParts::Part *part)
{
  KParts::MainWindow::createGUI(part);
}


void TopLevelSDI::loadSettings()
{
  ProjectManager::getInstance()->loadSettings();
  applyMainWindowSettings(kapp->config(), "Mainwindow");
}


void TopLevelSDI::saveSettings()
{
  ProjectManager::getInstance()->saveSettings();
  saveMainWindowSettings(kapp->config(), "Mainwindow");
}


void TopLevelSDI::slotSettings()
{
  KDialogBase dlg(KDialogBase::TreeList, i18n("Customize KDevelop"),
                  KDialogBase::Ok|KDialogBase::Cancel, KDialogBase::Ok, this,
                  "customization dialog");

  QVBox *vbox = dlg.addVBoxPage(i18n("General"));
  SettingsWidget *gsw = new SettingsWidget(vbox, "general settings widget");
  
  KConfig* config = kapp->config();
  config->setGroup("General Options");
  gsw->lastProjectCheckbox->setChecked(config->readBoolEntry("Read Last Project On Startup",true));


  vbox = dlg.addVBoxPage(i18n("Plugins"));
  PartSelectWidget *w = new PartSelectWidget(vbox, "part selection widget");
  connect( &dlg, SIGNAL(okClicked()), w, SLOT(accept()) );

  Core::getInstance()->doEmitConfigWidget(&dlg);
  dlg.exec();

  config->setGroup("General Options");
  config->writeEntry("Read Last Project On Startup",gsw->lastProjectCheckbox->isChecked());
}



#include "toplevel_sdi.moc"
