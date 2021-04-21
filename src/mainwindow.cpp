#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QProgressDialog>
#include <QTextStream>
#include <QStandardPaths>
#include <QSettings>
#include <QMessageBox>

#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->setWindowTitle("Migrated Teams Chat Viewer");
    this->resize(1200, 600);
    int convoListWidth = 400;
    ui->splitter->setSizes(QList<int>({convoListWidth, this->size().width() - convoListWidth}));

    readSettings();

    ui->convoFilter_lineEdit->setToolTip("Filter by subject or author");

    ui->convos_listWidget->setFont(QFont("", 12));

    QPalette p = ui->convo_edit->palette();
    p.setColor(QPalette::Highlight, QColor(Qt::blue));
    p.setColor(QPalette::HighlightedText, QColor(Qt::white));
    ui->convo_edit->setPalette(p);

    connect(ui->browse_pushButton, &QPushButton::clicked, this, &MainWindow::getFilePath);
    connect(ui->open_pushButton, &QPushButton::clicked, this, &MainWindow::readFile);
    connect(ui->convos_listWidget, &QListWidget::currentTextChanged, this, &MainWindow::displayConvo);
    connect(ui->convoFilter_lineEdit, &QLineEdit::textChanged, this, &MainWindow::filterConvos);

    auto searchConvoFunction = [this]() { findInConvo(ui->find_lineEdit->text()); };
    connect(ui->find_pushButton, &QPushButton::clicked, searchConvoFunction);
    connect(ui->find_lineEdit, &QLineEdit::returnPressed, searchConvoFunction);

    auto searchEverywhereFunction = [this]() { findEverywhere(ui->findEverywhere_lineEdit->text()); };
    connect(ui->findEverywhere_pushButton, &QPushButton::clicked, searchEverywhereFunction);
    connect(ui->findEverywhere_lineEdit, &QLineEdit::returnPressed, searchEverywhereFunction);

    connect(ui->resetFilters_pushButton, &QPushButton::clicked, ui->convoFilter_lineEdit, &QLineEdit::clear);
    connect(ui->resetFilters_pushButton, &QPushButton::clicked, [this]() { isFindEverywhereFilterActive = false; setConvoItems(subjects); });

    if (QFile::exists(ui->filePath_lineEdit->text()))
    {
        readFile();
    }
}

MainWindow::~MainWindow()
{
    writeSettings();
    delete ui;
}

void MainWindow::getFilePath()
{
   QString filePath = QFileDialog::getOpenFileName(this, "Open CSV file",
        QStandardPaths::standardLocations(QStandardPaths::DocumentsLocation).first(), "CSV (Comma delimited) (*.csv);;All files (*.*)");

    ui->filePath_lineEdit->setText(filePath);
}

