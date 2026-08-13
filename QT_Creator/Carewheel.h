#ifndef CAREWHEEL_H
#define CAREWHEEL_H

#include <QMainWindow>
#include <QTimer>
#include <opencv2/opencv.hpp>
#include <QProcess>
#include <QSerialPort>
#include <QTcpSocket>
#include <QtCharts>
#include <QLineSeries>
#include <QStringList>
#include <QValueAxis>

QT_BEGIN_NAMESPACE
namespace Ui {
class Carewheel;
}
QT_END_NAMESPACE

class Carewheel : public QMainWindow
{
    Q_OBJECT

public:
    Carewheel(QWidget *parent = nullptr);
    ~Carewheel();
    void setData(QStringList _data) ;
    QString getData(int position) ;
    void sendDate(QString _data) ;
    void design_monitoring() ;
    void color_connexion(QString color) ;
    void splitData(QString data) ;
    void setVitesse(int speed) ;
    int getVitesse() ;
    void setBpm(int _bpm) ;
    int getBpm() ;
    void setSpO2(int _spO2) ;
    int getSpO2() ;
    void checkConnexion() ;
    void affichageStatus() ;

private slots:
    void updateFrame() ;
    void connectWifi() ;
    void onReadyRead() ;

private:
    Ui::Carewheel *ui;

    cv::VideoCapture cap ;
    QTimer *timer ;
    QProcess *process ;
    //QSerialPort *serial ;
    QString buffer ;
    QTcpSocket *socket ;
    bool estConnecter ;
    QTimer *timer_checkConnexion ;
    QTimer *timer_status ;

    //variable pour la monitoring BPM
    QLineSeries *series ;
    QChart *chart ;
    QTimer *timer_chart ;
    int x = 0 ;
    QValueAxis *axisX ;
    QValueAxis *axisY ;

    //donnée depuis esp32
    QStringList list_data ;
    int vitesse ;
    int bpm ;
    int spO2 ;
    bool modeAuto ;
};
#endif // CAREWHEEL_H
