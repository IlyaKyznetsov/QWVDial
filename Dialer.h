#ifndef DIALER_H
#define DIALER_H

#include <QMap>
#include <QObject>
#include <QProcess>
#include <QRegExp>

class Dialer : public QObject
{
  Q_OBJECT
  Q_DISABLE_COPY(Dialer);

public:
  struct ConnectionState
  {
    int pidWvdial = -1;
    int pidPppd = -1;
    QString CCID;
    QString provider;
    QString interface;
    QString addressLocal;
    QString addressRemote;
    QString dnsPrimary;
    QString dnsSecondary;
  };
  explicit Dialer(QObject *parent = nullptr);

public Q_SLOTS:
  void reset();
  void dial();

Q_SIGNALS:
  void stateChanged(const ConnectionState state);

private Q_SLOTS:
  void onReadyReadStandardOutput();
  void onReadyReadStandardError();
  void onFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void debugStateChanged(const ConnectionState state);

private:
  void parseWvdial(const QList<QByteArray> &data);
  void parseUsbModeSwitch(const QList<QByteArray> &data);
  QProcess *p_usb_modeswitch, *p_wvdial;
  ConnectionState _state;
  enum Key
  {
    None = 0,
    CCID,
    Provider,
    PID,
    Interface,
    AddressLocal,
    AddressRemote,
    DnsPrimary,
    DnsSecondary
  };
  struct Filter
  {
    Key key;
    QRegExp rx;
  };
  QList<Filter> _filters;
};

#endif // DIALER_H
