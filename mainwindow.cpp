#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, ".");
    settings = new QSettings("Settings.ini", QSettings::IniFormat);
    modbus = new QModbusTcpClient(this);
    connect(modbus, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        statusBar()->showMessage(modbus->errorString(), 5000);
    });
    if (!modbus) {
        statusBar()->showMessage(tr("Could not create Modbus master."), 5000);
    }
    modbus->setConnectionParameter(QModbusDevice::NetworkAddressParameter, settings->value("MKON_IP", "192.168.1.99"));
    this->ui->setCurrentSpinBox->setValue(settings->value("Iset", 0).toInt());
    // some connect params.
    if (!modbus->connectDevice()) {
        statusBar()->showMessage(tr("Connect failed: ") + modbus->errorString(), 5000);
    }
    voltageMB = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 2816, 4);
    currentMB = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 2818, 4);
    connect(this, SIGNAL(readFinished(QModbusReply*, int)), this, SLOT(onReadReady(QModbusReply*, int)));
    readLoopTimer = new QTimer(this);
    connect(readLoopTimer, SIGNAL(timeout()), this, SLOT(readLoop()));
    readLoopTimer->start(1000);
}

void MainWindow::readLoop(){
    if (auto *replyVoltage = modbus->sendReadRequest(*voltageMB, 1)) {
        if (!replyVoltage->isFinished())
            connect(replyVoltage, &QModbusReply::finished, this, [this, replyVoltage](){
                emit readFinished(replyVoltage, 0);  // read fiinished connects to ReadReady()
            });
        else
            delete replyVoltage; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyCurrent = modbus->sendReadRequest(*currentMB, 1)) {
        if (!replyCurrent->isFinished())
            connect(replyCurrent, &QModbusReply::finished, this, [this, replyCurrent](){
                emit readFinished(replyCurrent, 1);
            });
        else
            delete replyCurrent; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
}

void MainWindow::onReadReady(QModbusReply* reply, int registerId){ // that's not right. there's four bytes.
    if (!reply)
        return;
    if (reply->error() == QModbusDevice::NoError) {
        const QModbusDataUnit unit = reply->result();
        float tmp = 0;
        unsigned short data[2];
        data[0] = unit.value(0);
        data[1] = unit.value(1);
        memcpy(&tmp, data, 4);
        if (registerId == 0){          // voltage register
            this->ui->Vlcd->display(tmp);
        }
        else if (registerId == 1) {                       // current register
            ui->Ilcd->display(tmp);
        }
    } else if (reply->error() == QModbusDevice::ProtocolError) {
        statusBar()->showMessage(tr("Read response error: %1 (Mobus exception: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->rawResult().exceptionCode(), -1, 16), 5000);
    } else {
        statusBar()->showMessage(tr("Read response error: %1 (code: 0x%2)").
                                    arg(reply->errorString()).
                                    arg(reply->error(), -1, 16), 5000);
    }
    reply->deleteLater();
}

void MainWindow::writeRegister(int registerAddr, int value){
    QModbusDataUnit *writeUnit = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddr, 2);
    writeUnit->setValue(0, value);
    QModbusReply *reply;
    reply = modbus->sendWriteRequest(*writeUnit, 1);
    if (reply){
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::ProtocolError) {
                    statusBar()->showMessage(tr("Write response error: %1 (Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16),
                        5000);
                } else if (reply->error() != QModbusDevice::NoError) {
                    statusBar()->showMessage(tr("Write response error: %1 (code: 0x%2)").
                        arg(reply->errorString()).arg(reply->error(), -1, 16), 5000);
                }
                reply->deleteLater();
            });
        } else {
            // broadcast replies return immediately
            reply->deleteLater();
        }
    } else {
        statusBar()->showMessage(tr("Write error: "), 5000);
    }
}

void MainWindow::writeRegister(int registerAddr, float value){
    QModbusDataUnit *writeUnit = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddr, 4);
    unsigned short data[2];
    memcpy(data, &value, 4);
    writeUnit->setValue(0, data[0]);
    writeUnit->setValue(1, data[1]);
    QModbusReply *reply;
    reply = modbus->sendWriteRequest(*writeUnit, 1);
    if (reply){
        if (!reply->isFinished()) {
            connect(reply, &QModbusReply::finished, this, [this, reply]() {
                if (reply->error() == QModbusDevice::ProtocolError) {
                    statusBar()->showMessage(tr("Write response error: %1 (Mobus exception: 0x%2)")
                        .arg(reply->errorString()).arg(reply->rawResult().exceptionCode(), -1, 16),
                        5000);
                } else if (reply->error() != QModbusDevice::NoError) {
                    statusBar()->showMessage(tr("Write response error: %1 (code: 0x%2)").
                        arg(reply->errorString()).arg(reply->error(), -1, 16), 5000);
                }
                reply->deleteLater();
            });
        } else {
            // broadcast replies return immediately
            reply->deleteLater();
        }
    } else {
        statusBar()->showMessage(tr("Write error: "), 5000);
    }
}

MainWindow::~MainWindow()
{
    //save Iset and Ip at da exit.
    delete ui;
}

