#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QString>
#include <QModelIndex>

class QPlainTextEdit;
class QTextEdit;
class TerminalWidget;
class QTreeView;
class QFileSystemModel;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private slots:
    void newDocument();
    void openDocument();
    void saveDocument();
    void openFileFromTree(const QModelIndex &index);

    bool compileCode();
    void runCode();
    void viewOutput();

private:
    QPlainTextEdit *codeEditor;
    TerminalWidget *terminalWidget;
    QTreeView *fileTree;
    QFileSystemModel *fileModel;
    QDockWidget *terminalDock;

    QString currentFile;
};

#endif