void MainWindow::readFile()
{
    subjects.clear();
    bodies.clear();
    senders.clear();
    sendersUnique.clear();
    isFindEverywhereFilterActive = false;

    QFile inFile(ui->filePath_lineEdit->text());
    loadedFilePath = ui->filePath_lineEdit->text();

    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        return;
    }

    QProgressDialog progress("Reading messages from " + loadedFilePath, "Cancel", 0, 0, this);
    progress.setWindowModality(Qt::WindowModal);
    progress.setMinimumDuration(500);

    QTextStream in(&inFile);

    QString headerLine = in.readLine();

    this->headers = parseLine(headerLine);

    // Count messages
    int messageCount = 0;
    while (!in.atEnd())
    {
        progress.setValue(messageCount);
        getLine(in);
        messageCount++;
    }
    in.seek(0);
    headerLine = in.readLine();

    progress.setMaximum(messageCount);

    QStringList potentialOwners;
    int readMessageCount = 0;
    while (!in.atEnd())
    {
        progress.setValue(readMessageCount);

        if (progress.wasCanceled())
        {
            break;
        }

        QString line = getLine(in);
        QStringList desiredFields = { "Subject", "Body", "From: (Name)" };
        int maxFieldIndex = 0;
        for (QString field : desiredFields)
        {
            if (!headers.contains(field))
            {
                progress.cancel();
                QMessageBox::critical(this, "Parse error", "Field \"" + field + "\" was not found in CSV headers");
                return;
            }

            maxFieldIndex = std::max(maxFieldIndex, headers.indexOf(field));
        }
        QStringList fields = parseLine(line, maxFieldIndex + 1);

        QString subject = fields.at(headers.indexOf("Subject"));

        if (!subjects.contains(subject))
        {
            subjects.append(subject);
        }

        int subjectIndex = subjects.indexOf(subject);

        // Determine owner of folder by looking for common name in conversations
        if (owner.isEmpty() && subject.startsWith("Conversation with "))
        {
            QString subjectTemp = subject;
            QString namesTemp = subjectTemp.remove(0, QString("Conversation with ").size());
            QStringList names = namesTemp.split(",");

            for (QString potentialOwner : potentialOwners)
            {
                if (!names.contains(potentialOwner))
                {
                    potentialOwners.removeOne(potentialOwner);
                }
            }

            if (potentialOwners.size() == 1)
            {
                owner = potentialOwners.first();
            }
            else
            {
                for (QString name : names)
                {
                    if (!potentialOwners.contains(name))
                    {
                        potentialOwners.append(name);
                    }
                }
            }
        }

        QString body = fields.at(headers.indexOf("Body"));

        bodies.resize(subjects.size());
        bodies[subjectIndex].append(body);

        QString sender = fields.at(headers.indexOf("From: (Name)"));

        senders.resize(subjects.size());
        senders[subjectIndex].append(sender);

        sendersUnique.resize(subjects.size());
        if (!sendersUnique[subjectIndex].contains(sender))
        {
            sendersUnique[subjectIndex].append(sender);
        }

        readMessageCount++;
    }

    // Clean up subject text
    for (QString& subject : subjects)
    {
        subject.remove("Conversation with ");
        subject.remove(owner);

        if (subject.startsWith(","))
        {
            subject.remove(0, 1);
        }
        if (subject.endsWith(","))
        {
            subject.remove(subject.size() - 1, 1);
        }
        subject.replace(",,", ",");
        subject.replace(",", ", ");
    }

    setConvoItems(subjects);

    if (!savedConvo.isEmpty() && savedFilePath == loadedFilePath)
    {
        ui->convos_listWidget->setCurrentRow(subjects.indexOf(savedConvo));
    }
}

void MainWindow::displayConvo(QString subject)
{
    ui->convo_edit->clear();

    // Remove search occurence indicator
    if (isFindEverywhereFilterActive)
    {
        subject = subject.left(subject.lastIndexOf(" ("));
    }

    if (subjects.contains(subject))
    {
        int subjectIndex = subjects.indexOf(subject);
        QString lastSender;
        QString appendString;
        for (int i = bodies[subjectIndex].size() - 1; i >= 0; i--)
        {
            QString body = bodies[subjectIndex][i];
            QString sender = senders[subjectIndex][i];

            if (sender != lastSender)
            {
                if (!lastSender.isEmpty())
                {
                    appendString.append("</p>");
                }
                QString alignString = sender == owner ? "right" : "left";
                QString colorString = sender == owner ? "AliceBlue" : "White";
                appendString.append("<p style=\"background-color:" + colorString + ";\" align=" + alignString + ">");
                appendString.append("<b>" + sender + "</b><br>");
            }
            if (sender == lastSender)
            {
                appendString.append("<br><br>");
            }
            appendString.append(body);
//            appendString.append("<br>");

            lastSender = sender;
        }
        ui->convo_edit->insertHtml(appendString);
        ui->convo_edit->moveCursor(QTextCursor::End);
    }

    if (isFindEverywhereFilterActive)
    {
        findInConvo(ui->findEverywhere_lineEdit->text());
    }
}

void MainWindow::filterConvos(QString filterString)
{
    QStringList filteredSubjects;
    for (QString subject : subjects)
    {
        if (subject.toLower().contains(filterString.toLower()))
        {
            filteredSubjects.append(subject);
        }
        else
        {
            for (QString sender : sendersUnique[subjects.indexOf(subject)])
            {
                if (sender.toLower().contains(filterString.toLower()))
                {
                    filteredSubjects.append(subject);
                }
            }
        }
    }

    setConvoItems(filteredSubjects);
}

bool MainWindow::findInConvo(QString findString)
{
    if (findString.isEmpty())
    {
        return false;
    }

    bool wrapped = false;
    if (!ui->convo_edit->find(findString, QTextDocument::FindBackward))
    {
        ui->convo_edit->moveCursor(QTextCursor::End);
        ui->convo_edit->find(findString, QTextDocument::FindBackward);
        ui->convo_edit->find("\n", QTextDocument::FindBackward);
        wrapped = true;
    }
    return wrapped;
}

