
#pragma once

#include <megucoprotocol.h>

typedef struct _zlimdb zlimdb;

class DataConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedChannelInfo(quint32 channelId, const QString& channelName) = 0;
    virtual void receivedSubscribeResponse(quint32 channelId) = 0;
    virtual void receivedTrade(quint32 channelId, const meguco_trade_entity& trade) = 0;
    virtual void receivedTicker(quint32 channelId, const meguco_ticker_entity& ticker) = 0;
  };

  DataConnection() : zdb(0) {}
  ~DataConnection() {close();}

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password, Callback& callback);
  bool isConnected() const;
  void close();
  bool process();
  void interrupt();

  bool loadChannelList();

  bool subscribe(quint32 channelId, quint64 lastReceivedTradeId);
  bool unsubscribe(quint32 channelId);

  const QString& getLastError() {return error;}

private:
  zlimdb* zdb;
  QString error;
  Callback* callback;

private:
  QString getZlimDbError();

  void zlimdbCallback(const zlimdb_header& message);

private:
  static void zlimdbCallback(void* user_data, const zlimdb_header* message) {((DataConnection*)user_data)->zlimdbCallback(*message);}
};
