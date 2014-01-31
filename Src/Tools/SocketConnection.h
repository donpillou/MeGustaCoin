

#pragma once

class SocketConnection
{
public:
  SocketConnection();
  ~SocketConnection();

  const QString& getLastError() {return error;}

  bool connect(const QString& host, quint16 port);
  void close();
  bool send(const QByteArray& data);
  bool send(const char* data, int len);
  bool recv(QByteArray& data);

  void interrupt();

private:
  void* curl;
  void* s;
  QString error;
  void* socketEvent;
  bool socketEventWriteMode;
  QByteArray sendBuffer;

  QMutex cancelEventMutex;
  void* cancelEvent;

  QString getSocketErrorString();
  QString getSocketErrorString(int error);
};