void MainWindow::findEverywhere(QString findString)
{
    if (findString.isEmpty())
    {
        isFindEverywhereFilterActive = false;
        setConvoItems(subjects);
        return;
    }

    // New search string
    if (findString != findEverywhereSearchText)
    {
        findEverywhereSearchText = findString;
        QStringList filteredSubjects;
        for (int subjectIdx = 0; subjectIdx < subjects.size(); subjectIdx++)
        {
            QStringList bodyList = bodies[subjectIdx];
            int numOccurences = 0;
            for (QString body : bodyList)
            {
                if (body.contains(findString))
                {
                    numOccurences++;
                }
            }
            if (numOccurences > 0)
            {
                filteredSubjects.append(subjects.at(subjectIdx) + " (" + QString::number(numOccurences) + ")");
            }
        }

        isFindEverywhereFilterActive = true;
        setConvoItems(filteredSubjects);
    }
    else    // Repeated search
    {
        // Go to new convo if current one wrapped
        if (findInConvo(findString))
        {
            ui->convos_listWidget->setCurrentRow(ui->convos_listWidget->currentRow() + 1);
        }
    }
}

void MainWindow::setConvoItems(const QStringList &items)
{
    ui->convos_listWidget->clear();
    ui->convos_listWidget->addItems(items);

    for (int i = 0; i < ui->convos_listWidget->count(); i++)
    {
        QListWidgetItem* item = ui->convos_listWidget->item(i);
        item->setSizeHint(QSize(30, 30));
    }

    ui->convos_listWidget->setCurrentRow(0);
}

void MainWindow::readSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Radiance Technologies", "migrated-teams-chat-viewer");

    settings.beginGroup("MainWindow");
    resize(settings.value("size", size()).toSize());
    move(settings.value("pos", pos()).toPoint());
    savedFilePath = settings.value("filePath", "").toString();
    ui->filePath_lineEdit->setText(savedFilePath);
    savedConvo = settings.value("currentConvo", "").toString();
    settings.endGroup();
}

void MainWindow::writeSettings()
{
    QSettings settings(QSettings::IniFormat, QSettings::UserScope, "Radiance Technologies", "migrated-teams-chat-viewer");

    settings.beginGroup("MainWindow");
    settings.setValue("size", size());
    settings.setValue("pos", pos());
    if (ui->convos_listWidget->count() > 0)
    {
        settings.setValue("filePath", ui->filePath_lineEdit->text());
    }
    settings.setValue("currentConvo", ui->convos_listWidget->currentItem()->text());
    settings.endGroup();
}

QStringList MainWindow::parseLine(QString line, int numFieldsToParse)
{
    numFieldsToParse = std::min(headers.size(), numFieldsToParse);

    QStringList fields;

    QTextStream lineStream(&line);
    QString singleChar = lineStream.read(1);
    QString value;
    bool inQuote = false;
    while (!lineStream.atEnd())
    {
        bool foundCommaOutsideQuotes = false;
        while (!foundCommaOutsideQuotes && !lineStream.atEnd())
        {
            if (singleChar == "\"")
                inQuote = !inQuote;
            else if (singleChar == "," && !inQuote)
                foundCommaOutsideQuotes = true;
            else
                value.append(singleChar);
            singleChar = lineStream.read(1);
        }
        fields.append(value);
        value.clear();

        // Stop parsing when we've found all requested fields
        if (numFieldsToParse > 0 && fields.size() == numFieldsToParse)
        {
            break;
        }
    }

    return fields;
}

QString MainWindow::getLine(QTextStream &in)
{
    QString line = in.readLine();

    // Use slower line reader if fields don't match what we expect
    if (headers.isEmpty() || headers.last() != "Sensitivity")
    {
        QTextStream lineStream(&line);

        int numFields = 1;
        bool inQuote = false;

        QString singleChar = lineStream.read(1);
        while (numFields < headers.size() && !in.atEnd())
        {
            if (singleChar == "\"")
            {
                inQuote = !inQuote;
            }
            else if (singleChar == "," && !inQuote)
            {
                numFields++;
            }

            if (lineStream.atEnd())
            {
                QString newChars = in.readLine();
                lineStream << newChars;
                line.append(newChars);
            }
            singleChar = lineStream.read(1);
        }
    }
    else    // faster
    {
        while (!line.endsWith("\"Normal\"") && !in.atEnd())
        {
            line += in.readLine();
        }
    }
    return line;
}

