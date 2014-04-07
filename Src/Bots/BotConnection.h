
#pragma once

class BotConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedLoginResponse(const BotProtocol::LoginResponse& response) {};
    virtual void receivedAuthResponse() {};
    virtual void receivedUpdateEntity(const BotProtocol::Header& header, char* data, size_t size) {}
    virtual void receivedRemoveEntity(const BotProtocol::Header& header) {}
    //virtual void receivedErrorResponse(const QString& errorMessage) {};
    //virtual void receivedEngine(const QString& engine) {};
    //virtual void receivedSession(quint32 id, const QString& name, const QString& engine) {};
  };

  BotConnection()/* : cachedHeader(false)*/ {}

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password);
  bool process(Callback& callback);
  void interrupt();

  bool createEntity(BotProtocol::EntityType type, const void* args, size_t size);
  bool removeEntity(BotProtocol::EntityType type, quint32 id);
  bool controlEntity(BotProtocol::EntityType type, quint32 id, const void* args, size_t size);

  //bool createSession(const QString& name, const QString& engine);
  //bool startSession(quint32 id, BotProtocol::StartSessionRequest::Mode mode);

  const QString& getLastError() {return error;}

private:
  SocketConnection connection;
  QByteArray recvBuffer;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;
  //BotProtocol::Header header;
  //bool cachedHeader;

  void handleMessage(const BotProtocol::Header& header, char* data, size_t dataSize);

  bool sendMessage(BotProtocol::MessageType type, const void* data, size_t dataSize);
  bool sendLoginRequest(const QString& userName);
  bool sendAuthRequest(const QByteArray& signature);

  //bool peekHeader(BotProtocol::MessageType& type);
  //bool peekHeaderExpect(BotProtocol::MessageType expectedType, size_t minSize);
  //bool receiveErrorResponse(BotProtocol::ErrorResponse& errorResponse);
  bool receiveLoginResponse(BotProtocol::LoginResponse& loginResponse);
  bool receiveAuthResponse();

  template<size_t N> void setString(char(&str)[N], const QString& value)
  {
    QByteArray buf = value.toUtf8();
    size_t size = buf.length() + 1;
    if(size > N - 1)
      size = N - 1;
    memcpy(str, buf.constData(), size);
    str[N - 1] = '\0';
  }
};
