#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QFileSystemModel>
#include <QTreeView>

class QPlainTextEdit;
class QDockWidget;
class TerminalWidget;

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
    bool compileCode(bool releaseMode = false);   // ← added bool parameter
    void runCode();
    void viewOutput();

private:
    QPlainTextEdit  *codeEditor;
    QDockWidget     *terminalDock;
    TerminalWidget  *terminalWidget;
    QFileSystemModel *fileModel;
    QTreeView        *fileTree;
    QString           currentFile;
};

#endif // MAINWINDOW_H