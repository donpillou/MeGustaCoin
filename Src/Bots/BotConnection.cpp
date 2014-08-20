
#include "stdafx.h"

bool BotConnection::connect(const QString& server, quint16 port, const QString& userName, const QString& password)
{
  connection.close();
  recvBuffer2.clear();

  if(!connection.connect(server, port))
  {
    error = connection.getLastError();
    return false;
  }

  if(!sendLoginRequest(userName))
    return false;
  BotProtocol::LoginResponse loginResponse;
  if(!receiveLoginResponse2(loginResponse))
    return false;

  QByteArray passwordData = password.toUtf8();
  QByteArray pwhmac = Sha256::hmac(QByteArray((char*)loginResponse.userKey, 32), passwordData);
  QByteArray signature = Sha256::hmac(QByteArray((char*)loginResponse.loginKey, 32), pwhmac);
  if(!sendAuthRequest(signature))
    return false;
  if(!receiveAuthResponse2())
    return false;

  recvBuffer2.clear();
  return true;
}

void BotConnection::interrupt()
{
  connection.interrupt();
}

bool BotConnection::process(Callback& callback)
{
  this->callback = &callback;

  if(!connection.recv(recvBuffer2))
  {
    error = connection.getLastError();
    return false;
  }

  char* buffer = recvBuffer2.data();
  unsigned int bufferSize = recvBuffer2.size();

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
        continue;
      }
    }
    break;
  }
  if(buffer > recvBuffer2.data())
    recvBuffer2.remove(0, buffer - recvBuffer2.data());
  if(recvBuffer2.size() > 4000)
  {
    error = "Received invalid data.";
    return false;
  }
  return true;
}

bool BotConnection::receiveMessage(BotProtocol::Header& header, char*& data, size_t& size)
{
  if(connection.recv((char*)&header, sizeof(header), sizeof(header)) != sizeof(header))
  {
    error = connection.getLastError();
    return false;
  }
  if(header.size < sizeof(BotProtocol::Header))
  {
    error = "Received invalid data.";
    return false;
  }
  size = header.size - sizeof(header);
  if(size > 0)
  {
    recvBuffer2.resize(size);
    data = recvBuffer2.data();
    if(connection.recv(data, size, size) != size)
    {
      error = connection.getLastError();
      return false;
    }
  }
  return true;
}

void BotConnection::handleMessage(const BotProtocol::Header& header, char* data, size_t size)
{
  switch((BotProtocol::MessageType)header.messageType)
  {
  case BotProtocol::updateEntity:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedUpdateEntity(*(BotProtocol::Entity*)data, size);
    break;
  case BotProtocol::removeEntity:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedRemoveEntity(*(const BotProtocol::Entity*)data);
    break;
  case BotProtocol::removeAllEntities:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedRemoveAllEntities(*(const BotProtocol::Entity*)data);
    break;
  case BotProtocol::controlEntityResponse:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedControlEntityResponse(header.requestId, *(BotProtocol::Entity*)data, size);
    break;
  case BotProtocol::createEntityResponse:
    if(size >= sizeof(BotProtocol::Entity))
      callback->receivedCreateEntityResponse(header.requestId, *(BotProtocol::Entity*)data, size);
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
  BotProtocol::setString(loginRequest.userName, userName);
  return sendMessage(BotProtocol::loginRequest, 0, &loginRequest, sizeof(loginRequest));
}

bool BotConnection::sendAuthRequest(const QByteArray& signature)
{
  BotProtocol::AuthRequest authRequest;
  Q_ASSERT(signature.size() == 32);
  memcpy(authRequest.signature, signature.constData(), 32);
  return sendMessage(BotProtocol::authRequest, 0, &authRequest, sizeof(authRequest));
}

bool BotConnection::receiveLoginResponse2(BotProtocol::LoginResponse& loginResponse)
{
  BotProtocol::Header header;
  char* data;
  size_t size;
  if(!receiveMessage(header, data, size))
    return false;
  if(header.messageType == BotProtocol::errorResponse && size >= sizeof(BotProtocol::ErrorResponse))
  {
    BotProtocol::ErrorResponse* errorResponse = (BotProtocol::ErrorResponse*)data;
    error = BotProtocol::getString(errorResponse->errorMessage);
    return false;
  }
  if(header.messageType != BotProtocol::loginResponse || size < sizeof(BotProtocol::LoginResponse))
  {
    error = "Could not receive login response.";
    return false;
  }
  loginResponse = *(BotProtocol::LoginResponse*)data;
  return true;
}

bool BotConnection::receiveAuthResponse2()
{
  BotProtocol::Header header;
  char* data;
  size_t size;
  if(!receiveMessage(header, data, size))
    return false;
  if(header.messageType == BotProtocol::errorResponse && size >= sizeof(BotProtocol::ErrorResponse))
  {
    BotProtocol::ErrorResponse* errorResponse = (BotProtocol::ErrorResponse*)data;
    error = BotProtocol::getString(errorResponse->errorMessage);
    return false;
  }
  if(header.messageType != BotProtocol::authResponse)
  {
    error = "Could not receive auth response.";
    return false;
  }
  return true;
}
