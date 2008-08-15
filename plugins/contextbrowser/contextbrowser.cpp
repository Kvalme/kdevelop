/*
  * This file is part of KDevelop
 *
 * Copyright 2007 David Nolden <david.nolden.kdevelop@art-master.de>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

#include "contextbrowser.h"

#include <QTimer>
#include <QApplication>
#include <qalgorithms.h>
#include <klocale.h>
#include <kaction.h>
#include <kpluginfactory.h>
#include <kpluginloader.h>
#include <ktexteditor/view.h>
#include <ktexteditor/document.h>
#include <ktexteditor/smartinterface.h>
#include <KTextEditor/TextHintInterface>
#include <kactioncollection.h>
#include <kdebug.h>
#include <interfaces/icore.h>
#include <interfaces/idocumentcontroller.h>
#include <interfaces/ilanguage.h>
#include <interfaces/iuicontroller.h>
#include <interfaces/ilanguagecontroller.h>
#include <language/duchain/duchainlock.h>
#include <language/duchain/duchain.h>
#include <language/duchain/ducontext.h>
#include <language/duchain/declaration.h>
#include <language/duchain/use.h>
#include <language/duchain/duchainutils.h>
#include <language/duchain/functiondefinition.h>
#include <language/interfaces/ilanguagesupport.h>
#include <language/backgroundparser/backgroundparser.h>
#include <language/backgroundparser/parsejob.h>
#include "contextbrowserview.h"
#include <language/duchain/uses.h>

const unsigned int highlightingTimeout = 150;

class ContextBrowserViewFactory: public KDevelop::IToolViewFactory
{
public:
    ContextBrowserViewFactory(ContextBrowserPlugin *plugin): m_plugin(plugin) {}

    virtual QWidget* create(QWidget */*parent*/ = 0)
    {
        return new ContextBrowserView(m_plugin);
    }

    virtual Qt::DockWidgetArea defaultPosition()
    {
        return Qt::LeftDockWidgetArea;
    }

    virtual QString id() const
    {
        return "org.kdevelop.ContextBrowser";
    }

private:
    ContextBrowserPlugin *m_plugin;
};

K_PLUGIN_FACTORY(ContextBrowserFactory, registerPlugin<ContextBrowserPlugin>(); )
K_EXPORT_PLUGIN(ContextBrowserFactory("kdevcontextbrowser"))

ContextBrowserPlugin::ContextBrowserPlugin(QObject *parent, const QVariantList&)
    : KDevelop::IPlugin(ContextBrowserFactory::componentData(), parent)
    , m_viewFactory(new ContextBrowserViewFactory(this))
{
  setXMLFile( "kdevcontextbrowser.rc" );

  KActionCollection* actions = actionCollection();

  core()->uiController()->addToolView(i18n("Context Browser"), m_viewFactory);

  connect( core()->documentController(), SIGNAL( textDocumentCreated( KDevelop::IDocument* ) ), this, SLOT( textDocumentCreated( KDevelop::IDocument* ) ) );
  connect( core()->documentController(), SIGNAL( documentClosed( KDevelop::IDocument* ) ), this, SLOT( documentClosed( KDevelop::IDocument* ) ) );
  connect( core()->languageController()->backgroundParser(), SIGNAL(parseJobFinished(KDevelop::ParseJob*)), this, SLOT(parseJobFinished(KDevelop::ParseJob*)));

  connect( DUChain::self(), SIGNAL( declarationSelected(DeclarationPointer) ), this, SLOT( declarationSelectedInUI(DeclarationPointer) ) );


  m_updateTimer = new QTimer(this);
  m_updateTimer->setSingleShot(true);
  connect( m_updateTimer, SIGNAL( timeout() ), this, SLOT( updateViews() ) );

  QAction* previousContext = actions->addAction("previous_context");
  previousContext->setText( i18n("&Previous Context") );
  previousContext->setShortcut( Qt::META | Qt::Key_Left );
  connect(previousContext, SIGNAL(triggered(bool)), this, SIGNAL(previousContextShortcut()));

  QAction* nextContext = actions->addAction("next_context");
  nextContext->setText( i18n("&Next Context") );
  nextContext->setShortcut( Qt::META | Qt::Key_Right );
  connect(nextContext, SIGNAL(triggered(bool)), this, SIGNAL(nextContextShortcut()));

  QAction* previousUse = actions->addAction("previous_use");
  previousUse->setText( i18n("&Previous Use") );
  previousUse->setShortcut( Qt::META | Qt::SHIFT |  Qt::Key_Left );
  connect(previousUse, SIGNAL(triggered(bool)), this, SLOT(previousUseShortcut()));

  QAction* nextUse = actions->addAction("next_use");
  nextUse->setText( i18n("&Next Use") );
  nextUse->setShortcut( Qt::META | Qt::SHIFT | Qt::Key_Right );
  connect(nextUse, SIGNAL(triggered(bool)), this, SLOT(nextUseShortcut()));
}

