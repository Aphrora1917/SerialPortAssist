#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <QMessageBox>
#include <QIODevice>
#include <QDateTime>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    Ui::MainWindow *ui;

    QLabel *label_state = nullptr;
    QTimer *timer_serial_ports_refresh = nullptr;

    QSerialPort *serial_port = nullptr;
    bool serial_port_connect_state = false;

    QString sentText;
    QString recvText;

    void flushTextSend(const QString &text);
    void flushTextRecv(const QString &text);

    void findSerialPorts();
    bool connectSerialPort();
    QByteArray prepareSendData(const QString &input);

    QByteArray appendCrc32(const QByteArray &data);
    QByteArray appendCrc16Modbus(const QByteArray &data);
};
#endif // MAINWINDOW_H
