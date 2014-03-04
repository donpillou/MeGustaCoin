
#include "stdafx.h"

bool DataConnection::connect()
{
  connection.close();
  recvBuffer.clear();

  if(!connection.connect("192.168.0.49", 40123))
  //if(!connection.connect("127.0.0.1", 40123))
  {
    error = connection.getLastError();
    return false;
  }

  // request server time
  DataProtocol::Header header;
  header.size = sizeof(header);
  header.destination = header.source = 0;
  header.messageType = DataProtocol::timeRequest;
  if(!connection.send((char*)&header, sizeof(header)))
  {
    error = connection.getLastError();
    return false;
  }
  qint64 localRequestTime = QDateTime::currentDateTime().toTime_t() * 1000ULL;
  QByteArray recvBuffer;
  recvBuffer.reserve(sizeof(DataProtocol::Header) + sizeof(DataProtocol::TimeResponse));
  do
  {
    if(!connection.recv(recvBuffer))
    {
      error = connection.getLastError();
      return false;
    }
  } while((unsigned int)recvBuffer.length() < sizeof(DataProtocol::Header) + sizeof(DataProtocol::TimeResponse));
  qint64 localResponseTime = QDateTime::currentDateTime().toTime_t() * 1000ULL;
  {
    DataProtocol::Header* header = (DataProtocol::Header*)recvBuffer.data();
    DataProtocol::TimeResponse* timeResponse2 = (DataProtocol::TimeResponse*)(header + 1);
    if(header->size != sizeof(DataProtocol::Header) + sizeof(DataProtocol::TimeResponse))
    {
      error = "Received invalid data.";
      return false;
    }
    if(header->messageType != DataProtocol::timeResponse)
    {
      error = "Could not request server time.";
      return false;
    }
    serverTimeToLocalTime = (localResponseTime - localRequestTime) / 2 + localRequestTime - timeResponse2->time;
  }

  return true;
}

void DataConnection::interrupt()
{
  connection.interrupt();
}

bool DataConnection::process(Callback& callback)
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

void DataConnection::handleMessage(DataProtocol::MessageType messageType, char* data, unsigned int size)
{
  switch(messageType)
  {
  case DataProtocol::MessageType::channelResponse:
    {
      int count = size / sizeof(DataProtocol::Channel);
      DataProtocol::Channel* channel = (DataProtocol::Channel*)data;
      for(int i = 0; i < count; ++i, ++channel)
      {
        channel->channel[sizeof(channel->channel) - 1] = '\0';
        QString channelName(channel->channel);
        callback->receivedChannelInfo(channelName);
      }
    }
    break;
  case DataProtocol::MessageType::subscribeResponse:
    if(size >= sizeof(DataProtocol::SubscribeResponse))
    {
      DataProtocol::SubscribeResponse* subscribeResponse = (DataProtocol::SubscribeResponse*)data;
      subscribeResponse->channel[sizeof(subscribeResponse->channel) - 1] = '\0';
      QString channelName(subscribeResponse->channel);
      callback->receivedSubscribeResponse(channelName, subscribeResponse->channelId);
    }
    break;
  case DataProtocol::MessageType::unsubscribeResponse:
    if(size >= sizeof(DataProtocol::SubscribeResponse))
    {
      DataProtocol::SubscribeResponse* unsubscribeResponse = (DataProtocol::SubscribeResponse*)data;
      unsubscribeResponse->channel[sizeof(unsubscribeResponse->channel) - 1] = '\0';
      QString channelName(unsubscribeResponse->channel);
      callback->receivedUnsubscribeResponse(channelName, unsubscribeResponse->channelId);
    }
    break;
  case DataProtocol::MessageType::tradeMessage:
    if(size >= sizeof(DataProtocol::TradeMessage))
    {
      DataProtocol::TradeMessage* tradeMessage = (DataProtocol::TradeMessage*)data;
      tradeMessage->trade.time += serverTimeToLocalTime;
      callback->receivedTrade(tradeMessage->channelId, tradeMessage->trade);
    }
    break;
  case DataProtocol::MessageType::tickerMessage:
    {
      DataProtocol::TickerMessage* tickerMessage = (DataProtocol::TickerMessage*)data;
      tickerMessage->ticker.time += serverTimeToLocalTime;
      callback->receivedTicker(tickerMessage->channelId, tickerMessage->ticker);
    }
    break;
  case DataProtocol::MessageType::errorResponse:
    if(size >= sizeof(DataProtocol::ErrorResponse))
    {
      DataProtocol::ErrorResponse* errorResponse = (DataProtocol::ErrorResponse*)data;
      errorResponse->errorMessage[sizeof(errorResponse->errorMessage) - 1] = '\0';
      QString errorMessage(errorResponse->errorMessage);
      callback->receivedErrorResponse(errorMessage);
    }
    break;
  default:
    break;
  }
}

bool DataConnection::loadChannelList()
{
  DataProtocol::Header header;
  header.size = sizeof(header);
  header.destination = header.source = 0;
  header.messageType = DataProtocol::channelRequest;
  if(!connection.send((char*)&header, sizeof(header)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool DataConnection::subscribe(const QString& channel, quint64 lastReceivedTradeId)
{
  char message[sizeof(DataProtocol::Header) + sizeof(DataProtocol::SubscribeRequest)];
  DataProtocol::Header* header = (DataProtocol::Header*)message;
  DataProtocol::SubscribeRequest* subscribeRequest = (DataProtocol::SubscribeRequest*)(header + 1);
  header->size = sizeof(message);
  header->destination = header->source = 0;
  header->messageType = DataProtocol::subscribeRequest;
  QByteArray channelNameData = channel.toUtf8();
  memcpy(subscribeRequest->channel, channelNameData.constData(), qMin(channelNameData.size() + 1, (int)sizeof(subscribeRequest->channel) - 1));
  subscribeRequest->channel[sizeof(subscribeRequest->channel) - 1] = '\0';
  if(lastReceivedTradeId)
  {
    subscribeRequest->maxAge = 0;
    subscribeRequest->sinceId =  lastReceivedTradeId;
  }
  else
  {
    subscribeRequest->maxAge = 24ULL * 60ULL * 60ULL * 1000ULL;
    subscribeRequest->sinceId =  0;
  }
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}

bool DataConnection::unsubscribe(const QString& channel)
{
  char message[sizeof(DataProtocol::Header) + sizeof(DataProtocol::UnsubscribeRequest)];
  DataProtocol::Header* header = (DataProtocol::Header*)message;
  DataProtocol::UnsubscribeRequest* unsubscribeRequest = (DataProtocol::UnsubscribeRequest*)(header + 1);
  header->size = sizeof(message);
  header->destination = header->source = 0;
  header->messageType = DataProtocol::unsubscribeRequest;
  QByteArray channelNameData = channel.toUtf8();
  memcpy(unsubscribeRequest->channel, channelNameData.constData(), qMin(channelNameData.size() + 1, (int)sizeof(unsubscribeRequest->channel) - 1));
  unsubscribeRequest->channel[sizeof(unsubscribeRequest->channel) - 1] = '\0';
  if(!connection.send(message, sizeof(message)))
  {
    error = connection.getLastError();
    return false;
  }
  return true;
}
