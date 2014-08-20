
#pragma once

class DataConnection
{
public:
  class Trade
  {
  public:
    double amount;
    quint64 time;
    double price;
  };

  class Callback
  {
  public:
    virtual void receivedChannelInfo(const QString& channelName) = 0;
    virtual void receivedSubscribeResponse(const QString& channelName, quint64 channelId, quint32 flags) = 0;
    virtual void receivedUnsubscribeResponse(const QString& channelName, quint64 channelId) = 0;
    virtual void receivedTrade(quint64 channelId, const DataProtocol::Trade& trade) = 0;
    virtual void receivedTicker(quint64 channelId, const DataProtocol::Ticker& ticker) = 0;
    virtual void receivedErrorResponse(const QString& message) = 0;
  };

  bool connect(const QString& server, quint16 port);
  bool process(Callback& callback);
  void interrupt();

  bool loadChannelList();

  bool subscribe(const QString& channel, quint64 lastReceivedTradeId);
  bool unsubscribe(const QString& channel);

  bool readTrade(quint64& channelId, DataProtocol::Trade& trade);

  const QString& getLastError() {return error;}

private:
  SocketConnection connection;
  QByteArray recvBuffer2;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;

  bool receiveMessage(DataProtocol::Header& header, char*& data, size_t& size);

  void handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int dataSize);
};
