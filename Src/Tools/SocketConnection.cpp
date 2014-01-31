
#include "stdafx.h"

#include <curl/curl.h>
#include <WinSock2.h>

#ifdef _WIN32
#define ERRNO WSAGetLastError()
#else
typedef int SOCKET;
#define ERRNO errno
#define SOCKET_ERROR (-1)
#endif

static class Curl
{
public:
  Curl()
  {
    curl_global_init(CURL_GLOBAL_ALL);
  }
  ~Curl()
  {
    curl_global_cleanup();
  }

} curl;

SocketConnection::SocketConnection() : curl(0), s((void*)INVALID_SOCKET), cancelEvent(0), socketEvent(0) {}

SocketConnection::~SocketConnection()
{
  close();
  if(curl)
    curl_easy_cleanup(curl);
}

bool SocketConnection::connect(const QString& host, quint16 port)
{
  close();

  QByteArray httpUrl = QString("http://%1:%2").arg(host).arg(port).toUtf8();

  // initialize curl
  if(!curl)
  {
    curl = curl_easy_init();
    if(!curl)
    {
      error = "Could not initialize curl.";
      return false;
    }
  }
  else
    curl_easy_reset(curl);

  // create connection
  curl_easy_setopt(curl, CURLOPT_URL, httpUrl.constData());
  curl_easy_setopt(curl, CURLOPT_CONNECT_ONLY, 1L);
  CURLcode status = curl_easy_perform(curl);
  if(status != CURLE_OK)
  {
    error = QString(curl_easy_strerror(status)) + ".";
    return false;
  }
  SOCKET s;
  status = curl_easy_getinfo(curl, CURLINFO_LASTSOCKET, &s);
  if(status != CURLE_OK)
  {
    error = QString(curl_easy_strerror(status)) + ".";
    return false;
  }
  this->s = (void*)s;

  {
    QMutexLocker locker(&cancelEventMutex);
    cancelEvent = WSACreateEvent();
  }
  socketEvent = WSACreateEvent();
  if(cancelEvent == WSA_INVALID_EVENT ||
    socketEvent == WSA_INVALID_EVENT ||
    WSAEventSelect(s, socketEvent, FD_READ | FD_CLOSE) == SOCKET_ERROR)
  {
    error = getSocketErrorString();
    {
      QMutexLocker locker(&cancelEventMutex);
      WSACloseEvent(cancelEvent);
      cancelEvent = WSA_INVALID_EVENT;
    }
    WSACloseEvent(socketEvent);
    socketEvent = WSA_INVALID_EVENT;
    this->s = (void*)INVALID_SOCKET;
    return false;
  }
  socketEventWriteMode = false;

  return true;
}

void SocketConnection::interrupt()
{
  QMutexLocker locker(&cancelEventMutex);
  if(cancelEvent != WSA_INVALID_EVENT)
    WSASetEvent(cancelEvent);
}

void SocketConnection::close()
{
  {
    QMutexLocker locker(&cancelEventMutex);
    WSACloseEvent(cancelEvent);
    cancelEvent = WSA_INVALID_EVENT;
  }
  WSACloseEvent(socketEvent);
  socketEvent = WSA_INVALID_EVENT;
  if(curl)
  {
    curl_easy_cleanup(curl);
    curl = 0;
  }
  s = (void*)INVALID_SOCKET;
  sendBuffer.clear();
}

bool SocketConnection::send(const QByteArray& dataArray)
{
  if((SOCKET)s == INVALID_SOCKET)
    return false;
  sendBuffer.append(dataArray);
  return true;
}

bool SocketConnection::send(const char* data, int len)
{
  if((SOCKET)s == INVALID_SOCKET)
    return false;
  sendBuffer.append(data, len);
  return true;
}

bool SocketConnection::recv(QByteArray& data)
{
  if((SOCKET)s == INVALID_SOCKET)
    return false;

  // try sending
  if(!sendBuffer.isEmpty())
  {
    size_t toSend = sendBuffer.size(), totalSent = 0, sent;
    const char* sendData = sendBuffer.data();
    while(totalSent < toSend)
    {
      CURLcode status = curl_easy_send(curl, sendData + totalSent, toSend - totalSent, &sent);
      if(status == CURLE_AGAIN)
        break;
      else if(status != CURLE_OK)
      {
        error = QString(curl_easy_strerror(status)) + ".";
        return false;
      }
      if(sent == 0)
      {
        error = "Connection was closed.";
        return false;
      }
      totalSent += sent;
    }
    if(totalSent > 0)
      sendBuffer.remove(0, totalSent);
  }

  // update event type?
  bool requireSocketEventWriteMode = !sendBuffer.isEmpty();
  if(requireSocketEventWriteMode != socketEventWriteMode)
  {
    if(WSAEventSelect((SOCKET)s, socketEvent, FD_READ | FD_CLOSE | (requireSocketEventWriteMode ? FD_WRITE : 0)) == SOCKET_ERROR)
    {
      error = getSocketErrorString();
      return false;
    }
    socketEventWriteMode = requireSocketEventWriteMode;
  }

  // wait
  HANDLE events[2] = { socketEvent, cancelEvent };
  DWORD result = WSAWaitForMultipleEvents(2, events, FALSE, WSA_INFINITE, FALSE);
  switch(result)
  {
  case WSA_WAIT_EVENT_0: // socket event
    break;
  case WSA_WAIT_EVENT_0 + 1: // cancel event
    WSAResetEvent(cancelEvent);
    return true;
  case WSA_WAIT_FAILED:
  default:
    error = getSocketErrorString();
    return false;
  }

  // check event
  WSANETWORKEVENTS networkEvents;
  if(WSAEnumNetworkEvents((SOCKET)s, socketEvent, &networkEvents) ==  SOCKET_ERROR)
  {
    error = getSocketErrorString();
    return false;
  }

  // read data if read event occurred
  if(networkEvents.lNetworkEvents & (FD_READ | FD_CLOSE))
  {
    int bufferSize = data.size();
    int bufferCapacity = bufferSize + 1500;
    data.resize(bufferCapacity);
    size_t received = 0;
    CURLcode status = curl_easy_recv(curl, (char*)data.data() + bufferSize, bufferCapacity - bufferSize, &received);
    data.resize(bufferSize + received);
    if(status != CURLE_AGAIN)
    {
      if(status != CURLE_OK)
      {
        error = QString(curl_easy_strerror(status)) + ".";
        return false;
      }
      if(received == 0)
      {
        error = "Connection was closed.";
        return false;
      }
    }
  }
  return true;
}

QString SocketConnection::getSocketErrorString()
{
  return getSocketErrorString(ERRNO);
}

QString SocketConnection::getSocketErrorString(int error)
{
#ifdef _WIN32
  char errorMessage[256];
  DWORD len = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        error,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR) errorMessage,
        256, NULL );
  while(len > 0 && isspace(((unsigned char*)errorMessage)[len - 1]))
    --len;
  errorMessage[len] = '\0';
  return QString(errorMessage);
#else
  const char* errorMessage = ::strerror(error);
  return QString(errorMessage);
#endif
}
