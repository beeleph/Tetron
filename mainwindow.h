#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QModbusTcpClient>
#include <QModbusTcpServer>
#include <QModbusDataUnit>
#include <QDebug>
#include <QSettings>
#include <QTimer>
#include <QThread>
#include <QModbusRtuSerialMaster>

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
    QModbusTcpClient *modbusMV210 = nullptr;
    //QModbusRtuSerialMaster *modbus = nullptr;
    QSettings *settings;
    QModbusDataUnit *voltageMB = nullptr;
    QModbusDataUnit *currentMB = nullptr;
    QModbusDataUnit *inputVMB = nullptr;
    QModbusDataUnit *outputMB = nullptr;
    QModbusDataUnit *overheatMB = nullptr;
    QModbusDataUnit *overheatMBQ1 = nullptr;
    QTimer *readLoopTimer, *loweringCurrentTimer;
    QPalette onPal, offPal;
    int modbusSlaveID = 0, msleep = 500, MV210InputReadedBit = 4;
    double cur = 0.0;



signals:
    void readFinished(QModbusReply* reply, int relayId);

private slots:
    void onReadReady(QModbusReply* reply, int registerId);
    void readLoop();
    void on_startButton_toggled(bool checked);
    void on_setCurrentSpinBox_valueChanged(double arg1);
    //void on_setIplineEdit_textChanged(const QString &arg1);
    void on_exitButton_clicked();
    void timeToStop();
    void writeRegister(int registerAddr, bool value);
    void writeRegister(int registerAddr, int value);
    void writeRegister(int registerAddr, float value);
    void loweringCurrent();
};
#endif // MAINWINDOW_H
