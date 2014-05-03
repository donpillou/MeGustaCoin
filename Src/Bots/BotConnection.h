
#pragma once

class BotConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedLoginResponse(const BotProtocol::LoginResponse& response) {};
    virtual void receivedAuthResponse() {};
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size) {}
    virtual void receivedRemoveEntity(const BotProtocol::Entity& entity) {}
    virtual void receivedControlEntityResponse(BotProtocol::Entity& entity, size_t size) {}
    virtual void receivedCreateEntityResponse(const BotProtocol::CreateEntityResponse& entity) {}
  };

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password);
  bool process(Callback& callback);
  void interrupt();

  const QString& getLastError() {return error;}

  bool createEntity(const void* args, size_t size);
  bool removeEntity(BotProtocol::EntityType type, quint32 id);
  bool controlEntity(const void* args, size_t size);

private:
  SocketConnection connection;
  QByteArray recvBuffer;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;

  void handleMessage(const BotProtocol::Header& header, char* data, size_t dataSize);

  bool sendMessage(BotProtocol::MessageType type, const void* data, size_t dataSize);
  bool sendLoginRequest(const QString& userName);
  bool sendAuthRequest(const QByteArray& signature);

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
