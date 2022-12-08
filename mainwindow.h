#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModbusTcpClient>
#include <QModbusDataUnit>
#include <QDebug>
#include <QSettings>
#include <QTimer>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;
    QModbusTcpClient *modbus = nullptr;
    QSettings *settings;
    QModbusDataUnit *voltageMB = nullptr;
    QModbusDataUnit *currentMB = nullptr;
    QTimer *readLoopTimer;

    void writeRegister(int registerAddr, int value);
    void writeRegister(int registerAddr, float value);

signals:
    void readFinished(QModbusReply* reply, int relayId);

private slots:
    void onReadReady(QModbusReply* reply, int registerId);
    void readLoop();
    void on_startButton_toggled(bool checked);
    void on_setCurrentSpinBox_valueChanged(double arg1);
    void on_setIplineEdit_textChanged(const QString &arg1);
    void on_exitButton_clicked();
    void timeToStop();
};
#endif // MAINWINDOW_H
