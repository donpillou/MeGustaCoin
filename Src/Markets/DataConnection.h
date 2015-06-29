
#pragma once

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

  static QString getString(const zlimdb_entity& entity, size_t offset, size_t length)
  {
    if(offset + length > entity.size)
      return QString();
    return QString::fromUtf8((const char*)&entity + offset, length);
  }

  static void setEntityHeader(zlimdb_entity& entity, uint64_t id, uint64_t time, uint16_t size)
  {
    entity.id = id;
    entity.time = time;
    entity.size = size;
  }

  static void setString(zlimdb_entity& entity, uint16_t& length, size_t offset, const QByteArray& str)
  {
    length = (uint16_t)str.length();
    qMemCopy((char*)&entity + offset, str.constData(), str.length());
  }

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
