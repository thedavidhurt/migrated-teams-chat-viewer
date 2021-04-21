#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

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
    void findInConvo(QString findString);

private:
    Ui::MainWindow *ui;

    QStringList subjects;
    QVector<QStringList> senders;
    QVector<QStringList> sendersUnique;
    QVector<QStringList> bodies;
    QString owner;

    QString loadedFilePath;
    QString savedFilePath;
    QString savedConvo;

    void setConvoItems(QStringList items);
    void readSettings();
    void writeSettings();
};
#endif // MAINWINDOW_H
