#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}

// ток изменять не слишком резко, может вылетить ошибка по превышению напряжения.
// Когда перейду с МВ210 на МК надо изменить на 1 регистр чтения вместо двух!!!
