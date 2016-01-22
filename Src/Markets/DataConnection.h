
#pragma once

typedef struct _zlimdb_ zlimdb;

class DataConnection
{
public:
  class Callback
  {
  public:
    virtual void receivedBrokerType(const meguco_broker_type_entity& brokerType, const QString& name) = 0;
    virtual void receivedBotType(const meguco_bot_type_entity& botType, const QString& name) = 0;

    virtual void receivedMarket(quint32 marketId, const QString& channelName) = 0;
    virtual void receivedBroker(quint32 brokerId, const meguco_user_broker_entity& broker, const QString& userName) = 0;
    virtual void removedBroker(quint32 brokerId) = 0;
    virtual void receivedSession(quint32 sessionId, const QString& name, const meguco_user_session_entity& session) = 0;
    virtual void removedSession(quint32 sessionId) = 0;

    virtual void receivedMarketTrade(quint32 marketId, const meguco_trade_entity& trade, qint64 timeOffset) = 0;
    virtual void receivedMarketTicker(quint32 marketId, const meguco_ticker_entity& ticker) = 0;

    virtual void receivedBrokerBalance(const meguco_user_broker_balance_entity& balance) = 0;
    virtual void removedBrokerBalance(quint64 balanceId) = 0;
    virtual void receivedBrokerOrder(const meguco_user_broker_order_entity& brokerOrder) = 0; 
    virtual void removedBrokerOrder(quint64 orderId) = 0;
    virtual void receivedBrokerTransaction(const meguco_user_broker_transaction_entity& transaction) = 0;
    virtual void removedBrokerTransaction(quint64 transactionId) = 0;
    virtual void receivedBrokerLog(const meguco_log_entity& log, const QString& message) = 0;
    virtual void removedBrokerLog(quint64 logId) = 0;

    virtual void receivedSessionOrder(const meguco_user_broker_order_entity& order) = 0;
    virtual void removedSessionOrder(quint64 orderId) = 0;
    virtual void receivedSessionTransaction(const meguco_user_broker_transaction_entity& transaction) = 0;
    virtual void removedSessionTransaction(quint64 transactionId) = 0;
    virtual void receivedSessionAsset(const meguco_user_session_asset_entity& asset) = 0;
    virtual void removedSessionAsset(quint64 assertId) = 0;
    virtual void receivedSessionLog(const meguco_log_entity& log, const QString& message) = 0;
    virtual void removedSessionLog(quint64 logId) = 0;
    virtual void receivedSessionProperty(const meguco_user_session_property_entity& property, const QString& name, const QString& value, const QString& unit) = 0;
    virtual void removedSessionProperty(quint64 propertyId) = 0;

    virtual void receivedProcess(const meguco_process_entity& process, const QString& cmd) = 0;
    virtual void removedProcess(quint64 processId) = 0;
  };

  DataConnection() : zdb(0), userTableId(0), selectedBrokerId(0), selectedSessionId(0) {}
  ~DataConnection() {close();}

  bool connect(const QString& server, quint16 port, const QString& userName, const QString& password, Callback& callback);
  bool isConnected() const;
  void close();
  bool process();
  void interrupt();

  bool subscribeMarket(quint32 marketId, quint64 lastReceivedTradeId);
  bool unsubscribeMarket(quint32 marketId);

  const QString& getLastError() {return error;}

  bool controlUser(quint64 entityId, meguco_user_control_code code, const void* args, size_t argsSize, bool& result);

  bool createBroker(quint64 brokerTypeId, const QString& userName, const QString& key, const QString& secret);
  bool removeBroker(quint32 brokerId);
  bool selectBroker(quint32 brokerId);
  bool controlBroker(quint64 entityId, meguco_user_broker_control_code code, bool& result);

  bool createBrokerOrder(meguco_user_broker_order_type type, double price, double amount, quint64& orderId);
  bool updateBrokerOrder(quint64 orderId, double price, double amount, bool& result);
  //bool controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code controlCode, bool& result);

  bool createSession(const QString& name, quint64 botTypeId, quint64 brokerId);
  bool removeSession(quint32 sessionId);
  bool selectSession(quint32 sessionId);
  bool controlSession2(quint64 entityId, meguco_user_session_control_code controlCode, bool& result); // todo: rename to controlSession

