#include "mainwindow.h"
#include "./ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("串口调试助手");
    this->setWindowIcon(QIcon(":/img/wrench.png"));

    label_state = new QLabel("");
    ui->statusbar->addWidget(label_state);

    serial_port = new QSerialPort;

    // 初始化串口设置信息
    findSerialPorts();
    timer_serial_ports_refresh = new QTimer;
    timer_serial_ports_refresh->setInterval(1000);
    connect(timer_serial_ports_refresh, &QTimer::timeout, this, [=](){
        findSerialPorts();
    });
    timer_serial_ports_refresh->start();

    QStringList items_baud_rate;
    items_baud_rate << "100" << "300" << "600" << "1200" << "2400" << "4800" << "9600" << "14400" << "19200" << "38400" << "56000"
              << "67600" << "115200" << "128000" << "230400" << "256000" << "460800" << "921600" << "1000000" << "2000000";
    ui->comboBox_baud_rate->addItems(items_baud_rate);
    ui->comboBox_baud_rate->setCurrentText("9600");

    QStringList items_check_digit;
    items_check_digit << "NONE" << "ODD" << "EVEN" << "MARK" << "SPACE";
    ui->comboBox_check_digit->addItems(items_check_digit);
    ui->comboBox_check_digit->setCurrentIndex(0);

    QStringList items_data_digit;
    items_data_digit << "5" << "6" << "7" << "8";
    ui->comboBox_data_digit->addItems(items_data_digit);
    ui->comboBox_data_digit->setCurrentIndex(3);

    QStringList items_stop_digit;
    items_stop_digit << "1" << "1.5" << "2";
    ui->comboBox_stop_digit->addItems(items_stop_digit);
    ui->comboBox_stop_digit->setCurrentIndex(0);

    ui->radioButton_recv_hex->setChecked(true);
    ui->radioButton_send_hex->setChecked(true);

    // 连接串口
    ui->pushButton_send->setEnabled(false);
    connect(ui->pushButton_serial_port_connect, &QPushButton::clicked, this, [=](){
        if (serial_port_connect_state == false) {
            if (connectSerialPort()) {
                this->ui->pushButton_serial_port_connect->setText("断开");
                serial_port_connect_state = true;
                ui->pushButton_send->setEnabled(true);
            }
        } else {
            this->serial_port->close();
            this->ui->pushButton_serial_port_connect->setText("连接");
            serial_port_connect_state = false;
            ui->pushButton_send->setEnabled(false);
        }
    });

    // 发送串口数据
    connect(ui->pushButton_send, &QPushButton::clicked, this, [=](){
        QString text = ui->plainTextEdit_send->toPlainText();
        QStringList info = text.split('\n', Qt::SkipEmptyParts);
        for(const QString &data : info) {
            QByteArray data_send = prepareSendData(data);
            serial_port->write(data_send);
            // qDebug() << "[origin]" << Qt::hex << prepareSendData(data).toHex(' ').toUpper();
            // qDebug() << "[added crc16]" << Qt::hex << calcCrc16Modbus(prepareSendData(data)).toHex(' ').toUpper();
            // qDebug() << "[added crc32]" << Qt::hex << calcCrc32(prepareSendData(data)).toHex(' ').toUpper();
            flushTextSend(data_send.toHex(' ').toUpper());
        }
    });

    // 接收串口数据
    connect(this->serial_port, &QSerialPort::readyRead, this, [=](){
        QByteArray msg = this->serial_port->readAll();
        if (msg.isEmpty()) return;
        if (ui->radioButton_recv_hex->isChecked()) {    // HEX模式
            flushTextRecv(msg.toHex(' ').toUpper());
        } else {    // ASCII模式
            QString asciiStr;
            for (char ch : msg) {
                if (ch >= 32 && ch <= 126) {  // 可打印ASCII范围
                    asciiStr += ch;
                } else if (ch == '\r') {
                    asciiStr += "\\r";
                } else if (ch == '\n') {
                    asciiStr += "\\n";
                } else if (ch == '\t') {
                    asciiStr += "\\t";
                } else {
                    // 其他控制字符显示为.
                    asciiStr += '.';
                }
            }
            flushTextRecv(asciiStr);
        }
    });

    // 校验值类型选择
    QStringList crc_type;
    crc_type << "CRC-16(Modbus)" << "CRC-32";
    ui->comboBox_crc_type->addItems(crc_type);

    // 仅在hex模式支持追加校验，hex模式显示选项
    connect(ui->radioButton_send_hex, &QRadioButton::clicked, this, [=](){
        ui->checkBox_atuo_add_crc->show();
        ui->comboBox_crc_type->show();
    });

    // 仅在hex模式支持追加校验，ascii模式隐藏选项
    connect(ui->radioButton_send_ascii, &QRadioButton::clicked, this, [=](){
        ui->checkBox_atuo_add_crc->hide();
        ui->comboBox_crc_type->hide();
    });

    // 清空日志
    connect(ui->pushButton_clear_log, &QPushButton::clicked, this, [=](){
        ui->plainTextEdit_log->clear();
    });

    // 清空发送
    connect(ui->pushButton_clear_send, &QPushButton::clicked, this, [=](){
        ui->plainTextEdit_send->clear();
    });
}

