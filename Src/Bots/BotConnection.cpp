
#include "stdafx.h"

bool BotConnection::connect(const QString& server, quint16 port, const QString& userName, const QString& password)
{
  connection.close();
  recvBuffer.clear();

  if(!connection.connect(server, port))
  {
    error = connection.getLastError();
    return false;
  }

  if(!sendLoginRequest(userName))
    return false;
  BotProtocol::LoginResponse loginResponse;
  if(!receiveLoginResponse(loginResponse))
    return false;

  QByteArray passwordData = password.toUtf8();
  QByteArray pwhmac = Sha256::hmac(QByteArray((char*)loginResponse.userKey, 32), passwordData);
  QByteArray signature = Sha256::hmac(QByteArray((char*)loginResponse.loginKey, 32), pwhmac);
  if(!sendAuthRequest(signature))
    return false;
  if(!receiveAuthResponse())
    return false;

  return true;
}

void BotConnection::interrupt()
{
  connection.interrupt();
}

bool BotConnection::process(Callback& callback)
{
  this->callback = &callback;

  if(!recvBuffer.isEmpty())
  {
    unsigned int bufferSize = recvBuffer.size();
    if(bufferSize >= sizeof(BotProtocol::Header) && bufferSize >= ((const BotProtocol::Header*)recvBuffer.data())->size)
      goto handle; // skip recv
  }

  if(!connection.recv(recvBuffer))
  {
    error = connection.getLastError();
    return false;
  }

handle:
  char* buffer = recvBuffer.data();
  unsigned int bufferSize = recvBuffer.size();

  for(;;)
  {
    if(bufferSize >= sizeof(BotProtocol::Header))
    {
      BotProtocol::Header* header = (BotProtocol::Header*)buffer;
      if(header->size < sizeof(BotProtocol::Header))
      {
        error = "Received invalid data.";
        return false;
      }
      if(bufferSize >= header->size)
      {
        handleMessage(*header, (char*)(header + 1), header->size - sizeof(BotProtocol::Header));
        buffer += header->size;
        bufferSize -= header->size;
        break;
      }
    }
    break;
  }
  if(buffer > recvBuffer.data())
    recvBuffer.remove(0, buffer - recvBuffer.data());
  if(recvBuffer.size() > 4000)
  {
    error = "Received invalid data.";
    return false;
  }
  return true;
}

void BotConnection::handleMessage(const BotProtocol::Header& header, char* data, size_t size)
{
  switch((BotProtocol::MessageType)header.messageType)
  {
  case BotProtocol::loginResponse:
    if(size >= sizeof(BotProtocol::LoginResponse))
      callback->receivedLoginResponse(*(BotProtocol::LoginResponse*)data);
    break;
  case BotProtocol::authResponse:
    callback->receivedAuthResponse();
    break;
  case BotProtocol::updateEntity:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedUpdateEntity(*(BotProtocol::Entity*)data, size);
    break;
  case BotProtocol::removeEntity:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedRemoveEntity(*(const BotProtocol::Entity*)data);
    break;
  default:
    break;
  }
}

bool BotConnection::sendMessage(BotProtocol::MessageType type, const void* data, size_t size)
{
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = type;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)data, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::createEntity(const void* args, size_t size)
{
  Q_ASSERT(size >= sizeof(BotProtocol::Entity));
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = BotProtocol::createEntity;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)args, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::removeEntity(BotProtocol::EntityType type, quint32 id)
{
  char message[sizeof(BotProtocol::Header) + sizeof(BotProtocol::Entity)];
  BotProtocol::Header* header = (BotProtocol::Header*)message;
  BotProtocol::Entity* entity = (BotProtocol::Entity*)(header + 1);
  header->size = sizeof(message);
  header->messageType = BotProtocol::removeEntity;
  entity->entityId = id;
  entity->entityType = type;
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::controlEntity(const void* args, size_t size)
{
  Q_ASSERT(size >= sizeof(BotProtocol::Entity));
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = BotProtocol::controlEntity;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)args, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::sendLoginRequest(const QString& userName)
{
  BotProtocol::LoginRequest loginRequest;
  setString(loginRequest.userName, userName);
  return sendMessage(BotProtocol::loginRequest, &loginRequest, sizeof(loginRequest));
}

bool BotConnection::sendAuthRequest(const QByteArray& signature)
{
  BotProtocol::AuthRequest authRequest;
  Q_ASSERT(signature.size() == 32);
  memcpy(authRequest.signature, signature.constData(), 32);
  return sendMessage(BotProtocol::authRequest, &authRequest, sizeof(authRequest));
}

bool BotConnection::receiveLoginResponse(BotProtocol::LoginResponse& loginResponse)
{
  struct LoginCallback : public Callback
  {
    BotProtocol::LoginResponse& loginResponse;
    QString error;
    bool finished;
  
    LoginCallback(BotProtocol::LoginResponse& loginResponse) : loginResponse(loginResponse), finished(false) {}
  
  public:
    virtual void receivedLoginResponse(const BotProtocol::LoginResponse& response)
    {
      loginResponse = response;
      finished = true;
    };
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size)
    {
      if((BotProtocol::EntityType)entity.entityType == BotProtocol::error && size >= sizeof(BotProtocol::Error))
      {
        error = ((BotProtocol::Error*)&entity)->errorMessage;
        finished = true;
      }
    };
  } callback(loginResponse);

  do
  {
    if(!process(callback))
      return false;
  } while(!callback.finished);
  if(!callback.error.isEmpty())
  {
    error = callback.error;
    return false;
  }
  return true;
}

bool BotConnection::receiveAuthResponse()
{
  struct AuthCallback : public Callback
  {
    QString error;
    bool finished;
  
    AuthCallback() : finished(false) {}
  
  public:
    virtual void receivedAuthResponse()
    {
      finished = true;
    };
    virtual void receivedUpdateEntity(BotProtocol::Entity& entity, size_t size)
    {
      if((BotProtocol::EntityType)entity.entityType == BotProtocol::error && size >= sizeof(BotProtocol::Error))
      {
        error = ((BotProtocol::Error*)&entity)->errorMessage;
        finished = true;
      }
    };
  } callback;

  do
  {
    if(!process(callback))
      return false;
  } while(!callback.finished);
  if(!callback.error.isEmpty())
  {
    error = callback.error;
    return false;
  }
  return true;
}

//bool BotConnection::createSession(const QString& name, const QString& engine)
//{
//  BotProtocol::CreateSessionRequest createSession;
//  setString(createSession.engine, engine);
//  setString(createSession.name, name);
//  return sendMessage(BotProtocol::createSessionRequest, &createSession, sizeof(createSession));
//}
//
//bool BotConnection::startSession(quint32 id, BotProtocol::StartSessionRequest::Mode mode)
//{
//  BotProtocol::StartSessionRequest startSession;
//  startSession.id = id;
//  startSession.mode = mode;
//  return sendMessage(BotProtocol::startSessionRequest, &startSession, sizeof(startSession));
//}

