
#include "stdafx.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <cerrno>
#endif

#include <zlimdbclient.h>

static class ZlimdbClient
{
public:
  ZlimdbClient()
  {
    zlimdb_init();
  }
  ~ZlimdbClient()
  {
    zlimdb_cleanup();
  }

} zlimdbClient;

bool DataConnection::connect(const QString& server, quint16 port, const QString& userName, const QString& password, Callback& callback)
{
  close();

  zdb = zlimdb_create(zlimdbCallback, this);
  if(!zdb)
    return error = getZlimDbError(), false;
  if(zlimdb_connect(zdb, server.toUtf8().constData(), port, userName.toUtf8().constData(), password.toUtf8().constData()) != 0)
    return error = getZlimDbError(), false;
  this->callback = &callback;
  return true;
}

bool DataConnection::isConnected() const
{
  return zlimdb_is_connected(zdb) == 0;
}

void DataConnection::close()
{
  if(zdb)
  {
    zlimdb_free(zdb);
    zdb = 0;
  }
}

void DataConnection::interrupt()
{
  zlimdb_interrupt(zdb);
}

bool DataConnection::process()
{
  for(;;)
    if(zlimdb_exec(zdb, 60 * 1000) != 0)
      switch(zlimdb_errno())
      {
      case zlimdb_local_error_interrupted:
        return true;
      case zlimdb_local_error_timeout:
        break;
      default:
        return error = getZlimDbError(), false;
      }
  return true;
}

QString DataConnection::getZlimDbError()
{
  int err = zlimdb_errno();
  if(err == zlimdb_local_error_system)
  {
#ifdef _WIN32
    TCHAR errorMessage[256];
    DWORD len = FormatMessage(
          FORMAT_MESSAGE_FROM_SYSTEM |
          FORMAT_MESSAGE_IGNORE_INSERTS,
          NULL,
          GetLastError(),
          MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
          (LPTSTR) errorMessage,
          256, NULL );
    Q_ASSERT(len >= 0 && len <= 256);
    while(len > 0 && isspace(errorMessage[len - 1]))
      --len;
    errorMessage[len] = '\0';
    return QString(errorMessage);
#else
    return QString(strerror(errno)) + ".";
#endif
  }
  else
    return QString(zlimdb_strerror(err)) + ".";
}

void DataConnection::zlimdbCallback(const zlimdb_header& message)
{
  switch(message.message_type)
  {
  case zlimdb_message_add_request:
    if(message.size >= sizeof(zlimdb_add_request) + sizeof(meguco_trade_entity))
    {
      const zlimdb_add_request* addRequest = (zlimdb_add_request*)&message;
      const meguco_trade_entity* trade = (const meguco_trade_entity*)((char*)addRequest + sizeof(zlimdb_add_request));
      callback->receivedTrade(addRequest->table_id, *trade);
    }
    break;
  default:
    break;
  }
  /*
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
      callback->receivedSubscribeResponse(channelName, subscribeResponse->channelId, subscribeResponse->flags);
    }
    break;
  case DataProtocol::MessageType::unsubscribeResponse:
    if(size >= sizeof(DataProtocol::UnsubscribeResponse))
    {
      DataProtocol::UnsubscribeResponse* unsubscribeResponse = (DataProtocol::UnsubscribeResponse*)data;
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
  */
}

bool DataConnection::loadChannelList()
{
  if(zlimdb_query(zdb, zlimdb_table_tables, zlimdb_query_type_all, 0) != 0)
    return error = getZlimDbError(), false;
  char buffer[0xffff];
  uint32_t size = sizeof(buffer);
  for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
    for(const zlimdb_table_entity* table; table = (const zlimdb_table_entity*)zlimdb_get_entity(sizeof(zlimdb_table_entity), &data, &size);)
    {
      QString channelName(QByteArray::fromRawData((char*)table + sizeof(zlimdb_table_entity), table->name_size));
      if(channelName.startsWith("markets/") && channelName.endsWith("/trades"))
        callback->receivedChannelInfo(table->entity.id, channelName.mid(8, channelName.length() - (8 + 7)));
    }
  if(zlimdb_errno() != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::subscribe(quint32 channelId, quint64 lastReceivedTradeId)
{
  if(lastReceivedTradeId != 0)
  {
    if(zlimdb_subscribe(zdb, channelId, zlimdb_query_type_since_id, lastReceivedTradeId) != 0)
      return error = getZlimDbError(), false;
  }
  else
  {
    int64_t serverTime, tableTime;
    if(zlimdb_sync(zdb, channelId, &serverTime, &tableTime) != 0)
      return error = getZlimDbError(), false;
    if(zlimdb_subscribe(zdb, channelId, zlimdb_query_type_since_time, tableTime - 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL) != 0)
      return error = getZlimDbError(), false;
  }

  char buffer[0xffff];
  uint32_t size = sizeof(buffer);
  for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
    for(meguco_trade_entity* trade = (meguco_trade_entity*)buffer, *end = (meguco_trade_entity*)(buffer + size); trade < end; trade = (meguco_trade_entity*)((char*)trade + trade->entity.size))
      callback->receivedTrade(channelId, *trade);
  if(zlimdb_errno() != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::unsubscribe(quint32 channelId)
{
  if(zlimdb_unsubscribe(zdb, channelId) != 0)
    return error = getZlimDbError(), false;
  return true;
}
