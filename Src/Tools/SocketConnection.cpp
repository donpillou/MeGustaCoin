
#include "stdafx.h"

#include <curl/curl.h>
#ifdef _WIN32
#include <WinSock2.h>
#else
#include <unistd.h>
#endif

#ifdef _WIN32
#define ERRNO WSAGetLastError()
#else
typedef int SOCKET;
#define ERRNO errno
#define SOCKET_ERROR (-1)
#define INVALID_SOCKET (-1)
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

SocketConnection::SocketConnection() : curl(0), s((void*)INVALID_SOCKET)
{
#ifdef _WIN32
  cancelEvent = 0;
  socketEvent = 0;
#else
  cancelPipe[0] = 0;
  cancelPipe[1] = 0;
#endif
}

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

#ifdef _WIN32
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
#else
  QMutexLocker locker(&cancelEventMutex);
  if(pipe(cancelPipe) == -1)
  {
    error = getSocketErrorString();
    this->s = (void*)INVALID_SOCKET;
    return false;
  }
#endif

  return true;
}

void SocketConnection::interrupt()
{
  QMutexLocker locker(&cancelEventMutex);
#ifdef _WIN32
  if(cancelEvent != WSA_INVALID_EVENT)
    WSASetEvent(cancelEvent);
#else
  if(cancelPipe[1] != 0)
    write(cancelPipe[1], "", 1);
#endif
}

void SocketConnection::close()
{
  {
    QMutexLocker locker(&cancelEventMutex);
#ifdef _WIN32
    if(cancelEvent != WSA_INVALID_EVENT)
    {
      WSACloseEvent(cancelEvent);
      cancelEvent = WSA_INVALID_EVENT;
    }
#else
    if(cancelPipe[0])
      ::close(cancelPipe[0]);
    if(cancelPipe[1])
      ::close(cancelPipe[1]);
    cancelPipe[0] = 0;
    cancelPipe[1] = 0;
#endif
  }
#ifdef _WIN32
  WSACloseEvent(socketEvent);
  socketEvent = WSA_INVALID_EVENT;
#endif
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

#ifdef _WIN32
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
#endif

  // wait
#ifdef _WIN32
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
#else
  fd_set rfds;
  fd_set wfds;
  FD_ZERO(&rfds);
  FD_ZERO(&wfds);
  for(;;)
  {
    const int timeout = 10000;
    timeval tv = { timeout/1000, (timeout%1000) * 1000 };
    FD_SET((int)s, &rfds);
    if(!sendBuffer.isEmpty())
      FD_SET((int)s, &wfds);
    FD_SET(cancelPipe[0], &rfds);
    int selectResult = select(((int)s > cancelPipe[0] ? (int)s : cancelPipe[0]) + 1, &rfds, &wfds, 0, &tv);
    if(selectResult == 0)
      continue;
    if(selectResult == -1)
    {
      error = getSocketErrorString();
      return false;
    }
    if(FD_ISSET(cancelPipe[0], &rfds))
    {
      char buf;
      read(cancelPipe[0], &buf, 1);
      return true;
    }
    break;
  }
#endif

#ifdef _WIN32
  // check event
  WSANETWORKEVENTS networkEvents;
  if(WSAEnumNetworkEvents((SOCKET)s, socketEvent, &networkEvents) ==  SOCKET_ERROR)
  {
    error = getSocketErrorString();
    return false;
  }
#endif

  // read data if read event occurred
#ifdef _WIN32
  if(networkEvents.lNetworkEvents & (FD_READ | FD_CLOSE))
#else
  if(FD_ISSET((int)s, &rfds))
#endif
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

