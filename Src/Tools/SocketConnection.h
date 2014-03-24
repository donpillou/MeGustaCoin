

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
  bool recv(const char* data, int maxLen, int& len);
  bool recv(const char* data, int len);

  void interrupt();

private:
  void* curl;
  void* s;
  QString error;
  QMutex cancelEventMutex;

#ifdef _WIN32
  void* socketEvent;
  bool socketEventWriteMode;
  void* cancelEvent;
#else
  int cancelPipe[2];
#endif

  QByteArray sendBuffer;

  QString getSocketErrorString();
  QString getSocketErrorString(int error);
};
