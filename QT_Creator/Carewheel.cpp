#include "Carewheel.h"
#include "./ui_GUICarewheel.h"
#include <QImage>
#include <QPixmap>
#include <arpa/inet.h>
#include <QtEndian>
#include <QThread>
#include <QGraphicsDropShadowEffect>

Carewheel::Carewheel(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::Carewheel)
{
    ui->setupUi(this);
    QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect(this) ;
    shadow->setBlurRadius(15);
    shadow->setOffset(0, 4);
    shadow->setColor(QColor(0,0,0,80));
    ui->frameVitesse->setGraphicsEffect(shadow);

    cap.open(0) ;
    timer = new QTimer(this) ;
    connect(timer, &QTimer::timeout, this, &Carewheel::updateFrame) ;
    timer->start(30);

    //status
    modeAuto = false ;
    vitesse = 0 ;
    timer_status = new QTimer() ;
    connect(timer_status, &QTimer::timeout, this, &Carewheel::affichageStatus) ;
    timer_status->start(500);

    //connection
    color_connexion("rouge");
    ui->lineIP->setPlaceholderText("IP:192.168.1.1") ;
    ui->linePort->setPlaceholderText("Port:5000") ;
    estConnecter = false ;

    process = new QProcess(this) ;
    process->start("/home/robert/Python/env_3.10.20/bin/python",
                   QStringList() << "/home/robert/CPP/QT/CareWheel/eye_tracking.py");

    qDebug() << "State:" << process->state();
    connect(process, &QProcess::readyReadStandardOutput, this, [=]() {
        QByteArray data = process->readAllStandardOutput() ;
        QString output = QString::fromUtf8(data).trimmed() ;
        //ui->labelDirection->setText(output) ;
        qDebug() << output ;
        if (output == "DROITE")
        {
            QPixmap maPhoto("/home/robert/CPP/QT/CareWheel/Direction/right-arrow.png") ;
            QPixmap photo = maPhoto.scaled(ui->iconeDirection->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
            ui->iconeDirection->setPixmap(photo) ;
            if (estConnecter)
            {
                sendDate("D\n");
            }
        }
        else if (output == "GAUCHE")
        {
            QPixmap maPhoto("/home/robert/CPP/QT/CareWheel/Direction/left-arrow.png") ;
            QPixmap photo = maPhoto.scaled(ui->iconeDirection->size(),
                                           Qt::KeepAspectRatio,
                                           Qt::SmoothTransformation);
            ui->iconeDirection->setPixmap(photo) ;
            if (estConnecter)
            {
                sendDate("G\n");
            }
        }
        else
        {
            QPixmap maPhoto("/home/robert/CPP/QT/CareWheel/Direction/up-arrow.png") ;
            QPixmap photo = maPhoto.scaled(ui->iconeDirection->size(),
                                                         Qt::KeepAspectRatio,
                                                         Qt::SmoothTransformation);
            ui->iconeDirection->setPixmap(photo) ;
            if (estConnecter)
            {
                sendDate("C\n");
            }
        }

        //qDebug() << output;
    }) ;


    //initialise serial port
    /*serial = new QSerialPort(this) ;
    serial->setPortName("/dev/ttyUSB0");
    serial->setBaudRate(QSerialPort::Baud115200) ;
    serial->setDataBits(QSerialPort::Data8) ;
    serial->setParity(QSerialPort::NoParity) ;
    serial->setStopBits(QSerialPort::OneStop) ;
    serial->setFlowControl(QSerialPort::NoFlowControl) ;*/

    connect(ui->bouttonConnecter, &QPushButton::clicked, this, &Carewheel::connectWifi) ;

    //initialise monitoring santé QChartView
    QValueAxis *axisX = new QValueAxis ;
    QValueAxis *axisY = new QValueAxis ;

    series = new QLineSeries() ;
    chart = new QChart() ;
    chart->addSeries(series);
    //chart->createDefaultAxes();

    axisX->setRange(0, 50);
    axisY->setRange(50, 120);

    chart->addAxis(axisX, Qt::AlignBottom);
    chart->addAxis(axisY, Qt::AlignLeft);

    series->attachAxis(axisX) ;
    series->attachAxis(axisY) ;

    ui->chartview->setChart(chart) ;
    ui->chartview->setRenderHint(QPainter::Antialiasing) ;

    /*timer_chart = new QTimer(this) ;
    connect(timer, &QTimer::timeout, this, [=](){
        int bpm = 60 + (rand()%40) ;
        series->append(x++, bpm);
        axisX->setRange(qMax(0, x - 50), x);
    }) ;
    timer->start(10) ;*/

}

Carewheel::~Carewheel()
{
    if (process)
    {
        process->kill();      // stop Python
        process->waitForFinished();
    }
    delete ui;
}

void Carewheel::color_connexion(QString color)
{
    if (color == "rouge")
    {
        ui->feu_connection->setStyleSheet(
            "border: 1px solid transparent ;"
            "border-radius: 10px ;"
            "background-color: red ;"
            ) ;
        ui->statusConnection->setText("Non connecté") ;
        ui->statusConnection->setStyleSheet("color: red ;");
    }
    else if (color == "green")
    {
        ui->feu_connection->setStyleSheet(
            "border: 1px solid transparent ;"
            "border-radius: 10px ;"
            "background-color: green ;"
            ) ;
        ui->statusConnection->setText("Connecté") ;
        ui->statusConnection->setStyleSheet("color: green ;");
    }
}

void Carewheel::connectWifi()
{
    QString ip ;
    int port ;
    socket = new QTcpSocket(this) ;
    ip = ui->lineIP->text() ;
    port = ui->linePort->text().toInt() ;
    socket->connectToHost(ip, port);

    if (socket->waitForConnected(port))
    {
        qDebug() << "connecté" ;
        connect(socket, &QTcpSocket::readyRead, this, &Carewheel::onReadyRead) ;
        estConnecter = true ;
        color_connexion("green");
        timer_checkConnexion = new QTimer() ;
        connect(timer_checkConnexion, &QTimer::timeout, this, &Carewheel::checkConnexion) ;
        timer_checkConnexion->start(1000) ;
    }
    else
    {
        qDebug() << "erreur de connection" << socket->errorString() ;
        color_connexion("rouge");
        estConnecter = false ;
    }
}

void Carewheel::checkConnexion()
{
    if (!socket)
    {
        return ;
    }
    if (socket->state() != QAbstractSocket::ConnectedState)
    {
        qDebug() << "Connexion perdue" ;
        color_connexion("rouge");
        estConnecter = false ;
        timer_checkConnexion->stop() ;
    }
}

void Carewheel::onReadyRead()
{
    buffer += QString::fromUtf8(socket->readAll());

    while (buffer.contains('\n'))
    {
        int index = buffer.indexOf('\n');
        QString ligne = buffer.left(index).trimmed();
        buffer.remove(0, index + 1);

        splitData(ligne);
        qDebug() << ligne;
        design_monitoring();
        affichageStatus();
    }
}

void Carewheel::affichageStatus()
{
    ui->valeurVitesse->setText(QString::number(getVitesse()) + " m/s");
    ui->statusMode->setText(modeAuto == true ? "Automatique" : "Commandé par vision") ;
}

void Carewheel::updateFrame()
{
    cv::Mat frame;
    cap >> frame;

    if (frame.empty()) return;

    cv::resize(frame, frame, cv::Size(320, 240));

    //SKIP DES FRAMES (OPTIONNEL mais recommandé)
    static int frame_skip = 0;
    frame_skip++;
    if(frame_skip % 3 != 0) return;

    cv::flip(frame, frame, 1);

    //ENCODAGE
    std::vector<uchar> buffer;
    cv::imencode(".jpg", frame, buffer);

    quint32 size = buffer.size();
    quint32 size_be = qToBigEndian(size);

    process->write(reinterpret_cast<const char*>(&size_be), 4);
    process->write(reinterpret_cast<const char*>(buffer.data()), size);

    process->waitForBytesWritten(); //important

    cv::cvtColor(frame, frame, cv::COLOR_BGR2RGB);
    QImage img(
        frame.data,
        frame.cols,
        frame.rows,
        frame.step,
        QImage::Format_RGB888
        );

    QPixmap image = QPixmap::fromImage(img).scaled(
                                       ui->labelCamera->size(),
                                       Qt::KeepAspectRatio,
                                       Qt::SmoothTransformation
        ) ;
    ui->labelCamera->setPixmap(image);
}

void Carewheel::setData(QStringList _data)
{
    list_data = _data ;
}

QString Carewheel::getData(int position)
{
    return list_data[position] ;
}

void Carewheel::splitData(QString data)
{
    int vitesse_t, bpm_t, spO2_t ;
    bool ok ;
    QStringList champs = data.split(";");
    if (champs.size() < 3) {
        qDebug() << "Ligne malformée, ignorée :" << data;
        return;
    }
    setData(champs) ;
    bpm_t = getData(0).toInt(&ok) ;
    setBpm((ok) ? bpm_t : 0);
    spO2_t = getData(1).toInt(&ok) ;
    setSpO2((ok) ? spO2_t : 0) ;
    vitesse_t = getData(2).toInt(&ok) ;
    setVitesse((ok) ? vitesse_t : 0);
}

void Carewheel::setVitesse(int speed)
{
    vitesse = speed ;
}

int Carewheel::getVitesse()
{
    return vitesse ;
}

void Carewheel::setBpm(int _bpm)
{
    bpm = _bpm ;
}

int Carewheel::getBpm()
{
    return bpm ;
}

void Carewheel::setSpO2(int _spO2)
{
    spO2 = _spO2 ;
}

int Carewheel::getSpO2()
{
    return spO2 ;
}

void Carewheel::sendDate(QString _data)
{
    socket->write(_data.toUtf8()) ;
    socket->waitForBytesWritten(1000) ;
}

//graph monitoring santé
void Carewheel::design_monitoring()
{
    int _bpm = 0 ;
    _bpm = getBpm() ;
    series->append(x++, _bpm);
    axisX->setRange(qMax(0, x - 50), x);
}
