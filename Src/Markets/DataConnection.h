
#pragma once

typedef struct _zlimdb zlimdb;

class DataConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedMarket(quint32 tableId, const QString& channelName) = 0;
    virtual void receivedBroker(quint32 brokerId, const meguco_user_broker_entity& broker) = 0;
    virtual void receivedSession(quint32 sessionId, const QString& name, const meguco_user_session_entity& session) = 0;
    virtual void receivedTrade(quint32 tableId, const meguco_trade_entity& trade, qint64 timeOffset) = 0;
    virtual void receivedTicker(quint32 tableId, const meguco_ticker_entity& ticker) = 0;
    virtual void receivedBrokerType(const meguco_broker_type_entity& brokerType, const QString& name) = 0;
    virtual void receivedBotType(const meguco_bot_type_entity& botType, const QString& name) = 0;
    virtual void receivedBrokerBalance(const meguco_user_broker_balance_entity& balance) = 0;
    virtual void receivedBrokerOrder(const meguco_user_broker_order_entity& brokerOrder) = 0; 
    virtual void receivedBrokerTransaction(const meguco_user_broker_transaction_entity& transaction) = 0;
    virtual void receivedSessionOrder(const meguco_user_broker_order_entity& order) = 0;
    virtual void receivedSessionTransaction(const meguco_user_broker_transaction_entity& transaction) = 0;
    virtual void receivedSessionAsset(const meguco_user_session_asset_entity& asset) = 0;
    virtual void receivedSessionLog(const meguco_log_entity& log, const QString& message) = 0;
    virtual void receivedSessionProperty(const meguco_user_session_property_entity& property, const QString& name, const QString& value, const QString& unit) = 0;
  };

  DataConnection() : zdb(0), selectedBrokerId(0), selectedSessionId(0) {}
  ~DataConnection() {close();}

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password, Callback& callback);
  bool isConnected() const;
  void close();
  bool process();
  void interrupt();

  bool subscribe(quint32 tableId, quint64 lastReceivedTradeId);
  bool unsubscribe(quint32 tableId);

  const QString& getLastError() {return error;}

  bool createBroker(quint64 brokerTypeId, const QString& userName, const QString& key, const QString& secret);
  bool removeBroker(quint32 brokerId);
  bool selectBroker(quint32 brokerId);
  bool controlBroker(meguco_user_broker_control_code code);

  bool createBrokerOrder(meguco_user_broker_order_type type, double price, double amount);
  bool controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size);
  bool removeBrokerOrder(quint64 orderId);

  bool createSession(const QString& name, quint64 botTypeId, quint64 brokerId);
  bool removeSession(quint32 sessionId);
  bool selectSession(quint32 sessionId);
  bool controlSession(quint32 sessionId, meguco_user_session_control_code code);

  bool createSessionAsset(meguco_user_session_asset_type type, double balanceComm, double balanceBase, double flipPrice);
  bool controlSessionAsset(quint64 assetId, meguco_user_session_asset_control_code code, const void* data, size_t size);
  bool removeSessionAsset(quint64 assetId);

  bool controlSessionProperty(quint64 propertyId, meguco_user_session_property_control_code code, const void* data, size_t size);

public:
  static void setString(void* data, uint16_t& length, size_t offset, const QByteArray& str)
  {
    length = (uint16_t)str.length();
    qMemCopy((char*)data + offset, str.constData(), str.length());
  }

private:
  struct TableInfo
  {
    enum Type
    {
      tradesTable,
      brokerTable,
      sessionTable,
      brokerBalanceTable,
      brokerOrdersTable,
      brokerTransactionsTable,
      sessionOrdersTable,
      sessionTransactionsTable,
      sessionAssetsTable,
      sessionLogTable,
      sessionPropertiesTable,
    } type;
    quint64 nameId;
    qint64 timeOffset;
  };

  class BrokerData
  {
  public:
    quint32 brokerTableId;
    quint32 balanceTableId;
    quint32 ordersTableId;
    quint32 transactionsTableId;

  public:
    BrokerData() : brokerTableId(0), balanceTableId(0), ordersTableId(0), transactionsTableId(0) {}
  };

  class SessionData
  {
  public:
    quint32 sessionTableId;
    quint32 ordersTableId;
    quint32 transactionsTableId;
    quint32 assetsTableId;
    quint32 logTableId;
    quint32 propertiesTableId;

  public:
    SessionData() : sessionTableId(0), ordersTableId(0), transactionsTableId(0), assetsTableId(0), logTableId(0) {}
  };

private:
  zlimdb* zdb;
  QString error;
  Callback* callback;
  QHash<quint32, TableInfo> tableInfo;
  QString brokerPrefix;
  QString sessionPrefix;
  quint32 lastBrokerId;
  quint32 lastSessionId;
  QHash<quint32, BrokerData> brokerData;
  QHash<quint32, SessionData> sessionData;
  quint32 selectedBrokerId;
  quint32 selectedSessionId;

private:
  QString getZlimDbError();

  void zlimdbCallback(const zlimdb_header& message);

  //bool removeTable(quint32 id);
  //bool controlEntity(quint32 tableId, quint64 entityId, quint32 code);

  bool subscribe(quint32 tableId, TableInfo::Type type);

  void addedEntity(uint32_t tableId, const zlimdb_entity& entity);

  bool addedTable(const zlimdb_table_entity& table);

private:
  static void zlimdbCallback(void* user_data, const zlimdb_header* message) {((DataConnection*)user_data)->zlimdbCallback(*message);}

  static bool getString(const zlimdb_entity& entity, size_t offset, size_t length, QString& result)
  {
    if(offset + length > entity.size)
      return false;
    result = QString::fromUtf8((const char*)&entity + offset, length);
    return true;
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
};