ContextBrowserPlugin::~ContextBrowserPlugin()
{
  foreach (KTextEditor::SmartRange* range, m_watchedRanges)
    range->removeWatcher(this);
}

void ContextBrowserPlugin::watchRange(KTextEditor::SmartRange* range)
{
  if (range->watchers().contains( this ))
    return;

  range->addWatcher(this);
  m_watchedRanges.insert(range);
}

void ContextBrowserPlugin::ignoreRange(KTextEditor::SmartRange* range)
{
  if (!range->watchers().contains( this ))
    return;

  range->removeWatcher(this);
  m_watchedRanges.remove(range);
}

void ContextBrowserPlugin::unload()
{
  core()->uiController()->removeToolView(m_viewFactory);
}

void ContextBrowserPlugin::rangeDeleted( KTextEditor::SmartRange *range ) {
  m_backups.remove( range );
  m_watchedRanges.remove(range);

  for(QMap<KTextEditor::View*, KTextEditor::SmartRange*>::iterator it = m_highlightedRange.begin(); it != m_highlightedRange.end(); ++it)
    if(*it == range) {
      m_highlightedRange.erase(it);
      return;
    }
}

///@todo this doesn't work, we use TextHintInterface instead atm.
void ContextBrowserPlugin::mouseEnteredRange( KTextEditor::SmartRange* range, KTextEditor::View* view ) {
  m_mouseHoverCursor = SimpleCursor(range->start());
  m_mouseHoverDocument = view->document()->url();
  m_updateViews << view;
  m_updateTimer->start(1);
}

void ContextBrowserPlugin::mouseExitedRange( KTextEditor::SmartRange* /*range*/, KTextEditor::View* view ) {
  clearMouseHover();
  m_updateViews << view;
  m_updateTimer->start(1);
}

void ContextBrowserPlugin::textHintRequested(const KTextEditor::Cursor& cursor, QString&) {
  m_mouseHoverCursor = SimpleCursor(cursor);
  KTextEditor::View* view = dynamic_cast<KTextEditor::View*>(sender());
  if(!view) {
    kWarning() << "could not cast to view";
  }else{
    m_mouseHoverDocument = view->document()->url();
    m_updateViews << view;
  }
  m_updateTimer->start(1);
}

void ContextBrowserPlugin::clearMouseHover() {
  m_mouseHoverCursor = SimpleCursor::invalid();
  m_mouseHoverDocument = KUrl();
}


KTextEditor::Attribute::Ptr highlightedUseAttribute(bool /*mouseHighlight*/) {
  static KTextEditor::Attribute::Ptr standardAttribute = KTextEditor::Attribute::Ptr();
  if( !standardAttribute ) {
    standardAttribute = KTextEditor::Attribute::Ptr( new KTextEditor::Attribute() );
    standardAttribute->setBackgroundFillWhitespace(true);
    standardAttribute->setBackground(Qt::yellow);//QApplication::palette().toolTipBase());
  }
  return standardAttribute;
}

KTextEditor::Attribute::Ptr highlightedDeclarationAttribute() {
  static KTextEditor::Attribute::Ptr standardAttribute = KTextEditor::Attribute::Ptr();
  if( !standardAttribute ) {
    standardAttribute = KTextEditor::Attribute::Ptr( new KTextEditor::Attribute() );
    standardAttribute->setBackgroundFillWhitespace(true);
    standardAttribute->setBackground(Qt::red);
  }
  return standardAttribute;
}

