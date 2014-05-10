
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
  case BotProtocol::controlEntityResponse:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedControlEntityResponse(header.requestId, *(BotProtocol::Entity*)data, size);
    break;
  case BotProtocol::createEntityResponse:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedCreateEntityResponse(header.requestId, *(const BotProtocol::Entity*)data);
    break;
  case BotProtocol::errorResponse:
    if(size >= sizeof(BotProtocol::ErrorResponse))
      callback->receivedErrorResponse(header.requestId, *(BotProtocol::ErrorResponse*)data);
    break;
  default:
    break;
  }
}

bool BotConnection::sendMessage(BotProtocol::MessageType type, quint32 requestId, const void* data, size_t size)
{
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = type;
  header.requestId = requestId;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)data, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::createEntity(quint32 requestId, const void* args, size_t size)
{
  Q_ASSERT(size >= sizeof(BotProtocol::Entity));
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = BotProtocol::createEntity;
  header.requestId = requestId;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)args, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::updateEntity(quint32 requestId, const void* args, size_t size)
{
  Q_ASSERT(size >= sizeof(BotProtocol::Entity));
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = BotProtocol::updateEntity;
  header.requestId = requestId;
  if(!connection.send((const char*)&header, sizeof(header)) ||
     !connection.send((const char*)args, size))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::removeEntity(quint32 requestId, BotProtocol::EntityType type, quint32 id)
{
  char message[sizeof(BotProtocol::Header) + sizeof(BotProtocol::Entity)];
  BotProtocol::Header* header = (BotProtocol::Header*)message;
  BotProtocol::Entity* entity = (BotProtocol::Entity*)(header + 1);
  header->size = sizeof(message);
  header->messageType = BotProtocol::removeEntity;
  header->requestId = requestId;
  entity->entityId = id;
  entity->entityType = type;
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::controlEntity(quint32 requestId, const void* args, size_t size)
{
  Q_ASSERT(size >= sizeof(BotProtocol::Entity));
  BotProtocol::Header header;
  header.size = sizeof(header) + size;
  header.messageType = BotProtocol::controlEntity;
  header.requestId = requestId;
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
  return sendMessage(BotProtocol::loginRequest, 0, &loginRequest, sizeof(loginRequest));
}

bool BotConnection::sendAuthRequest(const QByteArray& signature)
{
  BotProtocol::AuthRequest authRequest;
  Q_ASSERT(signature.size() == 32);
  memcpy(authRequest.signature, signature.constData(), 32);
  return sendMessage(BotProtocol::authRequest, 0, &authRequest, sizeof(authRequest));
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
    }
    virtual void receivedErrorResponse(quint32 requestId, BotProtocol::ErrorResponse& response)
    {
      error = BotProtocol::getString(response.errorMessage);
      finished = true;
    }
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
    }
    virtual void receivedErrorResponse(quint32 requestId, BotProtocol::ErrorResponse& response)
    {
      error = BotProtocol::getString(response.errorMessage);
      finished = true;
    }
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