MainWindow::~MainWindow()
{
    label_state->deleteLater();
    delete ui;
}

void MainWindow::flushTextSend(const QString &text)
{
    // 获取当前时间
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 格式化时间为“yyyy-MM-dd HH:mm:ss.zzz”
    QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");

    ui->plainTextEdit_log->appendPlainText("[" + formattedDateTime + "] - Send\n" + text + "\n");
}

void MainWindow::flushTextRecv(const QString &text)
{
    // 获取当前时间
    QDateTime currentDateTime = QDateTime::currentDateTime();

    // 格式化时间为“yyyy-MM-dd HH:mm:ss.zzz”
    QString formattedDateTime = currentDateTime.toString("yyyy-MM-dd HH:mm:ss.zzz");

    ui->plainTextEdit_log->appendPlainText("[" + formattedDateTime + "] - Recv\n" + text + "\n");
}

void MainWindow::findSerialPorts()
{
    QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    if (ports.isEmpty()) {
        if (ui->comboBox_serial_port->count() != 0) {
            ui->comboBox_serial_port->clear();
        }
        ui->comboBox_serial_port->addItem("无可用串口");
        ui->comboBox_serial_port->setEnabled(false);
        return;
    } else {
        ui->comboBox_serial_port->setEnabled(true);
        QString selected_serial_port = ui->comboBox_serial_port->currentText();

        ui->comboBox_serial_port->clear();
        for (auto it = ports.begin(); it != ports.end(); it++) {
            ui->comboBox_serial_port->addItem(it->portName());
        }

        ui->comboBox_serial_port->setCurrentText(selected_serial_port);
    }
    return;
}

bool MainWindow::connectSerialPort()
{
    if (ui->comboBox_serial_port->isEnabled() == false) {
        QMessageBox::warning(this, "Warn", QString("无串口可供连接"));
        return false;
    }

    if (serial_port) {
        QString serial_port_name = ui->comboBox_serial_port->currentText();
        serial_port->setPortName(serial_port_name);
        if (serial_port->open(QIODevice::ReadWrite) == false) {
            QMessageBox::warning(this, "Warn", QString("连接串口%1失败").arg(serial_port_name));
            return false;
        } else {
            // 设置波特率
            bool is_set_baud_rate = false;
            is_set_baud_rate = serial_port->setBaudRate(ui->comboBox_baud_rate->currentText().toInt());

            // 设置数据位
            bool is_set_data_digit = false;
            int data_digit = ui->comboBox_data_digit->currentText().toInt();
            if (data_digit == 5) is_set_data_digit = serial_port->setDataBits(QSerialPort::Data5);
            else if (data_digit == 6) is_set_data_digit = serial_port->setDataBits(QSerialPort::Data6);
            else if (data_digit == 7) is_set_data_digit = serial_port->setDataBits(QSerialPort::Data7);
            else if (data_digit == 8) is_set_data_digit = serial_port->setDataBits(QSerialPort::Data8);

            // 设置校验位
            bool is_set_check_digit = false;
            QString check_digit = ui->comboBox_check_digit->currentText();
            if (check_digit == "NONE") is_set_check_digit = serial_port->setParity(QSerialPort::NoParity);
            else if (check_digit == "ODD") is_set_check_digit = serial_port->setParity(QSerialPort::OddParity);
            else if (check_digit == "EVEN") is_set_check_digit = serial_port->setParity(QSerialPort::EvenParity);
            else if (check_digit == "MARK") is_set_check_digit = serial_port->setParity(QSerialPort::MarkParity);
            else if (check_digit == "SPACE") is_set_check_digit = serial_port->setParity(QSerialPort::SpaceParity);

            // 设置停止位
            bool is_set_stop_digit = false;
            float stop_digit = ui->comboBox_stop_digit->currentText().toFloat();
            if (stop_digit == 1.0) is_set_stop_digit = serial_port->setStopBits(QSerialPort::OneStop);
            else if (stop_digit == 1.5) is_set_stop_digit = serial_port->setStopBits(QSerialPort::OneAndHalfStop);
            else if (stop_digit == 2.0) is_set_stop_digit = serial_port->setStopBits(QSerialPort::TwoStop);

            if (!is_set_baud_rate) {
                QMessageBox::warning(this, "Warn", QString("波特率设置失败"));
                this->serial_port->close();
                return false;
            } else if (!is_set_data_digit) {
                QMessageBox::warning(this, "Warn", QString("数据位设置失败"));
                this->serial_port->close();
                return false;
            } else if (!is_set_check_digit) {
                QMessageBox::warning(this, "Warn", QString("校验位设置失败"));
                this->serial_port->close();
                return false;
            } else if (!is_set_stop_digit) {
                QMessageBox::warning(this, "Warn", QString("停止位设置失败"));
                this->serial_port->close();
                return false;
            } else {
                return true;
            }
        }
    } else {
        qDebug() << "serial_port == nullptr";
        return false;
    }
}

