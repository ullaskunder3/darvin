#include "mainwindow.h"

#include <QPlainTextEdit>
#include <QTextEdit>
#include <QVBoxLayout>
#include <QWidget>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    QVBoxLayout *layout = new QVBoxLayout(central);

    codeEditor = new QPlainTextEdit();
    outputConsole = new QTextEdit();

    outputConsole->setMaximumHeight(150);

    layout->addWidget(codeEditor);
    layout->addWidget(outputConsole);

    setWindowTitle("Darvin IDE");
    resize(900,600);
}
