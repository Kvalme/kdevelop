/***************************************************************************
 *   Copyright (C) 2002 by Roberto Raggi                                   *
 *   raggi@cli.di.unipi.it                                                 *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include <strstream>

#include "backgroundparser.h"
#include "problemreporter.h"
#include "JavaLexer.hpp"
#include "JavaRecognizer.hpp"
#include "JavaStoreWalker.hpp"
#include "JavaAST.hpp"
#include <kdebug.h>


BackgroundParser::BackgroundParser( ProblemReporter* reporter,
                                    const QString& source,
                                    const QString& filename )
    : m_reporter( reporter ),
      m_source( source.unicode(), source.length() ),
      m_fileName( filename.unicode(), filename.length() )
{
}

BackgroundParser::~BackgroundParser()
{
}

void BackgroundParser::run()
{
    QCString _fn = m_fileName.utf8();
    std::string fn( _fn.data() );

    QCString text = m_source.utf8();
    std::istrstream stream( text );

    JavaLexer lexer( stream );
    lexer.setFilename( fn );
    lexer.setProblemReporter( m_reporter );

    JavaRecognizer parser( lexer );
    parser.setFilename( fn );
    parser.setProblemReporter( m_reporter );

    try{
        lexer.resetErrors();
        parser.resetErrors();

        parser.compilationUnit();
        int errors = lexer.numberOfErrors() + parser.numberOfErrors();

    } catch( antlr::ANTLRException& ex ){
        kdDebug() << "*exception*: " << ex.toString().c_str() << endl;
        m_reporter->reportError( ex.what(),
				 m_fileName,
				 lexer.getLine(),
				 lexer.getColumn() );
    }

    kdDebug(9013) << "FINISHED!!" << endl;
}