KTextEditor::Attribute::Ptr highlightedSpecialObjectAttribute() {
  static KTextEditor::Attribute::Ptr standardAttribute = KTextEditor::Attribute::Ptr();
  if( !standardAttribute ) {
    standardAttribute = KTextEditor::Attribute::Ptr( new KTextEditor::Attribute() );
    standardAttribute->setBackgroundFillWhitespace(true);
    QColor color(Qt::yellow);
    color.setRed(90);
    standardAttribute->setBackground(color);
  }
  return standardAttribute;
}

void ContextBrowserPlugin::changeHighlight( KTextEditor::SmartRange* range, bool highlight, bool /*declaration*/, bool mouseHighlight ) {
  if( !range )
    return;

  KTextEditor::Attribute::Ptr attrib;
  if( highlight ) {
    if( !m_backups.contains(range) ) {
      m_backups[range] = range->attribute();
      watchRange(range);
    }
/*    if( declaration )
      attrib = highlightedDeclarationAttribute();
    else*/
      attrib = highlightedUseAttribute(mouseHighlight);
  }else{
    if( m_backups.contains(range) ) {
      attrib = m_backups[range];
      m_backups.remove(range);
    }
    ignoreRange(range);
  }

  range->setAttribute(attrib);
}

void ContextBrowserPlugin::changeHighlight( KTextEditor::View* view, KDevelop::Declaration* decl, bool highlight, bool mouseHighlight ) {
  if( !view || !decl || !decl->context() ) {
    kDebug() << "invalid view/declaration";
    return;
  }

  KTextEditor::SmartRange* range = decl->smartRange();
  if( range )
    changeHighlight( range, highlight, true, mouseHighlight );


  QList<KTextEditor::SmartRange*> uses;
  {
    KDevelop::DUChainReadLocker lock( DUChain::lock() );
    uses = decl->smartUses();
  }

  foreach(KTextEditor::SmartRange* range, uses)
    changeHighlight( range, highlight, false, mouseHighlight );

  if( FunctionDefinition* def = FunctionDefinition::definition(decl) )
    if( def->smartRange() )
      changeHighlight( def->smartRange(), highlight, false, mouseHighlight );
}

QWidget* masterWidget(QWidget* w) {
  while(w && w->parent() && qobject_cast<QWidget*>(w->parent()))
    w = qobject_cast<QWidget*>(w->parent());
  return w;
}

