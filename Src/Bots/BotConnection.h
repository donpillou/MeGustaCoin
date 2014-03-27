
#pragma once

class BotConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedLoginResponse(const BotProtocol::LoginResponse& response) {};
    virtual void receivedAuthResponse() {};
    virtual void receivedErrorResponse(const QString& errorMessage) {};
    virtual void receivedEngine(const QString& engine) {};
  };

  BotConnection()/* : cachedHeader(false)*/ {}

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password);
  bool process(Callback& callback);
  void interrupt();

  bool createSession(const QString& name, const QString& engine);

  const QString& getLastError() {return error;}

private:
  SocketConnection connection;
  QByteArray recvBuffer;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;
  //BotProtocol::Header header;
  //bool cachedHeader;

  void handleMessage(BotProtocol::MessageType messageType, char* data, unsigned int dataSize);

  bool sendLoginRequest(const QString& userName);
  bool sendAuthRequest(const QByteArray& signature);

  //bool peekHeader(BotProtocol::MessageType& type);
  //bool peekHeaderExpect(BotProtocol::MessageType expectedType, size_t minSize);
  //bool receiveErrorResponse(BotProtocol::ErrorResponse& errorResponse);
  bool receiveLoginResponse(BotProtocol::LoginResponse& loginResponse);
  bool receiveAuthResponse();
};
