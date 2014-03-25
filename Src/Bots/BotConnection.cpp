
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
  QByteArray pwhmac = Sha256::hmac(QByteArray((char*)loginResponse.userkey, 32), passwordData);
  QByteArray signature = Sha256::hmac(QByteArray((char*)loginResponse.loginkey, 32), pwhmac);
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
        handleMessage((BotProtocol::MessageType)header->messageType, (char*)(header + 1), header->size - sizeof(BotProtocol::Header));
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

void BotConnection::handleMessage(BotProtocol::MessageType messageType, char* data, unsigned int size)
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
  case BotProtocol::errorResponse:
    if(size >= sizeof(BotProtocol::ErrorResponse))
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
  header->messageType = BotProtocol::authRequest;
  Q_ASSERT(signature.size() == 32);
  memcpy(authRequest->signature, signature.constData(), 32);
  if(!connection.send((char*)message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
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