void ContextBrowserPlugin::updateViews()
{
  foreach( KTextEditor::View* view, m_updateViews ) {
    SimpleCursor c = SimpleCursor(view->cursorPosition());
    ///First: Check whether there is a special language object
    ///@todo Maybe make this optional, because it can be slow

    ///Find either a Declaration, or a use, that is in the Range.

    bool mouseHighlight = false;

    if( view->document()->url() == m_mouseHoverDocument && m_mouseHoverCursor.isValid() ) {
      c = m_mouseHoverCursor;
      mouseHighlight = true;
    }

    KTextEditor::SmartInterface* iface = dynamic_cast<KTextEditor::SmartInterface*>(view->document());

    bool foundSpecialObject = false;
    KDevelop::ILanguage* pickedLanguage = 0;

    if(iface) {
      //Delete old special highlighting
      if(m_highlightedRange.contains(view)) {
        QMutexLocker lock(iface ? iface->smartMutex() : 0);

        Q_ASSERT(m_highlightedRange[view]->document() == view->document());

        delete m_highlightedRange[view];
        m_highlightedRange.remove(view);
      }

      //Eventually find a special language object, and directly highlight it
      QList<KDevelop::ILanguage*> languages = ICore::self()->languageController()->languagesForUrl(view->document()->url());

      foreach( KDevelop::ILanguage* language, languages) {
        SimpleRange r = language->languageSupport()->specialLanguageObjectRange(view->document()->url(), c);
        if(r.isValid()) {
          //We've got our range. Highlight it and continue.

          m_highlightedRange[view] = iface->newSmartRange( r.textRange(), m_highlightedRange[view] );//iface->newSmartRange( view->document()->documentRange() );
          iface->addHighlightToDocument(m_highlightedRange[view]);
//           KTextEditor::SmartRange* highlightRange = iface->newSmartRange( r.textRange(), m_highlightedRange[view] );

          m_highlightedRange[view]->setAttribute( highlightedSpecialObjectAttribute() );
/*          m_highlightedRange[view]*/

          watchRange(m_highlightedRange[view]);

          foundSpecialObject = true;
          pickedLanguage = language;

          break;
        }
      }
    }

    KDevelop::DUChainReadLocker lock( DUChain::lock(), 100 );
    if(!lock.locked()) {
      kDebug() << "Failed to lock du-chain in time";
      continue;
    }

    TopDUContext* topContext = DUChainUtils::standardContextForUrl(view->document()->url());

    ///unhighlight the old uses
    if( m_highlightedDeclarations.contains(view) )
      changeHighlight( view, m_highlightedDeclarations[view].data(), false, mouseHighlight );

    Declaration* foundDeclaration = 0;

    if(!foundSpecialObject) {
      if(m_useDeclaration) {
        foundDeclaration = m_useDeclaration.data();
      }else{
        //If we haven't found a special language object, search for a use/declaration and eventually highlight it
//         kDebug() << "searching at cursor" << c.textCursor() << mouseHighlight << "document" << view->document()->url().pathOrUrl();
        foundDeclaration = DUChainUtils::declarationForDefinition( DUChainUtils::itemUnderCursor(view->document()->url(), c) );
      }
      if( foundDeclaration ) {
        m_highlightedDeclarations[view] = foundDeclaration;
        changeHighlight( view, foundDeclaration, true, mouseHighlight );
      }else{
//         kDebug() << "not found declaration";
        m_highlightedDeclarations.remove(view);
      }
    }

    ///Update the context widget, and maybe show a tooltip
    if(foundDeclaration || foundSpecialObject) {
      if(mouseHighlight || (view->isActiveView() && core()->documentController()->activeDocument() && core()->documentController()->activeDocument()->textDocument() == view->document())) {
        //Update the context browser, but only if this view is active or has been mouse-hovered
        bool foundVisibleContextView = false;
        foreach(ContextBrowserView* contextView, m_views) {
          if(masterWidget(contextView) == masterWidget(view)) {
            if(foundDeclaration)
              contextView->declarationWidget()->setDeclaration(foundDeclaration, topContext);
            else
              contextView->declarationWidget()->setSpecialNavigationWidget(pickedLanguage->languageSupport()->specialLanguageObjectNavigationWidget(view->document()->url(), c));

            if(contextView->isVisible())
              foundVisibleContextView = true;
          }
        }
        if(mouseHighlight && !foundVisibleContextView) {
          ///@todo show tooltip
        }
      }
    }

    ///Update the context information
    if(topContext && !mouseHighlight) {
      DUContext* ctx = topContext->findContextAt(c);
      while(ctx && (ctx->type() == DUContext::Template || ctx->type() == DUContext::Helper || ctx->localScopeIdentifier().isEmpty()) && ctx->parentContext())
        ctx = ctx->parentContext();
      if(ctx)
        foreach(ContextBrowserView* contextView, m_views)
          if(masterWidget(contextView) == masterWidget(view))
            contextView->contextWidget()->setContext(ctx, c);
    }

  }
  m_updateViews.clear();
  m_useDeclaration = DeclarationPointer();
}

void ContextBrowserPlugin::declarationSelectedInUI(DeclarationPointer decl)
{
  m_useDeclaration = decl;
m_updateTimer->start(highlightingTimeout);
}

void ContextBrowserPlugin::parseJobFinished(KDevelop::ParseJob* job)
{
  KDevelop::DUChainWriteLocker lock( DUChain::lock() );
  registerAsRangeWatcher(job->duChain());
}

void ContextBrowserPlugin::registerAsRangeWatcher(KDevelop::DUChainBase* base)
{
  if(base->smartRange())
    watchRange(base->smartRange());
}

