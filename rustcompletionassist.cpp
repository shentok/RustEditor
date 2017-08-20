/*
 *  RustEditor: Plugin to add Rust language support to QtCreator IDE.
 *  Copyright (C) 2015  Davide Ghilardi
 *
 *  This file is part of RustEditor.
 *
 *  RustEditor is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published by
 *  the Free Software Foundation, either version 2.1 of the License, or
 *  (at your option) any later version.
 *
 *  RustEditor is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with RustEditor.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "rustcompletionassist.h"
#include "rusteditorconstants.h"
#include "rusteditorplugin.h"
#include "configuration.h"

#include <coreplugin/messagemanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/id.h>
#include <texteditor/completionsettings.h>
#include <texteditor/convenience.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/functionhintproposal.h>


#include <utils/faketooltip.h>

#include <QIcon>
#include <QPainter>
#include <QLabel>
#include <QToolButton>
#include <QHBoxLayout>
#include <QApplication>
#include <QDesktopWidget>
#include <QDebug>
#include <QProcess>
#include <QTemporaryFile>
#include <QTextDocument>

using namespace TextEditor;

using namespace RustEditor::Internal;


enum RacerOutputPosMeaning{
    MATCH,
    LINENUM,
    CHARNUM,
    FILEPATH,
    TYPE,
    DESCR
};

enum CompletionOrder {
    SpecialMemberOrder = -5
};

static bool isActivationChar(const QChar &ch)
{
    return ch == QLatin1Char('(') || ch == QLatin1Char('.') || ch == QLatin1Char(',');
}

static bool isIdentifierChar(QChar ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

static bool isDelimiter(QChar ch)
{
    switch (ch.unicode()) {
    case '{':
    case '}':
    case '[':
    case ']':
    case ')':
    case '?':
    case '!':
    case ':':
    case ';':
    case ',':
    case '+':
    case '-':
    case '*':
    case '/':
        return true;

    default:
        return false;
    }
}

static bool checkStartOfIdentifier(const QString &word)
{
    if (! word.isEmpty()) {
        const QChar ch = word.at(0);
        if (ch.isLetter() || ch == QLatin1Char('_'))
            return true;
    }

    return false;
}

// ----------------------------
// RustCompletionAssistProvider
// ----------------------------
bool RustCompletionAssistProvider::supportsEditor(Core::Id editorId) const
{
    return editorId == Constants::RUSTEDITOR_ID;
}

IAssistProcessor *RustCompletionAssistProvider::createProcessor() const
{
    return new RustCompletionAssistProcessor;
}

int RustCompletionAssistProvider::activationCharSequenceLength() const
{
    return 1;
}

bool RustCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    return isActivationChar(sequence.at(0));
}

// -----------------------------
// RustCompletionAssistProcessor
// -----------------------------
RustCompletionAssistProcessor::RustCompletionAssistProcessor()
    : m_keywordIcon(QLatin1String(":/rusteditor/images/keyword.png"))
    , m_varIcon(QLatin1String(":/rusteditor/images/var.png"))
    , m_functionIcon(QLatin1String(":/rusteditor/images/func.png"))
    , m_typeIcon(QLatin1String(":/rusteditor/images/type.png"))
    , m_constIcon(QLatin1String(":/rusteditor/images/const.png"))
    , m_attributeIcon(QLatin1String(":/rusteditor/images/attribute.png"))
    , m_uniformIcon(QLatin1String(":/rusteditor/images/uniform.png"))
    , m_varyingIcon(QLatin1String(":/rusteditor/images/varying.png"))
    , m_otherIcon(QLatin1String(":/rusteditor/images/other.png"))
{}

RustCompletionAssistProcessor::~RustCompletionAssistProcessor()
{}

static AssistProposalItem *createCompletionItem(const QString &text, const QIcon &icon, int order = 0)
{
    AssistProposalItem *item = new AssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    return item;
}

/**
 * @brief RustCompletionAssistProcessor::getRacerIcon Match completion type with the relative icon
 * @param type String representing the type of the completion returned by racer
 * @return icon that matches the given type
 */
