
#pragma once

class BotConnection
{
public:
  class Callback
  {
  public:
    //virtual void receivedChannelInfo(const QString& channelName) = 0;
    //virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId) = 0;
    //virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId) = 0;
    //virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade) = 0;
    //virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker) = 0;
    //virtual void receivedErrorResponse(const QString& message) = 0;
  };

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password);
  bool process(Callback& callback);
  void interrupt();

  const QString& getLastError() {return error;}

private:
  SocketConnection connection;
  QByteArray recvBuffer;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;

  void handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int dataSize);
};