QByteArray MainWindow::prepareSendData(const QString &input)
{

    if (ui->radioButton_send_hex->isChecked()) {
        // HEX模式：解析十六进制字符串
        QString cleanStr = input;
        cleanStr.remove(QRegularExpression("[^0-9A-Fa-f]"));  // 移除非十六进制字符
        QByteArray res = QByteArray::fromHex(cleanStr.toLatin1());
        // 是否要自动追加CRC校验值
        if (ui->checkBox_atuo_add_crc->isChecked()) {
            if (ui->comboBox_crc_type->currentText() == "CRC-16(Modbus)") {
                res = appendCrc16Modbus(res);
            } else if (ui->comboBox_crc_type->currentText() == "CRC-32") {
                res = appendCrc32(res);
            }
        }
        return res;
    } else {
        // ASCII模式：处理转义字符
        QByteArray result;
        for (int i = 0; i < input.length(); ++i) {
            if (input[i] == '\\' && i + 1 < input.length()) {
                // 处理转义序列
                QChar next = input[i + 1];
                if (next == 'n') {
                    result.append('\n');
                    i++;
                } else if (next == 'r') {
                    result.append('\r');
                    i++;
                } else if (next == 't') {
                    result.append('\t');
                    i++;
                } else if (next == '\\') {
                    result.append('\\');
                    i++;
                } else if (next == 'x' && i + 3 < input.length()) {
                    // 十六进制转义：\x41
                    QString hexStr = input.mid(i + 2, 2);
                    bool ok;
                    int value = hexStr.toInt(&ok, 16);
                    if (ok) {
                        result.append(static_cast<char>(value));
                        i += 3;
                    } else {
                        result.append('\\');
                    }
                } else {
                    result.append('\\');
                }
            } else {
                // 普通字符
                result.append(input[i].toLatin1());
            }
        }
        return result;
    }
}

QByteArray MainWindow::appendCrc16Modbus(const QByteArray& data)
{
    static const uint16_t polynomial = 0xA001;  // CRC-16多项式
    uint16_t crc = 0xFFFF;                        // 初始化CRC值

    for (char byte : data) {
        crc ^= static_cast<uint8_t>(byte);
        for (int i = 0; i < 8; ++i) {
            if (crc & 0x0001) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }

    QByteArray res = data;
    // QByteArray out(2, 0);
    // memcpy(out.data(), &crc, sizeof(uint16_t));
    // qDebug() << "[crc16]" << out.toHex(' ').toUpper();
    res.append(reinterpret_cast<const char*>(&crc), sizeof(uint16_t));
    return res;
}

QByteArray MainWindow::appendCrc32(const QByteArray &data)
{
    static const uint32_t polynomial = 0xEDB88320; // CRC-32多项式
    uint32_t crc = 0xFFFFFFFF;                      // 初始化CRC值

    for (char byte : data) {
        crc ^= static_cast<uint8_t>(byte);
        for (int i = 0; i < 8; ++i) {
            if (crc & 1) {
                crc = (crc >> 1) ^ polynomial;
            } else {
                crc >>= 1;
            }
        }
    }
    crc = ~crc; // 取反得到最终CRC

    QByteArray res = data;
    // QByteArray out(4, 0);
    // memcpy(out.data(), &crc, sizeof(uint32_t));
    // qDebug() << "[crc32]" << out.toHex(' ').toUpper();
    res.append(reinterpret_cast<const char*>(&crc), sizeof(uint32_t));
    return res;
}
