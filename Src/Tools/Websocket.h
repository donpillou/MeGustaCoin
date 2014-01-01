
#pragma once

class Websocket
{
public:
  Websocket();
  ~Websocket();

  bool connect(const QString& url);

  void close();

  bool isConnected();

  bool read(QByteArray& data, unsigned int timeout);

  bool send(const QByteArray& data);

  bool sendPing();

private:
  void* data;
  QString error;
};

