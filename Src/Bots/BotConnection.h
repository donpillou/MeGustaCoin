
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

  BotConnection() : cachedHeader(false) {}

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
  BotProtocol::Header header;
  bool cachedHeader;

  void handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int dataSize);

  bool sendLoginRequest(const QString& userName);

  bool peekHeader(BotProtocol::MessageType& type);
  bool peekHeaderExpect(BotProtocol::MessageType expectedType, size_t minSize);
  bool receiveErrorResponse(BotProtocol::ErrorResponse& errorResponse);
  bool receiveLoginResponse(BotProtocol::LoginResponse& loginResponse);
};
