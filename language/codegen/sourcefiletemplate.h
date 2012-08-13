/*
 * This file is part of KDevelop
 * Copyright 2012 Miha Čančula <miha@noughmad.eu>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef KDEVELOP_SOURCEFILETEMPLATE_H
#define KDEVELOP_SOURCEFILETEMPLATE_H

#include <QString>
#include <QList>
#include <QVariant>

#include "../languageexport.h"

class KArchiveDirectory;

namespace KDevelop
{

class TemplateRenderer;

/**
 * Represents a source file template archive
 *
 * @section TemplateStructure Template Archive Structure
 *
 * Source file templates in KDevPlatform are archive files.
 * The archive must contain at least one .desktop file with the template's description.
 * If multiple such files are present, the one with the same base name as the archive itself will be used.
 *
 * The description file must contain a @c [General] section with the following keys:
 * @li @c Name - The user-visible name of this template
 * @li @c Comment - A short user-visible comment
 * @li @c Category - The category of this template. It can be nested, in which case levels are separated
 * with forward slashes. The top-level category is usually the language.
 * @li @c Files - List of files generated by this template. These are not actual file names, but names
 * of config groups describing those files.
 * @li @c OptionsFile (optional) - If the template uses custom configuration options, specify a path to
 * the options file here. @ref CustomOptions
 *
 * For each file name in the @c Files array, TemplateClassGenerator expects a section with the same name.
 * this section should contain three keys:
 * @li @c Name - User-visible name for this file. This will be show the the user in the dialog and can be translated.
 * @li @c File - The input file name in the template archive. The template for this file will be read from here.
 * @li @c OutputFile - The suggested output file name. This will be renderer as a template, so it can contain variables.
 *
 * An example template description is below. It shows all features described above.
 *
 * @code
 * [General]
 * Name=Example
 * Comment=Example description for a C++ Class
 * Category=C++
 * Options=options.kcfg
 * Files=Header,Implementation
 *
 * [Header]
 * Name=Header
 * File=class.h
 * OutputFile={{ name }}.h
 *
 * [Implementation]
 * Name=Implementation
 * File=class.cpp
 * OutputFile={{ name }}.cpp
 * @endcode
 *
 * @section CustomOptions
 *
 * Templates can expose additional configurations options.
 * This is done through a file with the same syntax and .kcfg files used by KConfig XT.
 * The name of this file is specified with the @c OptionsFile key in the [General] section of the description file.
 *
 * @note
 * The options are not parsed by TemplateClassGenerator.
 * Instead, hasCustomOptions() returns true if the template specifies a custom options file,
 * and customOptions() returns the full text of that file.
 * The parsing is done by TemplateOptionsPage.
 *
 * The file can (and should) provide a type, name, label and default value for each configuration option.
 * So far, only variables with types String, Int and Bool are recognized.
 * Label is the user-visible string that will be shown next to the input field.
 * The default value will be rendered as a template, so it can contain variables.
 *
 * After the user configures the options, they will be available to the template as context variables.
 * The variable name will match the option name, and its value will be the one set by the user.
 *
 * An example options.kcfg file for a class template is included below.
 *
 * @code
 * <?xml version="1.0" encoding="UTF-8"?>
 * <kcfg xmlns="http://www.kde.org/standards/kcfg/1.0"
 *      xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"
 *      xsi:schemaLocation="http://www.kde.org/standards/kcfg/1.0
 *      http://www.kde.org/standards/kcfg/1.0/kcfg.xsd">
 *  <kcfgfile arg="true"/>
 *  <group name="Private Class">
 *    <entry name="private_class_name" type="String">
 *      <label>Private class name</label>
 *      <default>{{ name }}Private</default>
 *    </entry>
 *    <entry name="private_member_name" type="String">
 *      <label>Private member name</label>
 *      <default>d</default>
 *    </entry>
 *  </group>
 * </kcfg>
 * @endcode
 *
 * In this example, if the class name is Example, the default value for the private class name will be ExamplePrivate.
 * After the user accepts the option values, the template will access them through the @c private_class_name
 * and @c private_member_name variables.
 *
 * For more information regarding the XML file format, refer to the KConfig XT documentation.
 */
class KDEVPLATFORMLANGUAGE_EXPORT SourceFileTemplate
{
public:
    /**
     * Describes a single output file in this template
     */
    struct OutputFile
    {
        /**
         * A unique identifier, equal to the list element in the @c OutputFiles entry
         */
        QString identifier;
        /**
         * The name of the input file within the archive, equal to the @c File entry
         */
        QString fileName;
        /**
         * User-visible label, equal to the @c Name entry in this file's group in the template description file
         */
        QString label;
        /**
         * The default output file name, equal to the @c OutputFile entry
         *
         * @note The output name can contain template variables, so make sure to pass it through
         * TemplateRenderer::render before using it or showing it to the user.
         */
        QString outputName;
    };

    /**
     * Describes one configuration option
     */
    struct ConfigOption
    {
        /**
         * The type of this option.
         *
         * Currently supported are Int, String and Bool
         */
        QString type;
        /**
         * A unique identifier for this option
         */
        QString name;
        /**
         * User-visible label
         */
        QString label;
        /**
         * A context description for the option, shown to the user as a tooltip
         */
        QString context;

        /**
         * The default value of this option
         */
        QVariant value;

        /**
         * The maximum value of this entry, as a string
         *
         * This is applicable only to integers
         */
        QString maxValue;
        /**
         * The minimum value of this entry, as a string
         *
         * This is applicable only to integers
         */
        QString minValue;
    };

    /**
     * Creates a SourceFileTemplate representing the template archive with
     * description file @p templateDescription.
     *
     * @param templateDescription template description file, used to find the
     *        archive and read information
     */
    SourceFileTemplate(const QString& templateDescription);

    /**
     * Copy constructor
     *
     * Creates a SourceFileTemplate representing the same template archive as @p other.
     * This new objects shares no data with the @p other, so they can be read and deleted independently.
     *
     * @param other the template to copy
     */
    SourceFileTemplate(const SourceFileTemplate& other);

    /**
     * Creates an invalid SourceFileTemplate
     */
    SourceFileTemplate();

    /**
     * Destroys this SourceFileTemplate
     */
    ~SourceFileTemplate();

    SourceFileTemplate& operator=(const SourceFileTemplate& other);

    void setTemplateDescription(const QString& templateDescription);

    /**
     * Returns true if this SourceFileTemplate represents an actual template archive, and false otherwise
     */
    bool isValid() const;

    /**
     * The name of this template, corresponds to the @c Name entry in the description file
     */
    QString name() const;

    /**
     * The top-level directory in the template archive
     *
     * @sa KArchive::directory()
     */
    const KArchiveDirectory* directory() const;

    /**
     * The list of all output files in this template
     */
    QList<OutputFile> outputFiles() const;

    /**
     * @return true if the template uses any custom options, false otherwise
     **/
    bool hasCustomOptions() const;

    /**
     * Return the custom options this template exposes
     **/
    QHash<QString, QList<ConfigOption> > customOptions(TemplateRenderer* renderer) const;

    /**
     * The type of this template, which is the first category specified in the template description file.
     * This can be any string, but TemplateClassAssistant only supports @c Class and @c Test. 
     */
    QString type() const;

    /**
     * The name of the programming language of the generated files
     */
    QString languageName() const;

private:
    class SourceFileTemplatePrivate* const d;
};
}

#endif // KDEVELOP_SOURCEFILETEMPLATE_H
