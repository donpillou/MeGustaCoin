
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
  QByteArray pwhmac = Sha256::hmac(QByteArray((char*)loginResponse.userkey, 64), passwordData);
  QByteArray signature = Sha256::hmac(QByteArray((char*)loginResponse.loginkey, 64), pwhmac);
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

  if(!connection.recv(recvBuffer))
  {
    error = connection.getLastError();
    return false;
  }

  char* buffer = recvBuffer.data();
  unsigned int bufferSize = recvBuffer.size();

  for(;;)
  {
    if(bufferSize >= sizeof(DataProtocol::Header))
    {
      DataProtocol::Header* header = (DataProtocol::Header*)buffer;
      if(header->size < sizeof(DataProtocol::Header))
      {
        error = "Received invalid data.";
        return false;
      }
      if(bufferSize >= header->size)
      {
        handleMessage((DataProtocol::MessageType)header->messageType, (char*)(header + 1), header->size - sizeof(DataProtocol::Header));
        buffer += header->size;
        bufferSize -= header->size;
        continue;
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

void BotConnection::handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int size)
{
  switch(messageType)
  {
  case BotProtocol::loginResponse:
    if(size >= sizeof(BotProtocol::LoginResponse))
      callback->receivedLoginResponse(*(BotProtocol::LoginResponse*)data);
    break;
  case BotProtocol::authResponse:
    callback->receivedAuthResponse();
    break;
  case BotProtocol::MessageType::errorResponse:
    if(size >= sizeof(DataProtocol::ErrorResponse))
    {
      BotProtocol::ErrorResponse* errorResponse = (BotProtocol::ErrorResponse*)data;
      errorResponse->errorMessage[sizeof(errorResponse->errorMessage) - 1] = '\0';
      QString errorMessage(errorResponse->errorMessage);
      callback->receivedErrorResponse(errorMessage);
    }
    break;
  default:
    break;
  }
}

bool BotConnection::sendLoginRequest(const QString& userName)
{
  unsigned char message[sizeof(BotProtocol::Header) + sizeof(BotProtocol::LoginRequest)];
  BotProtocol::Header* header = (BotProtocol::Header*)message;
  BotProtocol::LoginRequest* loginRequest = (BotProtocol::LoginRequest*)(header + 1);
  header->size = sizeof(message);
  header->source = 0;
  header->destination = 0;
  header->messageType = BotProtocol::loginRequest;
  QByteArray userNameData = userName.toUtf8();
  memcpy(loginRequest->username, userNameData.constData(), qMin(userNameData.size() + 1, (int)sizeof(loginRequest->username) - 1));
  loginRequest->username[sizeof(loginRequest->username) - 1] = '\0';
  if(!connection.send((char*)message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool BotConnection::sendAuthRequest(const QByteArray& signature)
{
  unsigned char message[sizeof(BotProtocol::Header) + sizeof(BotProtocol::AuthRequest)];
  BotProtocol::Header* header = (BotProtocol::Header*)message;
  BotProtocol::AuthRequest* authRequest = (BotProtocol::AuthRequest*)(header + 1);
  header->size = sizeof(message);
  header->source = 0;
  header->destination = 0;
  header->messageType = BotProtocol::loginRequest;
  Q_ASSERT(signature.size() == 64);
  memcpy(authRequest->signature, signature.constData(), 64);
  if(!connection.send((char*)message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

//bool BotConnection::peekHeader(BotProtocol::MessageType& type)
//{
//  if(cachedHeader)
//    return true;
//  if(!connection.recv((char*)&header, sizeof(BotProtocol::Header)))
//  {
//    error = connection.getLastError();
//    return false;
//  }
//  cachedHeader = true;
//  return true;
//}
//
//bool BotConnection::peekHeaderExpect(BotProtocol::MessageType expectedType, size_t minSize)
//{
//  BotProtocol::MessageType messageType;
//  if(!peekHeader(messageType))
//    return false;
//  if(messageType != expectedType)
//  {
//    if(messageType == BotProtocol::errorResponse)
//    {
//      BotProtocol::ErrorResponse errorResponse;
//      if(!receiveErrorResponse(errorResponse))
//        return false;
//      error = errorResponse.errorMessage;
//      return false;
//    }
//    error = "Received unexpected message.";
//    return false;
//  }
//  if(header.size < sizeof(BotProtocol::Header) + minSize)
//  {
//    error = "Received message is too small.";
//    return false;
//  }
//  return true;
//}
//
//bool BotConnection::receiveErrorResponse(BotProtocol::ErrorResponse& errorResponse)
//{
//  if(!peekHeaderExpect(BotProtocol::loginResponse, sizeof(BotProtocol::ErrorResponse)))
//    return false;
//  if(!connection.recv((char*)&errorResponse, header.size))
//  {
//    error = connection.getLastError();
//    return false;
//  }
//  errorResponse.errorMessage[sizeof(errorResponse.errorMessage) - 1] = '\0';
//  cachedHeader = false;
//  return true;
//}

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
    virtual void receivedErrorResponse(const QString& errorMessage)
    {
      error = errorMessage;
      finished = true;
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
    virtual void receivedErrorResponse(const QString& errorMessage)
    {
      error = errorMessage;
      finished = true;
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
