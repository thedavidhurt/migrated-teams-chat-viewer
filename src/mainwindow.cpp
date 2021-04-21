#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QFileDialog>
#include <QProgressDialog>
#include <QTextStream>
#include <QStandardPaths>
#include <QSettings>
#include <QTimer>

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

    QTimer::singleShot(0, [this]() {
        if (QFile::exists(ui->filePath_lineEdit->text()))
        {
            readFile();
        }
    });
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
    QFile inFile(ui->filePath_lineEdit->text());
    loadedFilePath = ui->filePath_lineEdit->text();

    if (!inFile.open(QIODevice::ReadOnly | QIODevice::Text))
        return;

    QTextStream in(&inFile);

    QString headers = in.readLine();

    int messageCount = 0;
    while (!in.atEnd())
    {
        QString line = in.readLine();
        while (!line.endsWith("\"Normal\""))
        {
            line += in.readLine();
        }
        messageCount++;
    }
    in.seek(0);
    headers = in.readLine();

    QProgressDialog progress("Reading messages from " + loadedFilePath, "Cancel", 0, messageCount, this);
    progress.setWindowModality(Qt::WindowModal);

    QStringList potentialOwners;
    int readMessageCount = 0;
    while (!in.atEnd())
    {
        progress.setValue(readMessageCount);

        if (progress.wasCanceled())
            break;

        QString line = in.readLine();
        while (!line.endsWith("\"Normal\""))
        {
            line += in.readLine();
        }

        QTextStream lineStream(&line);
        QString singleChar = lineStream.read(1);
        bool inQuote = false;
        QString subject;
        while (!lineStream.atEnd())
        {
            if (singleChar == "\"")
                inQuote = !inQuote;
            else if (singleChar == "," && !inQuote)
                break;
            else
                subject.append(singleChar);
            singleChar = lineStream.read(1);
        }

        if (lineStream.atEnd())
        {
            std::cout << "Didn't parse subject correctly" << std::endl;
        }

        if (!subjects.contains(subject))
        {
            subjects.append(subject);
        }

        int subjectIndex = subjects.indexOf(subject);

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

        singleChar = lineStream.read(1);
        inQuote = false;
        QString body;
        while (!lineStream.atEnd())
        {
            if (singleChar == "\"")
                inQuote = !inQuote;
            else if (singleChar == "," && !inQuote)
                break;
            else
                body.append(singleChar);
            singleChar = lineStream.read(1);
        }

        if (lineStream.atEnd())
        {
            std::cout << "Didn't parse body correctly" << std::endl;
        }

        bodies.resize(subjects.size());
        bodies[subjectIndex].append(body);

        singleChar = lineStream.read(1);
        inQuote = false;
        QString sender;
        while (!lineStream.atEnd())
        {
            if (singleChar == "\"")
                inQuote = !inQuote;
            else if (singleChar == "," && !inQuote)
                break;
            else
                sender.append(singleChar);
            singleChar = lineStream.read(1);
        }

        if (lineStream.atEnd())
        {
            std::cout << "Didn't parse body correctly" << std::endl;
        }

        senders.resize(subjects.size());
        senders[subjectIndex].append(sender);

        sendersUnique.resize(subjects.size());
        if (!sendersUnique[subjectIndex].contains(sender))
        {
            sendersUnique[subjectIndex].append(sender);
        }

        readMessageCount++;
    }

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
}

void MainWindow::filterConvos(QString filterString)
{
    ui->convos_listWidget->clear();

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

void MainWindow::findInConvo(QString findString)
{
    if (!ui->convo_edit->find(findString, QTextDocument::FindBackward))
    {
        ui->convo_edit->moveCursor(QTextCursor::End);
        ui->convo_edit->find(findString, QTextDocument::FindBackward);
    }
}

void MainWindow::setConvoItems(QStringList items)
{

    ui->convos_listWidget->addItems(items);

    for (int i = 0; i < ui->convos_listWidget->count(); i++)
    {
        QListWidgetItem* item = ui->convos_listWidget->item(i);
        item->setSizeHint(QSize(30, 30));
    }
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