void ContextBrowserPlugin::registerAsRangeWatcher(KDevelop::DUContext* ctx)
{
  if(!ctx)
    return;
  if(dynamic_cast<TopDUContext*>(ctx) && static_cast<TopDUContext*>(ctx)->flags() & TopDUContext::ProxyContextFlag && !ctx->importedParentContexts().isEmpty())
    return registerAsRangeWatcher(ctx->importedParentContexts()[0].context());

  foreach(Declaration* decl, ctx->localDeclarations())
    registerAsRangeWatcher(decl);

  for(int a = 0; a < ctx->usesCount(); ++a) {
    KTextEditor::SmartRange* range = ctx->useSmartRange(a);
    if(range)
      watchRange(range);
  }

  foreach(DUContext* child, ctx->childContexts())
    registerAsRangeWatcher(child);
}

void ContextBrowserPlugin::textDocumentCreated( KDevelop::IDocument* document )
{
  Q_ASSERT(document->textDocument());

  connect( document->textDocument(), SIGNAL(destroyed( QObject* )), this, SLOT( documentDestroyed( QObject* ) ) );
  connect( document->textDocument(), SIGNAL( viewCreated( KTextEditor::Document* , KTextEditor::View* ) ), this, SLOT( viewCreated( KTextEditor::Document*, KTextEditor::View* ) ) );

  foreach( KTextEditor::View* view, document->textDocument()->views() )
    viewCreated( document->textDocument(), view );

  KDevelop::DUChainWriteLocker lock( DUChain::lock() );
  QList<TopDUContext*> chains = DUChain::self()->chainsForDocument( document->url() );

  foreach( TopDUContext* chain, chains )
    registerAsRangeWatcher( chain );
}

void ContextBrowserPlugin::documentClosed( KDevelop::IDocument* /*document*/ )
{
}

void ContextBrowserPlugin::documentDestroyed( QObject* /*obj*/ )
{
}

void ContextBrowserPlugin::viewDestroyed( QObject* obj )
{
  m_highlightedDeclarations.remove(static_cast<KTextEditor::View*>(obj));
  m_updateViews.remove(static_cast<KTextEditor::View*>(obj));
}

void ContextBrowserPlugin::cursorPositionChanged( KTextEditor::View* view, const KTextEditor::Cursor& /*newPosition*/ )
{
  clearMouseHover();
  m_updateViews.insert(view);
  m_updateTimer->start(highlightingTimeout/2);
}

void ContextBrowserPlugin::viewCreated( KTextEditor::Document* , KTextEditor::View* v )
{
  disconnect( v, SIGNAL( cursorPositionChanged( KTextEditor::View*, const KTextEditor::Cursor& ) ), this, SLOT( cursorPositionChanged( KTextEditor::View*, const KTextEditor::Cursor& ) ) ); ///Just to make sure that multiple connections don't happen
  connect( v, SIGNAL( cursorPositionChanged( KTextEditor::View*, const KTextEditor::Cursor& ) ), this, SLOT( cursorPositionChanged( KTextEditor::View*, const KTextEditor::Cursor& ) ) );
  connect( v, SIGNAL(destroyed( QObject* )), this, SLOT( viewDestroyed( QObject* ) ) );

  KTextEditor::TextHintInterface *iface = dynamic_cast<KTextEditor::TextHintInterface*>(v);
  if( !iface )
      return;

  iface->enableTextHints(highlightingTimeout);

  connect(v, SIGNAL(needTextHint(const KTextEditor::Cursor&, QString&)), this, SLOT(textHintRequested(const KTextEditor::Cursor&, QString&)));
}

void ContextBrowserPlugin::registerToolView(ContextBrowserView* view)
{
  m_views << view;
}

void ContextBrowserPlugin::previousUseShortcut()
{
  switchUse(false);
}

void ContextBrowserPlugin::nextUseShortcut()
{
  switchUse(true);
}

