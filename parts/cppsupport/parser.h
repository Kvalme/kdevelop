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

#ifndef PARSER_H
#define PARSER_H

#include <qstring.h>
#include <qstringlist.h>
#include <qdom.h>

class ProblemReporter;

namespace CppSupport{

class Driver;
class Lexer;
class Token;
class Error;

class Parser{
public:
    Parser( ProblemReporter* pr, Driver* drv, Lexer* lexer );
    virtual ~Parser();

    QString fileName() const;
    void setFileName( const QString& fileName );


    int errors() const;

    virtual void resetErrors();
    virtual bool reportError( const Error& err );
    /*TODO: remove*/ virtual bool reportError( const QString& msg );
    /*TODO: remove*/ virtual void syntaxError();
    /*TODO: remove*/ virtual void parseError();

    void dump();

public /*rules*/ :

    bool parseTranslationUnit();
    bool parseDefinition();
    bool parseBlockDeclaration();
    bool parseLinkageSpecification();
    bool parseLinkageBody();
    bool parseNamespace();
    bool parseNamespaceAliasDefinition();
    bool parseUsing();
    bool parseUsingDirective();
    bool parseTypedef();
    bool parseAsmDefinition();
    bool parseTemplateDeclaration();
    bool parseDeclaration2();
	bool parseDeclaration();


    bool parseNestedNameSpecifier();
    bool parseUnqualiedName();
    bool parseStringLiteral();
    bool parseName();
    bool parseOperatorFunctionId();
    bool parseTemplateArgumentList();
    bool parseOperator();
    bool parseCvQualify( QStringList& cv );
    bool parseSimpleTypeSpecifier();
    bool parsePtrOperator();
    bool parseTemplateArgument();
    bool parseTypeSpecifier();
    bool parseDeclarator();
    bool parseTemplateParameterList();
    bool parseTemplateParameter();
    bool parseStorageClassSpecifier();
    bool parseFunctionSpecifier();
    bool parseConstantExpression();
    bool parseInitDeclaratorList();
    bool parseInitDeclarator();
    bool parseFunctionBody();
    bool parseParameterDeclarationClause();
    bool parseCtorInitializer();
    bool parsePtrToMember();
    bool parseEnumSpecifier();
    bool parseClassSpecifier();
    bool parseElaboratedTypeSpecifier();
    bool parseDeclaratorId();
    bool parseExceptionSpecification();
    bool parseCommaExpression();
    bool parseEnumeratorList();
    bool parseEnumerator();
    bool parseTypeParameter();
    bool parseParameterDeclaration();
    bool parseTypeId();
    bool parseAbstractDeclarator();
    bool parseParameterDeclarationList();
    bool parseAssignmentExpression();
    bool parseMemberSpecificationList();
    bool parseMemberSpecification();
    bool parseAccessSpecifier();
    bool parseBaseClause();
    bool parseTypeIdList();
    bool parseMemInitializerList();
    bool parseMemInitializer();
    bool parseInitializer();
    bool parseBaseSpecifierList();
    bool parseBaseSpecifier();
    bool parseInitializerClause();
    bool parseMemInitializerId();

    // expression
    bool parseExpression();

    // statement
    bool parseCondition();
    bool parseStatement();
    bool parseExpressionStatement();
    bool parseWhileStatement();
    bool parseDoStatement();
    bool parseForStatement();
    bool parseCompoundStatement();
    bool parseForInitStatement();
    bool parseIfStatement();
    bool parseSwitchStatement();
    bool parseLabeledStatement();
    bool parseDeclarationStatement();

    bool skipUntil( int token );
    bool skipUntilDeclaration();
    bool skip( int l, int r );

private:
    ProblemReporter* m_problemReporter;
    Driver* driver;
    Lexer* lex;
    QDomDocument* dom;
    QDomElement translationUnit;
    QString m_fileName;
    int m_errors;
    int m_maxErrors;
};

inline QString Parser::fileName() const
{
    return m_fileName;
}

inline int Parser::errors() const
{
    return m_errors;
}

inline void Parser::resetErrors()
{
    m_errors = 0;
}


}

#endif
