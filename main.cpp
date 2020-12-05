/*
[Dialer Defaults]
Modem = /dev/ttyUSB2
Modem Type = Analog Modem
Baud = 115200
ISDN = 0
Phone = *99#
Username = beeline
Password = beeline
Stupid Mode = 1
Init1 = AT+CCID
Init2 = AT+CSPN?
Init3 = ATZ
Init4 = ATQ0 V1 E1 S0=0 &C1 &D2 +FCLASS=0
Init5 = AT+CGDCONT=1,"IP","internet"
*/

#include "Dialer.h"
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char *argv[])
{
  QCoreApplication a(argc, argv);
  Dialer w;
  w.dial();
  QTimer::singleShot(5000, &w, &Dialer::reset);
  QTimer::singleShot(15000, &w, &Dialer::dial);
  return a.exec();
}
