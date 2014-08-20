
#pragma once

class BotConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size) {}
    virtual void receivedRemoveEntity(const BotProtocol::Entity& entity) {}
    virtual void receivedRemoveAllEntities(const BotProtocol::Entity& entity) {}
    virtual void receivedControlEntityResponse(quint32 requestId, BotProtocol::Entity& entity, size_t size) {}
    virtual void receivedCreateEntityResponse(quint32 requestId, BotProtocol::Entity& entity, size_t size) {};
    virtual void receivedErrorResponse(quint32 requestId, BotProtocol::ErrorResponse& response) {}
  };

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password);
  bool process(Callback& callback);
  void interrupt();

  const QString& getLastError() {return error;}

  bool createEntity(quint32 requestId, const void* args, size_t size);
  bool updateEntity(quint32 requestId, const void* args, size_t size);
  bool removeEntity(quint32 requestId, BotProtocol::EntityType type, quint32 id);
  bool controlEntity(quint32 requestId, const void* args, size_t size);

private:
  SocketConnection connection;
  QByteArray recvBuffer2;
  QString error;
  Callback* callback;
  qint64 serverTimeToLocalTime;

  bool receiveMessage(BotProtocol::Header& header, char*& data, size_t& size);

  void handleMessage(const BotProtocol::Header& header, char* data, size_t dataSize);

  bool sendMessage(BotProtocol::MessageType type, quint32 requestId, const void* data, size_t dataSize);
  bool sendLoginRequest(const QString& userName);
  bool sendAuthRequest(const QByteArray& signature);

  bool receiveLoginResponse2(BotProtocol::LoginResponse& loginResponse);
  bool receiveAuthResponse2();
};
