#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope, ".");
    settings = new QSettings("Tetron.ini", QSettings::IniFormat);
    modbus = new QModbusTcpClient();
    modbusMV210 = new QModbusTcpClient();
    /*modbus = new QModbusRtuSerialMaster();

    modbus->setConnectionParameter(QModbusDevice::SerialPortNameParameter,"COM4");
    modbus->setConnectionParameter(QModbusDevice::SerialParityParameter, 0);
    modbus->setConnectionParameter(QModbusDevice::SerialBaudRateParameter,9600);
    modbus->setConnectionParameter(QModbusDevice::SerialDataBitsParameter,8);
    modbus->setConnectionParameter(QModbusDevice::SerialStopBitsParameter,1);*/

    modbusSlaveID = 5;
    connect(modbus, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        statusBar()->showMessage(modbus->errorString(), 5000);
    });
    connect(modbusMV210, &QModbusClient::errorOccurred, [this](QModbusDevice::Error) {
        statusBar()->showMessage(modbus->errorString(), 5000);
    });
    if (!modbus || !modbusMV210) {
        statusBar()->showMessage(tr("Could not create Modbus master."), 5000);
    }
    modbus->setConnectionParameter(QModbusDevice::NetworkAddressParameter, "192.168.1.99");//settings->value("MKON_IP", "192.168.1.99"));
    modbus->setConnectionParameter(QModbusDevice::NetworkPortParameter, "502");
    modbus->setTimeout(3000);
    modbusMV210->setConnectionParameter(QModbusDevice::NetworkAddressParameter, "192.168.1.100");//settings->value("MKON_IP", "192.168.1.99"));
    modbusMV210->setConnectionParameter(QModbusDevice::NetworkPortParameter, "502");
    modbusMV210->setTimeout(3000);
    //modbus->setNumberOfRetries(5);
    //ui->setIplineEdit->setText(settings->value("MKON_IP", "192.168.1.99").toString());
    // some connect params.
    if (!modbus->connectDevice()) {
        statusBar()->showMessage(tr("Connect failed: ") + modbus->errorString(), 5000);
        qDebug() << modbus->connectionParameter(QModbusDevice::NetworkAddressParameter);
        qDebug() << " modbus->connectDevice failed" + modbus->errorString();
    }
    if (!modbusMV210->connectDevice()) {
        statusBar()->showMessage(tr("Connect failed: ") + modbus->errorString(), 5000);
        qDebug() << modbus->connectionParameter(QModbusDevice::NetworkAddressParameter);
        qDebug() << " modbus->connectDevice failed" + modbus->errorString();
    }
    voltageMB = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 2816, 2);
    currentMB = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 2818, 2);
    inputVMB = new QModbusDataUnit(QModbusDataUnit::Coils, 1296, 1);
    outputMB = new QModbusDataUnit(QModbusDataUnit::Coils, 1298, 1);
    overheatMB = new QModbusDataUnit(QModbusDataUnit::Coils, 1297, 1);
    overheatMBQ1 = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, 51, 2);

    connect(this, SIGNAL(readFinished(QModbusReply*, int)), this, SLOT(onReadReady(QModbusReply*, int)));
    readLoopTimer = new QTimer(this);
    loweringCurrentTimer = new QTimer(this);
    connect(readLoopTimer, SIGNAL(timeout()), this, SLOT(readLoop()));
    connect(loweringCurrentTimer, SIGNAL(timeout()), this, SLOT(loweringCurrent()));
    readLoopTimer->start(1000);
    onPal.setColor(QPalette::WindowText, QColor("#ef5350"));
    offPal.setColor(QPalette::WindowText, QColor("#66bb6a"));
    ui->overheatLabel->setPalette(onPal);
    ui->inputVlabel->setPalette(onPal);
    ui->outputVlabel->setPalette(onPal);
    ui->connectionLabel->setPalette(onPal);
    ui->overheatLabelQ1->setPalette(onPal);
    ui->setCurrentSpinBox->setValue(settings->value("Iset", 0.0).toFloat());
    ui->startButton->setEnabled(false);
}

