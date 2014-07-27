
#pragma once

class DataProtocol
{
public:
  enum MessageType
  {
    errorResponse,
    subscribeRequest,
    subscribeResponse,
    unsubscribeRequest,
    unsubscribeResponse,
    tradeRequest,
    tradeResponse,
    registerSourceRequest,
    registerSourceResponse,
    registerSinkRequest,
    registerSinkResponse,
    registeredSinkMessage,
    tradeMessage,
    channelRequest,
    channelResponse,
    timeRequest,
    timeResponse,
    timeMessage,
    tickerMessage,
  };

  enum TradeFlag
  {
    replayedFlag = 0x01,
    syncFlag = 0x02,
  };

#pragma pack(push, 4)
  struct Header
  {
    quint32 size; // including header
    quint64 source; // client id
    quint64 destination; // client id
    quint16 messageType; // MessageType
  };

  struct ErrorResponse
  {
    quint16 messageType;
    quint64 channelId;
    char errorMessage[128];
  };

  struct SubscribeRequest
  {
    char channel[33];
    quint64 maxAge;
    quint64 sinceId;
  };
  struct SubscribeResponse
  {
    char channel[33];
    quint64 channelId;
    quint32 flags;
  };
  struct UnsubscribeRequest
  {
    char channel[33];
  };
  struct UnsubscribeResponse
  {
    char channel[33];
    quint64 channelId;
  };

  struct Trade
  {
    quint64 id;
    quint64 time;
    double price;
    double amount;
    quint32 flags;
  };
  struct TradeMessage
  {
    quint64 channelId;
    Trade trade;
  };

  struct Channel
  {
    char channel[33];
  };

  struct TimeResponse
  {
    quint64 time;
  };

  struct Ticker
  {
    quint64 time;
    double bid;
    double ask;
  };

  struct TickerMessage
  {
    quint64 channelId;
    Ticker ticker;
  };
#pragma pack(pop)

};