  bool createSessionAsset(meguco_user_session_asset_type type, double balanceComm, double balanceBase, double flipPrice, quint64& assetId);
  bool updateSessionAsset(quint64 assetId, double flipPrice, bool& result);
  //bool controlSessionAsset(quint64 assetId, meguco_user_session_asset_control_code controlCode, bool& result);

  bool updateSessionProperty(quint64 propertyId, const QString& value);

private:
  struct TableInfo
  {
    enum Type
    {
      tablesTable,
      marketTradesTable,
      brokerTable,
      sessionTable,
      brokerBalanceTable,
      brokerOrdersTable,
      brokerTransactionsTable,
      brokerLogTable,
      sessionOrdersTable,
      sessionTransactionsTable,
      sessionAssetsTable,
      sessionLogTable,
      sessionPropertiesTable,
      processesTable,
    } type;
    quint32 nameId;
    qint64 timeOffset;
  };

  class MarketData
  {
  public:
    quint32 tradesTableId;
    quint32 tickerTableId;

  public:
    MarketData() : tradesTableId(0), tickerTableId(0) {}
  };

  class BrokerData
  {
  public:
    quint32 brokerTableId;
    quint32 balanceTableId;
    quint32 ordersTableId;
    quint32 transactionsTableId;
    quint32 logTableId;

  public:
    BrokerData() : brokerTableId(0), balanceTableId(0), ordersTableId(0), transactionsTableId(0), logTableId(0) {}
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
    SessionData() : sessionTableId(0), ordersTableId(0), transactionsTableId(0), assetsTableId(0), logTableId(0), propertiesTableId(0) {}
  };

private:
  zlimdb* zdb;
  QString error;
  Callback* callback;
  QHash<quint32, TableInfo> tableInfo;
  quint32 userTableId;
  QString userTablesPrefix;
  QString brokerTablesPrefix;
  QString sessionTablesPrefix;
  //quint32 lastBrokerId;
  //quint32 lastSessionId;
  QHash<QString, MarketData> marketData;
  QHash<quint32, MarketData*> marketDataById;
  QHash<quint32, BrokerData> brokerData;
  QHash<quint32, SessionData> sessionData;
  quint32 selectedBrokerId;
  quint32 selectedSessionId;

private:
  QString getZlimDbError();

  void zlimdbCallback(const zlimdb_header& message);

  //bool removeTable(quint32 id);
  //bool controlEntity(quint32 tableId, quint64 entityId, quint32 code);

  bool subscribe(quint32 tableId, TableInfo::Type tableType, zlimdb_query_type queryType = zlimdb_query_type_all);
  bool unsubscribe(quint32 tableId);

  void addedEntity(uint32_t tableId, const zlimdb_entity& entity);
  void updatedEntity(uint32_t tableId, const zlimdb_entity& entity);
  void receivedEntity(uint32_t tableId, const zlimdb_entity& entity);
  void removedEntity(uint32_t tableId, uint64_t entityId);

  bool addedTable(const zlimdb_table_entity& table);
  void removedTable(uint32_t tableId);

private:
  static void zlimdbCallback(void* user_data, const zlimdb_header* message) {((DataConnection*)user_data)->zlimdbCallback(*message);}

  static bool getString(const zlimdb_entity& entity, size_t offset, size_t length, QString& result)
  {
    if(!length || offset + length > entity.size)
      return false;
    char* str = (char*)&entity + offset;
    if(str[--length])
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

  static bool copyString(const QByteArray& str, zlimdb_entity& entity, uint16_t& length, size_t maxSize)
  {
    length = (uint16_t)str.length() + 1;
    if((size_t)entity.size + length > maxSize)
      return false;
    qMemCopy((char*)&entity + entity.size, str.constData(), length);
    entity.size += length;
    return true;
  }

  static bool copyString(const QByteArray& str, void* data, size_t& size, uint16_t& length, size_t maxSize)
  {
    length = (uint16_t)str.length() + 1;
    if(size + length > maxSize)
      return false;
    qMemCopy((char*)data + size, str.constData(), length);
    size += length;
    return true;
  }
};
