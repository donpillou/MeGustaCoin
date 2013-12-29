
#include "stdafx.h"

#include <easywsclient.hpp>

Websocket::Websocket() : data(0) {}

Websocket::~Websocket()
{
  close();
}

bool Websocket::connect(const QString& url)
{
  if(data)
    return false;

  data = easywsclient::WebSocket::from_url(url.toUtf8().constData());
  if(!data)
    return false;

  return true;

  /*
  CURL* curl = curl_easy_init();

 if(!curl)
  {
    error = "Could not initialize curl.";
    return 0;
  }

  struct WriteResult
  {

    static size_t writeResponse(void *ptr, size_t size, size_t nmemb, void *stream)
    {

    }
  } writeResult;

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResult::writeResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeResult);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 20);

  struct curl_slist* headerFields;
  headerFields = curl_slist_append(headerFields, "Host: %s");
  headerFields = curl_slist_append(headerFields, "Upgrade: websocket");
  headerFields = curl_slist_append(headerFields, "Connection: Upgrade");
  headerFields = curl_slist_append(headerFields, "Sec-WebSocket-Key: x3JJHMbDL1EzLkh9GBhXDw==");
  headerFields = curl_slist_append(headerFields, "Sec-WebSocket-Version: 13");

  curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headerFields);

  


  curl_slist_free_all(headerFields);
  */
}

void Websocket::close()
{
  if(!data)
    return;

  easywsclient::WebSocket::pointer ws = (easywsclient::WebSocket::pointer)data;
  ws->close();
  delete ws;
  ws = 0;
}

bool Websocket::isConnected()
{
  if(!data)
    return false;

  easywsclient::WebSocket::pointer ws = (easywsclient::WebSocket::pointer)data;
  if(ws->getReadyState() != easywsclient::WebSocket::OPEN)
    return false;

  return true;
}

bool Websocket::read(QByteArray& buffer, unsigned int timeout)
{
  if(!data)
    return false;

  easywsclient::WebSocket::pointer ws = (easywsclient::WebSocket::pointer)data;
  if (ws->getReadyState() != easywsclient::WebSocket::OPEN)
    return false;

  struct CallbackHandler
  {
    void operator()(const std::string & message)
    {
      buffer->append(message.c_str(), message.length());
    }

    easywsclient::WebSocket::pointer ws;
    QByteArray* buffer;
  } callbackHandler = { ws, &buffer };

  buffer.clear();

  if(ws->getReadyState() != easywsclient::WebSocket::CLOSED)
  {
    ws->poll(timeout);
    ws->dispatch(callbackHandler);
  }
  return ws->getReadyState() == easywsclient::WebSocket::OPEN;
}

bool Websocket::send(const QByteArray& buffer)
{
  if(!data)
    return false;

  easywsclient::WebSocket::pointer ws = (easywsclient::WebSocket::pointer)data;

  ws->send(buffer.constData());
  return true;
}

