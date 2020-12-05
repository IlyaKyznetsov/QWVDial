#include "Dialer.h"
#include <QDateTime>
#include <QDebug>
#include <QStringList>
#include <QThread>

#define D(x) qWarning() << QThread::currentThread() << QDateTime::currentDateTime().time() << x
#define DF() qWarning() << QThread::currentThread() << QDateTime::currentDateTime().time() << __PRETTY_FUNCTION__

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
Dialer::Dialer(QObject *parent) : QObject(parent)
{
  DF();

  p_usb_modeswitch = new QProcess(this);
  p_wvdial = new QProcess(this);
  qRegisterMetaType<Dialer::ConnectionState>("Dialer::State");
  const QString ip4 = "[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}\\.[0-9]{1,3}";
  _filters.append(Filter{CCID, QRegExp("^\\+CCID: ([0-9]+)$")});
  //  _filters.append(Filter{Provider, QRegExp("^\\+COPS:.*\\\"(.*)\\\".*$")});
  _filters.append(Filter{Provider, QRegExp("^\\+CSPN: \\\"(.*)\\\"")}); //+CSPN: "Beeline",1
  _filters.append(Filter{PID, QRegExp("^--> Pid of pppd: ([0-9]+)$")});
  _filters.append(Filter{Interface, QRegExp("^--> Using interface ([a-z0-9_]+)$")});
  _filters.append(Filter{AddressLocal, QRegExp("^--> local  IP address (" + ip4 + ")$")});
  _filters.append(Filter{AddressRemote, QRegExp("^--> remote IP address (" + ip4 + ")$")});
  _filters.append(Filter{DnsPrimary, QRegExp("^--> primary   DNS address (" + ip4 + ")$")});
  _filters.append(Filter{DnsSecondary, QRegExp("^--> secondary DNS address (" + ip4 + ")$")});

  connect(p_usb_modeswitch, &QProcess::readyReadStandardOutput, this, &Dialer::onReadyReadStandardOutput);
  connect(p_usb_modeswitch, &QProcess::readyReadStandardError, this, &Dialer::onReadyReadStandardError);
  connect(p_usb_modeswitch, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Dialer::onFinished);
  connect(p_wvdial, &QProcess::readyReadStandardOutput, this, &Dialer::onReadyReadStandardOutput);
  connect(p_wvdial, &QProcess::readyReadStandardError, this, &Dialer::onReadyReadStandardError);
  connect(p_wvdial, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Dialer::onFinished);

  connect(this, &Dialer::stateChanged, this, &Dialer::debugStateChanged);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::reset()
{
  DF();
  // ID_VENDOR_ID=1e0e ID_MODEL_ID=9001

  QStringList args;
  args << "-v 1e0e"
       << "-p 9001"
       << "--reset-usb";
  p_usb_modeswitch->start(QString("/usr/sbin/usb_modeswitch"), args);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::dial()
{
  DF();

  QStringList args;
  args << "--no-syslog";
  args << "--config=/wvdial.conf";
  p_wvdial->start(QString("/usr/bin/wvdial"), args);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::onReadyReadStandardOutput()
{
  QProcess *process = static_cast<QProcess *>(sender());
  DF() << process;
  QByteArray data = process->readAllStandardOutput().simplified();
  if (p_wvdial == process)
  {
    D(data);
  }
  else if (p_usb_modeswitch == process)
  {
    D(data);
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::onReadyReadStandardError()
{
  QProcess *process = qobject_cast<QProcess *>(sender());
  if (!process)
    return;
  if (p_wvdial == process)
  {
    parseWvdial(process->readAllStandardError().split('\n'));
    return;
  }
  if (p_usb_modeswitch == process)
  {
    //    parseUsbModeSwitch(process->readAllStandardError().split('\n'));
    return;
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::onFinished(int exitCode, QProcess::ExitStatus exitStatus)
{
  DF();
  QProcess *process = qobject_cast<QProcess *>(sender());
  if (!process)
    return;

  DF() << process << exitCode << exitStatus;
  if (p_wvdial == process)
  {
    _state = ConnectionState();
  }
  else if (p_usb_modeswitch == process)
  {
  }
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::debugStateChanged(const Dialer::ConnectionState state)
{
  DF();
  D("CCID          " << state.CCID);
  D("provider      " << state.provider);
  D("pid           " << state.pidPppd);
  D("interface     " << state.interface);
  D("addressLocal  " << state.addressLocal);
  D("addressRemote " << state.addressRemote);
  D("dnsPrimary    " << state.dnsPrimary);
  D("dnsSecondary  " << state.dnsSecondary);
}

/*=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=*/
void Dialer::parseWvdial(const QList<QByteArray> &data)
{
  for (const QByteArray &item : data)
  {
    if (item.isEmpty())
      continue;

    for (const Filter &filter : _filters)
    {
      if (-1 != filter.rx.indexIn(item))
      {
        switch (filter.key)
        {
          case CCID: _state.CCID = filter.rx.cap(1); break;
          case Provider: _state.provider = filter.rx.cap(1); break;
          case PID: _state.pidPppd = filter.rx.cap(1).toInt(); break;
          case Interface: _state.interface = filter.rx.cap(1); break;
          case AddressLocal: _state.addressLocal = filter.rx.cap(1); break;
          case AddressRemote: _state.addressRemote = filter.rx.cap(1); break;
          case DnsPrimary: _state.dnsPrimary = filter.rx.cap(1); break;
          case DnsSecondary: _state.dnsSecondary = filter.rx.cap(1); break;
          default:;
        }
        emit stateChanged(_state);
        break;
      }
    }
  }
}
