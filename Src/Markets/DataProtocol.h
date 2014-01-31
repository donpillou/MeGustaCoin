
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
  };

#pragma pack(push, 1)
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
  };
  struct SubscribeResponse
  {
    char channel[33];
    quint64 channelId;
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
#pragma pack(pop)

};