void MainWindow::readLoop(){
    if (modbus->errorString().isEmpty()){
        ui->connectionLabel->setPalette(offPal);
    }
    else
        ui->connectionLabel->setPalette(onPal);
    if (auto *replyVoltage = modbus->sendReadRequest(*voltageMB, modbusSlaveID)) {
        if (!replyVoltage->isFinished())
            connect(replyVoltage, &QModbusReply::finished, this, [this, replyVoltage](){
                emit readFinished(replyVoltage, 0);  // read fiinished connects to ReadReady()
            });
        else
            delete replyVoltage; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyCurrent = modbus->sendReadRequest(*currentMB, modbusSlaveID)) {
        if (!replyCurrent->isFinished())
            connect(replyCurrent, &QModbusReply::finished, this, [this, replyCurrent](){
                emit readFinished(replyCurrent, 1);
            });
        else
            delete replyCurrent; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyInputV = modbus->sendReadRequest(*inputVMB, modbusSlaveID)) {
        if (!replyInputV->isFinished())
            connect(replyInputV, &QModbusReply::finished, this, [this, replyInputV](){
                emit readFinished(replyInputV, 2);
            });
        else
            delete replyInputV; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyOutputV = modbus->sendReadRequest(*outputMB, modbusSlaveID)) {
        if (!replyOutputV->isFinished())
            connect(replyOutputV, &QModbusReply::finished, this, [this, replyOutputV](){
                emit readFinished(replyOutputV, 3);
            });
        else
            delete replyOutputV; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyOverheat = modbus->sendReadRequest(*overheatMB, modbusSlaveID)) {
        if (!replyOverheat->isFinished())
            connect(replyOverheat, &QModbusReply::finished, this, [this, replyOverheat](){
                emit readFinished(replyOverheat, 4);
            });
        else
            delete replyOverheat; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    if (auto *replyOverheatQ1 = modbusMV210->sendReadRequest(*overheatMBQ1, 1)) {
        if (!replyOverheatQ1->isFinished())
            connect(replyOverheatQ1, &QModbusReply::finished, this, [this, replyOverheatQ1](){
                emit readFinished(replyOverheatQ1, 5);
            });
        else
            delete replyOverheatQ1; // broadcast replies return immediately
    } else {
        statusBar()->showMessage(tr("Read error: ") + modbus->errorString(), 5000);
    }
    //ui->overheatLabel->setAutoFillBackground(true);
}

void MainWindow::onReadReady(QModbusReply* reply, int registerId){ // that's not right. there's four bytes.
    if (!reply)
        return;
    if (reply->error() == QModbusDevice::NoError) {    
        const QModbusDataUnit unit = reply->result();
        if (registerId == 2)
            if (unit.value(0))
                ui->inputVlabel->setPalette(onPal);
            else
                ui->inputVlabel->setPalette(offPal);
        else
        if (registerId == 3)
            if (unit.value(0))
                ui->outputVlabel->setPalette(onPal);
            else
                ui->outputVlabel->setPalette(offPal);
        else
        if (registerId == 4)
            if (unit.value(0))
                ui->overheatLabel->setPalette(onPal);
            else
                ui->overheatLabel->setPalette(offPal);
        else
        if (registerId == 5)
            if ((MV210InputReadedBit & unit.value(0)) == MV210InputReadedBit){
                ui->overheatLabelQ1->setPalette(offPal);
                if (!ui->startButton->isEnabled())
                    ui->startButton->setEnabled(true);
            }
            else{
                ui->overheatLabelQ1->setPalette(onPal);
                qDebug() << " ehm " + QString::number(unit.value(0));
                if (ui->startButton->isEnabled()){
                    ui->startButton->setChecked(false); // check if working properly??
                    ui->startButton->setEnabled(false);
                }
            }
        else{
            float tmp = 0;
            unsigned short data[2];
            data[1] = unit.value(0);
            data[0] = unit.value(1);
            memcpy(&tmp, data, 4);
            if (registerId == 0){          // voltage register
                this->ui->Vlcd->display(tmp);
            }
            else if (registerId == 1) {                       // current register
                ui->Ilcd->display(tmp);
            }
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

void MainWindow::writeRegister(int registerAddr, bool value){
    QModbusDataUnit *writeUnit = new QModbusDataUnit(QModbusDataUnit::Coils, registerAddr, 1); //???? or two?
    writeUnit->setValue(0, value);
    QModbusReply *reply;
    reply = modbus->sendWriteRequest(*writeUnit, modbusSlaveID);
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

void MainWindow::writeRegister(int registerAddr, int value){
    QModbusDataUnit *writeUnit = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddr, 2);
    writeUnit->setValue(0, value);
    QModbusReply *reply;
    reply = modbus->sendWriteRequest(*writeUnit, modbusSlaveID);
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
    QModbusDataUnit *writeUnit = new QModbusDataUnit(QModbusDataUnit::HoldingRegisters, registerAddr, 2);
    unsigned short data[2];
    memcpy(data, &value, 4);
    writeUnit->setValue(0, data[1]);
    writeUnit->setValue(1, data[0]);
    QModbusReply *reply;
    reply = modbus->sendWriteRequest(*writeUnit, modbusSlaveID);
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
    modbus->disconnectDevice();
    delete ui;
}


void MainWindow::on_startButton_toggled(bool checked)
{
    if (checked){
        //writeRegister(1280, true);     // turn on remote mode
        //ui->setCurrentSpinBox->setEnabled(false);
        //writeRegister(2560, 6); // turn ON current supply
        //QThread::msleep(msleep);
        //QThread::msleep(msleep);
        //writeRegister(2563, (float)250);
        //QThread::msleep(msleep);
        writeRegister(2565, (float)15); // set voltage
        writeRegister(2560, 1); //confirm voltage
        //QThread::msleep(msleep);
        writeRegister(2567, (float)ui->setCurrentSpinBox->value()); // set current
        writeRegister(2560, 2); // confirm set current
        //QThread::msleep(msleep);
        writeRegister(2560, 6); // turn ON current supply
        writeRegister(2561, (float)15.7);
        //QThread::msleep(msleep);
        //QThread::msleep(msleep);
        //QThread::msleep(msleep);
    }else{
        //ui->setCurrentSpinBox->setEnabled(true);
        /*writeRegister(2567, (float)0);
        writeRegister(2560, 2);
        writeRegister(2561, (float)15.7);*/
        emit on_setCurrentSpinBox_valueChanged(0);
        //cur = ui->setCurrentSpinBox->value();
        //loweringCurrentTimer->start(msleep);
        QTimer::singleShot(msleep, this, &MainWindow::loweringCurrent);

        //writeRegister(1280, false); // turn off remote mode
    }
}


void MainWindow::on_setCurrentSpinBox_valueChanged(double arg1)
{
    if (ui->startButton->isChecked() || loweringCurrentTimer->isActive()){
        writeRegister(2567, (float)arg1);
        writeRegister(2560, 2); // confirm set current
        writeRegister(2561, (float)15.7);
        /*QThread::msleep(msleep);
        writeRegister(2565, (float)15); // set voltage
        writeRegister(2560, 1); //confirm voltage
        QThread::msleep(msleep);
        writeRegister(2561, (float)15.7);*/

    }
}

/*void MainWindow::on_setIplineEdit_textChanged(const QString &arg1)
{
    modbus->disconnect();
    modbus->setConnectionParameter(QModbusDevice::NetworkAddressParameter, arg1);
    if (!modbus->connectDevice()) {
        statusBar()->showMessage(tr("Connect failed: ") + modbus->errorString(), 5000);
    }
}*/

void MainWindow::on_exitButton_clicked()
{
    //settings->setValue("MKON_IP", ui->setIplineEdit->text());
    settings->setValue("Iset", ui->setCurrentSpinBox->value());
    //writeRegister(2567, (float)0); // set current to zero
    //QThread::msleep(msleep);
    //writeRegister(2560, 2); // confirm set current
    //QThread::msleep(msleep);
    writeRegister(2560, 7); // turn OFF current supply
    writeRegister(2561, (float)15.7);
    //QThread::msleep(msleep);
    //writeRegister(1280, 0); // turn off remote mode
    QTimer::singleShot(1000, this, &MainWindow::timeToStop);
    modbus->disconnectDevice();
}

void MainWindow::timeToStop(){
    QCoreApplication::exit();
}

void MainWindow::loweringCurrent(){
    writeRegister(2560, 7); //  turn off current supply
    writeRegister(2561, (float)15.7);
    /*return;
    if (cur > 10){
        cur -= 10;
        on_setCurrentSpinBox_valueChanged(cur);
        qDebug() << "sending -- " + QString::number(cur);
    }
    else{
        loweringCurrentTimer->stop();
        emit on_setCurrentSpinBox_valueChanged(0);
        writeRegister(2560, 7); //  turn off current supply
        writeRegister(2561, (float)15.7);
    }*/
}