const QIcon &RustCompletionAssistProcessor::getRacerIcon(const QString &type){
    if(type == QLatin1String("Module")) return m_otherIcon;
    else if(type == QLatin1String("Struct")) return m_typeIcon;
    else if(type == QLatin1String("MatchArm")) return m_otherIcon; //??
    else if(type == QLatin1String("Function")) return m_functionIcon;
    else if(type == QLatin1String("Crate")) return m_otherIcon; //
    else if(type == QLatin1String("Let")) return m_varIcon;
    else if(type == QLatin1String("IfLet")) return m_varIcon; //??
    else if(type == QLatin1String("StructField")) return m_attributeIcon;
    else if(type == QLatin1String("Impl")) return m_typeIcon;
    else if(type == QLatin1String("Enum")) return m_typeIcon;
    else if(type == QLatin1String("EnumVariant")) return m_typeIcon;
    else if(type == QLatin1String("Type")) return m_typeIcon;
    else if(type == QLatin1String("FnArg")) return m_varIcon;
    else if(type == QLatin1String("Trait")) return m_typeIcon;
    else if(type == QLatin1String("Const")) return m_constIcon;
    else if(type == QLatin1String("Static")) return m_constIcon;
    else return m_otherIcon;
}

IAssistProposal *RustCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    if (interface->reason() == IdleEditor && !acceptsIdleEditor(interface))
        return 0;

    const QTextDocument *doc = interface->textDocument();

    const QString result = runRacer("complete", *doc, interface->position());

    //Keep the compatibility with 3.x until 4.0 is out
#if (QTC_VERSION_MAJOR == 3) && (QTC_VERSION_MINOR == 6)
    QList<AssistProposalItem *> m_completions;
#elif (QTC_VERSION_MAJOR == 4) && (QTC_VERSION_MINOR >= 0)
    QList<AssistProposalItemInterface *> m_completions; // all possible completions at given point
#endif

    if (!result.isEmpty() == 0){
        QStringList lines = result.split(QLatin1String("\n"));
        foreach (QString l, lines) {
            if(l.left(5) == QString::fromLatin1("MATCH")){
                QStringList fields = l.split(QLatin1String(","));
                m_completions << createCompletionItem(fields[MATCH].mid(6),getRacerIcon(fields[TYPE]));
            }
        }
    }else{
        Core::MessageManager::write(result);
    }

    int pos = interface->position() - 1;
    while (isIdentifierChar(interface->characterAt(pos)))
        --pos;

    return new GenericProposal(pos + 1, m_completions);
}

bool RustCompletionAssistProcessor::acceptsIdleEditor(const AssistInterface *interface)
{
    const int cursorPosition = interface->position();
    const QChar ch = interface->characterAt(cursorPosition - 1);

    const QChar characterUnderCursor = interface->characterAt(cursorPosition);

    if (isIdentifierChar(ch) && (characterUnderCursor.isSpace() ||
                                 characterUnderCursor.isNull() ||
                                 isDelimiter(characterUnderCursor))) {
        int pos = interface->position() - 1;
        for (; pos != -1; --pos) {
            if (! isIdentifierChar(interface->characterAt(pos)))
                break;
        }
        ++pos;

        const QString word = interface->textAt(pos, cursorPosition - pos);
        if (word.length() > 2 && checkStartOfIdentifier(word)) {
            for (int i = 0; i < word.length(); ++i) {
                if (! isIdentifierChar(word.at(i)))
                    return false;
            }
            return true;
        }
    }

    return isActivationChar(ch);
}

QString RustCompletionAssistProcessor::runRacer(const QString &command, const QTextDocument &doc, int position)
{
    //Save the current document to a temporary file so I can call racer on it
    QTemporaryFile tmpsrc;
    if (!tmpsrc.open())
        return QString();

    {
        const QString fullDoc = doc.toPlainText();
        QTextStream tmpstream(&tmpsrc);
        tmpstream << fullDoc;
    }

    int charnum, linenum;
    TextEditor::Convenience::convertPosition(&doc, position, &linenum, &charnum);

    const Settings &rustEditorSettings = Configuration::getSettingsPtr();

    QString rustPath = rustEditorSettings.rustSrcPath().toString();
    QString racerPath = rustEditorSettings.racerPath().toString();
    if (!racerPath.isEmpty() && !racerPath.endsWith(QLatin1Char('/')))
        racerPath.append(QLatin1Char('/'));

    //set environment variable pointing to where rust source is located
    QStringList env;
    env << (QLatin1String("RUST_SRC_PATH=")+rustPath);

    //run 'racer complete <linenum> <charnum> <filename>' and retrieve the output
    QStringList params;
    params << command << QString::number(linenum) << QString::number(charnum) << tmpsrc.fileName();

    QProcess process;
    process.setEnvironment(env);
    process.start(racerPath+QLatin1String("racer"), params);
    process.waitForFinished();

    if (process.exitCode() != 0)
        return QString();

    return QString::fromLatin1(process.readAllStandardOutput());
}