void ContextBrowserPlugin::switchUse(bool forward)
{
  if(core()->documentController()->activeDocument() && core()->documentController()->activeDocument()->textDocument() && core()->documentController()->activeDocument()->textDocument()->activeView()) {
    KTextEditor::Document* doc = core()->documentController()->activeDocument()->textDocument();
    KDevelop::SimpleCursor c(doc->activeView()->cursorPosition());


    KDevelop::DUChainReadLocker lock( DUChain::lock() );
    KDevelop::TopDUContext* chosen = DUChainUtils::standardContextForUrl(doc->url());

    if( chosen )
    {
      DUContext* ctx = chosen->findContextAt(c);

      //Try finding a declaration under the cursor
      Declaration* decl = DUChainUtils::itemUnderCursor(doc->url(), c);
      if(decl) {
        KDevVarLengthArray<IndexedTopDUContext> usingFiles = DUChain::uses()->uses(decl->id());
        
        if(decl->topContext()->indexForUsedDeclaration(decl, false) != std::numeric_limits<int>::max() && usingFiles.indexOf(decl->topContext()) == -1)
          usingFiles.insert(decl->topContext(), 0);
        
        if(decl->range().contains(c) && decl->url() == chosen->url()) {
            //The cursor is directly on the declaration. Jump to the first or last use.
            if(!usingFiles.isEmpty()) {
            TopDUContext* top = (forward ? usingFiles[0] : usingFiles.back()).data();
            if(top) {
              QList<SimpleRange> useRanges = allUses(top, decl, true);
              qSort(useRanges);
              if(!useRanges.isEmpty()) {
                SimpleRange selectUse = forward ? useRanges.first() : useRanges.back();
                lock.unlock();
                core()->documentController()->openDocument(KUrl(top->url().str()), KTextEditor::Range(selectUse.start.textCursor(), selectUse.start.textCursor()));
              }
            }
          }
          return;
        }
        //Check whether we are within a use
        QList<SimpleRange> localUses = allUses(chosen, decl, true);
        qSort(localUses);
        for(int a = 0; a < localUses.size(); ++a) {
        if(localUses[a].contains(c)) {
          int nextUse = (forward ? a+1 : a-1);
          
          //Make sure we end up behind the use
          while(forward && nextUse < localUses.size() && (localUses[nextUse].start <= localUses[a].end || localUses[nextUse].isEmpty()))
            ++nextUse;
          
          //Make sure we end up before the use
          while(!forward && nextUse >= 0 && (localUses[nextUse].start >= localUses[a].start || localUses[nextUse].isEmpty()))
            --nextUse;
          //Jump to the next use
            
          kDebug() << "count of uses:" << localUses.size() << "nextUse" << nextUse;
          
          if(nextUse < 0 || nextUse == localUses.size()) {
            kDebug() << "jumping to next file";
            //Jump to the first use in the next using top-context
            int indexInFiles = usingFiles.indexOf(chosen);
            if(indexInFiles != -1) {
              
              int nextFile = (forward ? indexInFiles+1 : indexInFiles-1);
              kDebug() << "current file" << indexInFiles << "nextFile" << nextFile;
              
              if(nextFile < 0 || nextFile >= usingFiles.size()) {
                //Open the declaration
                KUrl u(decl->url().str());
                SimpleRange range = decl->range();
                range.end = range.start;
                lock.unlock();
                core()->documentController()->openDocument(u, range.textRange());
                return;
              }else{
                TopDUContext* nextTop = usingFiles[nextFile].data();
                
                KUrl u(nextTop->url().str());
                
                QList<SimpleRange> nextTopUses = allUses(nextTop, decl, true);
                qSort(nextTopUses);
                
                if(!nextTopUses.isEmpty()) {
                  SimpleRange range = forward ? nextTopUses.front() : nextTopUses.back();
                  range.end = range.start;
                  lock.unlock();
                  core()->documentController()->openDocument(u, range.textRange());
                }
                return;
              }
            }else{
              kDebug() << "not found own file in use list";
            }
          }else{
              KUrl url(chosen->url().str());
              SimpleRange range = localUses[nextUse];
              range.end = range.start;
              lock.unlock();
              core()->documentController()->openDocument(url, range.textRange());
              return;
          }
        }
        }
      }

      ctx = ctx->parentContext(); //It may happen, for example in the case of function-declarations, that the use is one context higher.
    }
  }
}

void ContextBrowserPlugin::unRegisterToolView(ContextBrowserView* view)
{
  m_views.removeAll(view);
}


#include "contextbrowser.moc"

// kate: space-indent on; indent-width 2; tab-width 4; replace-tabs on; auto-insert-doxygen on
