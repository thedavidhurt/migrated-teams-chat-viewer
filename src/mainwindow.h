#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTextStream>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

public slots:
    void getFilePath();
    void readFile();
    void displayConvo(QString subject);
    void filterConvos(QString filterString);
    bool findInConvo(QString findString);
    void findEverywhere(QString findString);

private:
    Ui::MainWindow *ui;

    QStringList headers;
    QStringList subjects;
    QVector<QStringList> senders;
    QVector<QStringList> sendersUnique;
    QVector<QStringList> bodies;
    QString owner;

    QString loadedFilePath;
    QString savedFilePath;
    QString savedConvo;

    bool isFindEverywhereFilterActive;
    QString findEverywhereSearchText;

    void setConvoItems(const QStringList& items);
    void readSettings();
    void writeSettings();
    QStringList parseLine(QString line, int numFieldsToParse = 0);
    QString getLine(QTextStream& in);
};
#endif // MAINWINDOW_H
