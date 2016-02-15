
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
  userTablesPrefix = "users/" + userName;
  brokerTablesPrefix = "users/" + userName + "/brokers/";
  sessionTablesPrefix = "users/" + userName + "/sessions/";
  this->callback = &callback;

  // subscribe to tables table
  if(zlimdb_subscribe(zdb, zlimdb_table_tables, zlimdb_query_type_all, 0, zlimdb_subscribe_flag_none) != 0)
    return error = getZlimDbError(), false;
  TableInfo& tableInfo = this->tableInfo[zlimdb_table_tables];
  tableInfo.type = TableInfo::tablesTable;
  {
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    QString tableName;
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const zlimdb_table_entity* table = (const zlimdb_table_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(zlimdb_table_entity));
          table;
          table = (const zlimdb_table_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer, sizeof(zlimdb_table_entity), &table->entity))
        if(!addedTable(*table))
          return false;
    if(zlimdb_errno() != 0)
      return error = getZlimDbError(), false;
  }
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
  tableInfo.clear();
  userTableId = 0;
  marketData.clear();
  marketDataById.clear();
  brokerData.clear();
  sessionData.clear();
  selectedBrokerId = 0;
  selectedSessionId = 0;
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
    Q_ASSERT(len <= 256);
    while(len > 0 && isspace(errorMessage[len - 1]))
      --len;
    errorMessage[len] = '\0';
    return QString(errorMessage);
#else
    return QString(strerror(errno)) + ".";
#endif
  }
  return QString(zlimdb_strerror(err)) + ".";
}

void DataConnection::zlimdbCallback(const zlimdb_header& message)
{
  switch(message.message_type)
  {
  case zlimdb_message_add_request:
    if(message.size >= sizeof(zlimdb_add_request) + sizeof(zlimdb_entity))
    {
      const zlimdb_add_request* addRequest = (const zlimdb_add_request*)&message;
      const zlimdb_entity* entity = (const zlimdb_entity*)(addRequest + 1);
      if(sizeof(zlimdb_add_request) + entity->size <= message.size)
        addedEntity(addRequest->table_id, *entity);
    }
    break;
  case zlimdb_message_update_request:
    if(message.size >= sizeof(zlimdb_update_request) + sizeof(zlimdb_entity))
    {
      const zlimdb_update_request* updateRequest = (const zlimdb_update_request*)&message;
      const zlimdb_entity* entity = (const zlimdb_entity*)(updateRequest + 1);
      if(sizeof(zlimdb_add_request) + entity->size <= message.size)
        updatedEntity(updateRequest->table_id, *entity);
    }
    break;
  case zlimdb_message_remove_request:
    if(message.size >= sizeof(zlimdb_remove_request))
    {
      const zlimdb_remove_request* removeRequest = (const zlimdb_remove_request*)&message;
      removedEntity(removeRequest->table_id, removeRequest->id);
    }
    break;
  case zlimdb_message_reload_request:
    if(message.size >= sizeof(zlimdb_reload_request))
    {
      const zlimdb_reload_request* reloadRequest = (const zlimdb_reload_request*)&message;
      reloadedTable(reloadRequest->table_id);
    }
    break;
  default:
    break;
  }
}

void DataConnection::addedEntity(uint32_t tableId, const zlimdb_entity& entity)
{
  if(tableId == zlimdb_table_tables)
  {
    if(entity.size >= sizeof(zlimdb_table_entity))
      addedTable(*(const zlimdb_table_entity*)&entity);
  }
  else
    receivedEntity(tableId, entity);
}

void DataConnection::updatedEntity(uint32_t tableId, const zlimdb_entity& entity)
{
  receivedEntity(tableId, entity);
}

void DataConnection::receivedEntity(uint32_t tableId, const zlimdb_entity& entity)
{
  QHash<quint32, TableInfo>::ConstIterator it = tableInfo.find(tableId);
  if(it == tableInfo.end())
    return;
  const TableInfo& tableInfo = it.value();
  switch(tableInfo.type)
  {
  case TableInfo::processesTable:
    if(entity.size >= sizeof(meguco_process_entity))
    {
      const meguco_process_entity* process = (const meguco_process_entity*)&entity;
      QString cmd;
      if(getString(process->entity, sizeof(*process), process->cmd_size, cmd))
        callback->receivedProcess(*process, cmd);
    }
    break;
  case TableInfo::marketTradesTable:
    if(entity.size >= sizeof(meguco_trade_entity))
      callback->receivedMarketTrade(tableInfo.nameId, *(const meguco_trade_entity*)&entity, tableInfo.timeOffset);
    break;
  case TableInfo::brokerTable:
    if(entity.id == 1 && entity.size >= sizeof(meguco_user_broker_entity))
    {
      const meguco_user_broker_entity* broker = (const meguco_user_broker_entity*)&entity;
      QString userName;
      if(getString(broker->entity, sizeof(*broker), broker->user_name_size, userName))
        callback->receivedBroker(tableInfo.nameId, *broker, userName);
    }
    break;
  case TableInfo::sessionTable:
    if(entity.id == 1 && entity.size >= sizeof(meguco_user_session_entity))
    {
      const meguco_user_session_entity* session = (const meguco_user_session_entity*)&entity;
      QString name;
      if(getString(session->entity, sizeof(*session), session->name_size, name))
        callback->receivedSession(tableInfo.nameId, name, *session);
    }
    break;
  case TableInfo::brokerBalanceTable:
    if(entity.size >= sizeof(meguco_user_broker_balance_entity))
      callback->receivedBrokerBalance(*(const meguco_user_broker_balance_entity*)&entity);
    break;
  case TableInfo::brokerOrdersTable:
    if(entity.size >= sizeof(meguco_user_broker_order_entity))
      callback->receivedBrokerOrder(*(const meguco_user_broker_order_entity*)&entity);
    break;
  case TableInfo::brokerTransactionsTable:
    if(entity.size >= sizeof(meguco_user_broker_transaction_entity))
      callback->receivedBrokerTransaction(*(const meguco_user_broker_transaction_entity*)&entity);
    break;
  case TableInfo::brokerLogTable:
    if(entity.size >= sizeof(meguco_log_entity))
    {
      const meguco_log_entity* logMessage = (const meguco_log_entity*)&entity;
      QString message;
      if(getString(logMessage->entity, sizeof(*logMessage), logMessage->message_size, message))
        callback->receivedBrokerLog(*(const meguco_log_entity*)&entity, message);
    }
    break;
  case TableInfo::sessionOrdersTable:
    if(entity.size >= sizeof(meguco_user_broker_order_entity))
      callback->receivedSessionOrder(*(const meguco_user_broker_order_entity*)&entity);
    break;
  case TableInfo::sessionTransactionsTable:
    if(entity.size >= sizeof(meguco_user_broker_transaction_entity))
      callback->receivedSessionTransaction(*(const meguco_user_broker_transaction_entity*)&entity);
    break;
  case TableInfo::sessionAssetsTable:
    if(entity.size >= sizeof(meguco_user_session_asset_entity))
      callback->receivedSessionAsset(*(const meguco_user_session_asset_entity*)&entity);
    break;
  case TableInfo::sessionLogTable:
    if(entity.size >= sizeof(meguco_log_entity))
    {
      const meguco_log_entity* logMessage = (const meguco_log_entity*)&entity;
      QString message;
      if(getString(logMessage->entity, sizeof(*logMessage), logMessage->message_size, message))
        callback->receivedSessionLog(*logMessage, message);
    }
    break;
  case TableInfo::sessionPropertiesTable:
    if(entity.size >= sizeof(meguco_user_session_property_entity))
    {
      const meguco_user_session_property_entity* property = (const meguco_user_session_property_entity*)&entity;
      QString name, value, unit;
        if(getString(property->entity, sizeof(*property), property->name_size, name))
          if(getString(property->entity, sizeof(*property) + property->name_size, property->value_size, value))
            if(getString(property->entity, sizeof(*property) + property->name_size + property->value_size, property->unit_size, unit))
              callback->receivedSessionProperty(*property, name, value, unit);
    }
    break;
  case TableInfo::sessionMarkersTable:
    if(entity.size >= sizeof(meguco_user_session_marker_entity))
    {
      const meguco_user_session_marker_entity* marker = (const meguco_user_session_marker_entity*)&entity;
      callback->receivedSessionMarker(*marker);
    }
    break;
  default:
    break;
  }
}

void DataConnection::removedEntity(uint32_t tableId, uint64_t entityId)
{
  QHash<quint32, TableInfo>::ConstIterator it = tableInfo.find(tableId);
  if(it == tableInfo.end())
    return;
  const TableInfo& tableInfo = it.value();
  switch(tableInfo.type)
  {
  case TableInfo::tablesTable:
    removedTable(entityId);
    break;
  case TableInfo::processesTable:
    callback->removedProcess(entityId);
    break;
  case TableInfo::marketTradesTable:
  case TableInfo::brokerTable:
  case TableInfo::sessionTable:
    // not removable
    break;
  case TableInfo::brokerBalanceTable:
    callback->removedBrokerBalance(entityId);
    break;
  case TableInfo::brokerOrdersTable:
    callback->removedBrokerOrder(entityId);
    break;
  case TableInfo::brokerTransactionsTable:
    callback->removedBrokerTransaction(entityId);
    break;
  case TableInfo::brokerLogTable:
    callback->removedBrokerLog(entityId);
    break;
  case TableInfo::sessionOrdersTable:
    callback->removedSessionOrder(entityId);
    break;
  case TableInfo::sessionTransactionsTable:
    callback->removedSessionTransaction(entityId);
    break;
  case TableInfo::sessionAssetsTable:
    callback->removedSessionAsset(entityId);
    break;
  case TableInfo::sessionPropertiesTable:
    callback->removedSessionProperty(entityId);
    break;
  case TableInfo::sessionLogTable:
    callback->removedSessionLog(entityId);
    break;
  case TableInfo::sessionMarkersTable:
    callback->removedSessionMarker(entityId);
    break;
  default:
    break;
  }
}

void DataConnection::reloadedTable(uint32_t tableId)
{
  QHash<quint32, TableInfo>::ConstIterator it = tableInfo.find(tableId);
  if(it == tableInfo.end())
    return;
  const TableInfo& tableInfo = it.value();
  switch(tableInfo.type)
  {
  case TableInfo::sessionOrdersTable:
    callback->clearSessionOrders();
    query(tableId, tableInfo.type);
    break;
  case TableInfo::sessionTransactionsTable:
    callback->clearSessionTransactions();
    query(tableId, tableInfo.type);
    break;
  case TableInfo::sessionAssetsTable:
    callback->clearSessionAssets();
    query(tableId, tableInfo.type);
    break;
  case TableInfo::sessionPropertiesTable:
    callback->clearSessionProperties();
    query(tableId, tableInfo.type);
    break;
  case TableInfo::sessionLogTable:
    callback->clearSessionLog();
    query(tableId, tableInfo.type);
    break;
  case TableInfo::sessionMarkersTable:
    callback->clearSessionMarkers();
    query(tableId, tableInfo.type);
    break;
  default:
    break;
  }
}

bool DataConnection::addedTable(const zlimdb_table_entity& table)
{
  QString tableName;
  if(!getString(table.entity, sizeof(table), table.name_size, tableName))
    return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
  if(tableName == "processes")
  {
    if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0, zlimdb_subscribe_flag_none) != 0)
      return error = getZlimDbError(), false;
    TableInfo& tableInfo = this->tableInfo[table.entity.id];
    tableInfo.type = TableInfo::processesTable;
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    QString cmd;
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_process_entity* process = (const meguco_process_entity*)zlimdb_get_first_entity(
        (zlimdb_header*)buffer, sizeof(meguco_process_entity));
          process;
          process = (const meguco_process_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,
                                                                         sizeof(meguco_process_entity),
                                                                         &process->entity))
      {
        if(!getString(process->entity, sizeof(*process), process->cmd_size, cmd))
          return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
        callback->receivedProcess(*process, cmd);
      }
  }
  else if(tableName == "brokers")
  {
    if(zlimdb_query(zdb, table.entity.id, zlimdb_query_type_all, 0) != 0)
      return error = getZlimDbError(), false;
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    QString name;
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_broker_type_entity* brokerType = (const meguco_broker_type_entity*)zlimdb_get_first_entity(
        (zlimdb_header*)buffer, sizeof(meguco_broker_type_entity));
          brokerType;
          brokerType = (const meguco_broker_type_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,
                                                                                sizeof(meguco_broker_type_entity),
                                                                                &brokerType->entity))
      {
        if(!getString(brokerType->entity, sizeof(*brokerType), brokerType->name_size, name))
          return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
        callback->receivedBrokerType(*brokerType, name);
      }
  }
  else if(tableName == "bots")
  {
    if(zlimdb_query(zdb, table.entity.id, zlimdb_query_type_all, 0) != 0)
      return error = getZlimDbError(), false;
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    QString name;
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_bot_type_entity* botType = (const meguco_bot_type_entity*)zlimdb_get_first_entity(
        (zlimdb_header*)buffer, sizeof(meguco_bot_type_entity));
          botType;
          botType = (const meguco_bot_type_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,
                                                                          sizeof(meguco_bot_type_entity),
                                                                          &botType->entity))
      {
        if(!getString(botType->entity, sizeof(*botType), botType->name_size, name))
          return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
        callback->receivedBotType(*botType, name);
      }
  }
  else if(tableName.startsWith("markets/"))
  {
    QString marketId = tableName.mid(8, tableName.lastIndexOf('/') - 8);
    MarketData& marketData = this->marketData[marketId];
    if(tableName.endsWith("/trades"))
    {
      marketData.tradesTableId = table.entity.id;
      marketDataById[table.entity.id] = &marketData;
      callback->receivedMarket(table.entity.id, marketId);
    }
    else if(tableName.endsWith("/ticker"))
      marketData.tickerTableId = table.entity.id;
  }
  else if(tableName.startsWith(userTablesPrefix))
  {
    if(tableName.startsWith(brokerTablesPrefix))
    {
      quint64 brokerId = tableName.mid(brokerTablesPrefix.length(),
                                       tableName.lastIndexOf('/') - brokerTablesPrefix.length()).toULongLong();
      BrokerData& brokerData = this->brokerData[brokerId];
      if(tableName.endsWith("/broker"))
      {
        brokerData.brokerTableId = table.entity.id;
        TableInfo& tableInfo = this->tableInfo[table.entity.id];
        tableInfo.type = TableInfo::brokerTable;
        tableInfo.nameId = brokerId;
        if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0, zlimdb_subscribe_flag_none) != 0)
          return error = getZlimDbError(), false;
        char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
        while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
          for(const meguco_user_broker_entity* broker = (const meguco_user_broker_entity*)zlimdb_get_first_entity(
            (zlimdb_header*)buffer, sizeof(meguco_user_broker_entity));
              broker;
              broker = (const meguco_user_broker_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,
                                                                                sizeof(meguco_user_broker_entity),
                                                                                &broker->entity))
          {
            if(broker->entity.id != 1)
              continue;
            QString userName;
            if(getString(broker->entity, sizeof(*broker), broker->user_name_size, userName))
              callback->receivedBroker(brokerId, *broker, userName);
            break;
          }
        if(zlimdb_errno() != 0)
          return error = getZlimDbError(), false;
      }
      else if(tableName.endsWith("/balance"))
        brokerData.balanceTableId = table.entity.id;
      else if(tableName.endsWith("/orders"))
        brokerData.ordersTableId = table.entity.id;
      else if(tableName.endsWith("/transactions"))
        brokerData.transactionsTableId = table.entity.id;
      else if(tableName.endsWith("/log"))
        brokerData.logTableId = table.entity.id;
    }
    else if(tableName.startsWith(sessionTablesPrefix))
    {
      quint32 sessionId = tableName.mid(sessionTablesPrefix.length(),
                                        tableName.lastIndexOf('/') - sessionTablesPrefix.length()).toUInt();
      SessionData& sessionData = this->sessionData[sessionId];
      if(tableName.endsWith("/session"))
      {
        sessionData.sessionTableId = table.entity.id;
        TableInfo& tableInfo = this->tableInfo[table.entity.id];
        tableInfo.type = TableInfo::sessionTable;
        tableInfo.nameId = sessionId;
        if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0, zlimdb_subscribe_flag_none) != 0)
          return error = getZlimDbError(), false;
        char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
        while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
          for(const meguco_user_session_entity* session = (const meguco_user_session_entity*)zlimdb_get_first_entity(
            (zlimdb_header*)buffer, sizeof(meguco_user_session_entity));
              session;
              session = (const meguco_user_session_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,
                                                                                  sizeof(meguco_user_session_entity),
                                                                                  &session->entity))
          {
            if(session->entity.id != 1)
              continue;
            QString name;
            if(!getString(session->entity, sizeof(*session), session->name_size, name))
              return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
            callback->receivedSession(sessionId, name, *session);
            break;
          }
        if(zlimdb_errno() != 0)
          return error = getZlimDbError(), false;
      }
      else if(tableName.endsWith("/orders"))
        sessionData.ordersTableId = table.entity.id;
      else if(tableName.endsWith("/transactions"))
        sessionData.transactionsTableId = table.entity.id;
      else if(tableName.endsWith("/assets"))
        sessionData.assetsTableId = table.entity.id;
      else if(tableName.endsWith("/log"))
        sessionData.logTableId = table.entity.id;
      else if(tableName.endsWith("/properties"))
        sessionData.propertiesTableId = table.entity.id;
      else if(tableName.endsWith("/markers"))
        sessionData.markersTableId = table.entity.id;
    }
    else if(tableName.endsWith("/user"))
      userTableId = table.entity.id;
  }
  return true;
}

void DataConnection::removedTable(uint32_t tableId)
{
  QHash<quint32, TableInfo>::ConstIterator it = tableInfo.find(tableId);
  if(it == tableInfo.end())
    return;
  const TableInfo& tableInfo = it.value();
  switch(tableInfo.type)
  {
  case TableInfo::brokerTable:
    callback->removedBroker(tableInfo.nameId);
    break;
  case TableInfo::sessionTable:
    callback->removedSession(tableInfo.nameId);
    break;
  default:
    break;
  }
}

bool DataConnection::subscribeMarket(quint32 marketId, quint64 lastReceivedTradeId)
{
  int64_t tableTime, timeOffset;
  {
    int64_t serverTime;
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    if(zlimdb_sync(zdb, marketId, &serverTime, &tableTime) != 0)
      return error = getZlimDbError(), false;
    timeOffset = now - tableTime;
  }

  QHash<quint32, MarketData*>::ConstIterator it = marketDataById.find(marketId);
  if(it == marketDataById.end())
    return error = "Unknown market id", false;
  MarketData& marketData = *it.value();

  if(lastReceivedTradeId != 0)
  {
    if(zlimdb_subscribe(zdb, marketData.tradesTableId, zlimdb_query_type_since_id, lastReceivedTradeId, zlimdb_subscribe_flag_none) != 0)
      return error = getZlimDbError(), false;
  }
  else
  {
    if(zlimdb_subscribe(zdb, marketData.tradesTableId, zlimdb_query_type_since_time, tableTime - 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL, zlimdb_subscribe_flag_none) != 0)
      return error = getZlimDbError(), false;
  }

  TableInfo& tableInfo = this->tableInfo[marketData.tradesTableId];
  tableInfo.type = TableInfo::marketTradesTable;
  tableInfo.timeOffset = timeOffset;
  tableInfo.nameId = marketId;

  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
    for(const meguco_trade_entity* trade = (const meguco_trade_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_trade_entity));
        trade;
        trade = (const meguco_trade_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer, sizeof(meguco_trade_entity), &trade->entity))
      callback->receivedMarketTrade(marketId, *trade, timeOffset);
  if(zlimdb_errno() != 0)
    return error = getZlimDbError(), false;

  if(marketData.tickerTableId != 0)
  {
    if(zlimdb_subscribe(zdb, marketData.tickerTableId, zlimdb_query_type_since_last, 0, zlimdb_subscribe_flag_none) != 0)
      return error = getZlimDbError(), false;
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_ticker_entity* ticker = (const meguco_ticker_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_ticker_entity));
          ticker;
          ticker = (const meguco_ticker_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer, sizeof(meguco_ticker_entity), &ticker->entity))
        callback->receivedMarketTicker(marketId, *ticker);
    if(zlimdb_errno() != 0)
      return error = getZlimDbError(), false;
  }

  return true;
}

bool DataConnection::unsubscribeMarket(quint32 marketId)
{
  QHash<quint32, MarketData*>::ConstIterator it = marketDataById.find(marketId);
  if(it == marketDataById.end())
    return error = "Unknown market id", false;
  MarketData& marketData = *it.value();

  if(marketData.tradesTableId != 0)
  {
    if(zlimdb_unsubscribe(zdb, marketData.tradesTableId) != 0)
      return error = getZlimDbError(), false;
    tableInfo.remove(marketData.tradesTableId);
  }
  if(marketData.tickerTableId != 0)
  {
    if(zlimdb_unsubscribe(zdb, marketData.tickerTableId) != 0)
      return error = getZlimDbError(), false;
    tableInfo.remove(marketData.tickerTableId);
  }
  return true;
}

bool DataConnection::controlUser(quint64 entityId, meguco_user_control_code controlCode, const void* args, size_t argsSize, bool& result)
{
  if(!userTableId)
    return error = "Unknown user table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, userTableId, entityId, controlCode, args, argsSize, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE))
  {
    if(zlimdb_errno() == 0)
      return result = false, true;
    return error = getZlimDbError(), false;
  }
  return result = true, true;
}

bool DataConnection::createBroker(quint64 brokerTypeId, const QString& userName, const QString& key, const QString& secret)
{
  if(!userTableId)
    return error = "Unknown user table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_broker_entity* broker = (meguco_user_broker_entity*)buffer;
  QByteArray userNameData = userName.toUtf8(), keyData = key.toUtf8(), secretData = secret.toUtf8();
  setEntityHeader(broker->entity, 0, 0, sizeof(meguco_user_broker_entity));
  broker->broker_type_id = brokerTypeId;
  if(!copyString(userNameData, broker->entity, broker->user_name_size, ZLIMDB_MAX_ENTITY_SIZE) ||
     !copyString(keyData, broker->entity, broker->key_size, ZLIMDB_MAX_ENTITY_SIZE) ||
     !copyString(secretData, broker->entity, broker->secret_size, ZLIMDB_MAX_ENTITY_SIZE))
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_parameter);
    return error = getZlimDbError(), false;
  }

  if(zlimdb_control(zdb, userTableId, 0, meguco_user_control_create_broker, &broker->entity, broker->entity.size, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE))
    return error = getZlimDbError(), false;
  uint64_t* response = (uint64_t*)zlimdb_get_response_data((zlimdb_header*)buffer, sizeof(uint64_t));
  if(!response)
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_response);
    return error = getZlimDbError(), false;
  }
  uint64_t brokerId = *response;

  BrokerData& brokerData = this->brokerData[brokerId];
  QString tableName = QString("%1%2/broker").arg(brokerTablesPrefix, QString::number(brokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.brokerTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/balance").arg(brokerTablesPrefix, QString::number(brokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.balanceTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/orders").arg(brokerTablesPrefix, QString::number(brokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/transactions").arg(brokerTablesPrefix, QString::number(brokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/log").arg(brokerTablesPrefix, QString::number(brokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.logTableId) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::removeBroker(quint32 brokerId)
{
  QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(brokerId);
  if(it == this->brokerData.end())
    return error = "Unknown broker id", false;
  if(!userTableId)
    return error = "Unknown user table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, userTableId, brokerId, meguco_user_control_remove_broker, 0, 0, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE))
    return error = getZlimDbError(), false;
  this->brokerData.erase(it);
  if(selectedBrokerId == brokerId)
    selectedBrokerId = 0;
  return true;
}

bool DataConnection::subscribe(quint32 tableId, TableInfo::Type tableType, zlimdb_query_type queryType)
{
  if(zlimdb_subscribe(zdb, tableId, queryType, 0, zlimdb_subscribe_flag_none) != 0)
    return error = getZlimDbError(), false;

  TableInfo& tableInfo = this->tableInfo[tableId];
  tableInfo.type = tableType;

  return receiveResponse(tableType);
}

bool DataConnection::query(quint32 tableId, TableInfo::Type tableType)
{
  if(zlimdb_query(zdb, tableId, zlimdb_query_type_all, 0) != 0)
    return error = getZlimDbError(), false;
  return receiveResponse(tableType);
}

bool DataConnection::receiveResponse(TableInfo::Type tableType)
{
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  switch(tableType)
  {
  case TableInfo::brokerBalanceTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_broker_balance_entity* balance = (const meguco_user_broker_balance_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_balance_entity));
          balance;
          balance = (const meguco_user_broker_balance_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_balance_entity), &balance->entity))
        callback->receivedBrokerBalance(*balance);
    break;
  case TableInfo::brokerOrdersTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_broker_order_entity* order = (const meguco_user_broker_order_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_order_entity));
          order;
          order = (const meguco_user_broker_order_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_broker_order_entity), &order->entity))
        callback->receivedBrokerOrder(*order);
    break;
  case TableInfo::brokerTransactionsTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_broker_transaction_entity* transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_transaction_entity));
          transaction;
          transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_transaction_entity), &transaction->entity))
        callback->receivedBrokerTransaction(*transaction);
    break;
  case TableInfo::brokerLogTable:
    {
      QString message;
      while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
        for(const meguco_log_entity* log = (const meguco_log_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_log_entity));
            log;
            log = (const meguco_log_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_log_entity), &log->entity))
        {
          if(!getString(log->entity, sizeof(*log), log->message_size, message))
            return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
          callback->receivedBrokerLog(*log, message);
        }
    }
    break;
  case TableInfo::sessionOrdersTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_broker_order_entity* order = (const meguco_user_broker_order_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_order_entity));
          order;
          order = (const meguco_user_broker_order_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_broker_order_entity), &order->entity))
        callback->receivedSessionOrder(*order);
    break;
  case TableInfo::sessionTransactionsTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_broker_transaction_entity* transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_broker_transaction_entity));
          transaction;
          transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_broker_transaction_entity), &transaction->entity))
        callback->receivedSessionTransaction(*transaction);
    break;
  case TableInfo::sessionAssetsTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_session_asset_entity* asset = (const meguco_user_session_asset_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_session_asset_entity));
          asset;
          asset = (const meguco_user_session_asset_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_session_asset_entity), &asset->entity))
        callback->receivedSessionAsset(*asset);
    break;
  case TableInfo::sessionLogTable:
    {
      QString message;
      while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
        for(const meguco_log_entity* log = (const meguco_log_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_log_entity));
            log;
            log = (const meguco_log_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_log_entity), &log->entity))
        {
          if(!getString(log->entity, sizeof(*log), log->message_size, message))
            return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
          callback->receivedSessionLog(*log, message);
        }
    }
    break;
  case TableInfo::sessionPropertiesTable:
    {
      QString name, value, unit;
      while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
        for(const meguco_user_session_property_entity* property = (const meguco_user_session_property_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_session_property_entity));
            property;
            property = (const meguco_user_session_property_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_session_property_entity), &property->entity))
        {
          if(!getString(property->entity, sizeof(*property), property->name_size, name))
              return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
          if(!getString(property->entity, sizeof(*property) + property->name_size, property->value_size, value))
              return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
          if(!getString(property->entity, sizeof(*property) + property->name_size + property->value_size, property->unit_size, unit))
              return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
          callback->receivedSessionProperty(*property, name, value, unit);
        }
    }
    break;
  case TableInfo::sessionMarkersTable:
    while(zlimdb_get_response(zdb, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) == 0)
      for(const meguco_user_session_marker_entity* marker = (const meguco_user_session_marker_entity*)zlimdb_get_first_entity((zlimdb_header*)buffer, sizeof(meguco_user_session_marker_entity));
          marker;
          marker = (const meguco_user_session_marker_entity*)zlimdb_get_next_entity((zlimdb_header*)buffer,sizeof(meguco_user_session_marker_entity), &marker->entity))
        callback->receivedSessionMarker(*marker);
    break;
  default:
    Q_ASSERT(false);
    break;
  }
  if(zlimdb_errno() != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::unsubscribe(quint32 tableId)
{
  if(zlimdb_unsubscribe(zdb, tableId) != 0)
    return error = getZlimDbError(), false;
  tableInfo.remove(tableId);
  return true;
}

bool DataConnection::selectBroker(quint32 brokerId)
{
  if(this->selectedBrokerId == brokerId)
    return true;
  if(this->selectedBrokerId != 0)
  {
    QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(this->selectedBrokerId);
    if(it != this->brokerData.end())
    {
      const BrokerData& brokerData = *it;
      if(brokerData.balanceTableId != 0 && !unsubscribe(brokerData.balanceTableId))
        return false;
      if(brokerData.ordersTableId != 0 && !unsubscribe(brokerData.ordersTableId))
        return false;
      if(brokerData.transactionsTableId != 0 && !unsubscribe(brokerData.transactionsTableId))
        return false;
      if(brokerData.logTableId != 0 && !unsubscribe(brokerData.logTableId))
        return false;

      callback->clearBrokerBalance();
      callback->clearBrokerOrders();
      callback->clearBrokerTransactions();
      callback->clearBrokerLog();
    }
  }
  this->selectedBrokerId = 0;
  if(brokerId != 0)
  {
    QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(brokerId);
    if(it == this->brokerData.end())
      return error = "Unknown broker id", false;
    this->selectedBrokerId = brokerId;
    const BrokerData& brokerData = *it;
    if(brokerData.balanceTableId != 0 && !subscribe(brokerData.balanceTableId, TableInfo::brokerBalanceTable))
      return false;
    if(brokerData.ordersTableId != 0 && !subscribe(brokerData.ordersTableId, TableInfo::brokerOrdersTable))
      return false;
    if(brokerData.transactionsTableId != 0 && !subscribe(brokerData.transactionsTableId, TableInfo::brokerTransactionsTable))
      return false;
    if(brokerData.logTableId != 0 && !subscribe(brokerData.logTableId, TableInfo::brokerLogTable, zlimdb_query_type_since_next))
      return false;
  }
  return true;
}

bool DataConnection::controlBroker(quint64 entityId, meguco_user_broker_control_code controlCode, bool& result)
{
  QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(this->selectedBrokerId);
  if(it == this->brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.brokerTableId)
    return error = "Unknown broker table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, brokerData.brokerTableId, entityId, controlCode, 0, 0, (zlimdb_header*)buffer, sizeof(buffer)) != 0)
  {
    if(zlimdb_errno() == 0)
      return result = false, true;
    return error = getZlimDbError(), false;
  }
  return result = true, true;
}

bool DataConnection::createBrokerOrder(meguco_user_broker_order_type type, double price, double amount, quint64& orderId)
{
  QHash<quint32, BrokerData>::ConstIterator it = brokerData.find(this->selectedBrokerId);
  if(it == brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.brokerTableId)
    return error = "Unknown broker table id", false;

  meguco_user_broker_order_entity order;
  setEntityHeader(order.entity, 0, 0, sizeof(order));
  order.type = type;
  order.state = meguco_user_broker_order_submitting;
  order.price = price;
  order.amount = amount;
  order.total = 0;
  order.raw_id = 0;
  order.timeout = 0;
  order.raw_id = 0;

  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, brokerData.brokerTableId, 0, meguco_user_broker_control_create_order, &order.entity, order.entity.size, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) != 0)
  {
    if(zlimdb_errno() == 0)
      return orderId = 0, true;
    return error = getZlimDbError(), false;
  }
  uint64_t* response = (uint64_t*)zlimdb_get_response_data((zlimdb_header*)buffer, sizeof(uint64_t));
  if(!response)
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_response);
    return error = getZlimDbError(), false;
  }
  orderId = *response;
  return true;
}

bool DataConnection::updateBrokerOrder(quint64 orderId, double price, double amount, bool& result)
{
  QHash<quint32, BrokerData>::ConstIterator it = brokerData.find(this->selectedBrokerId);
  if(it == brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.brokerTableId)
    return error = "Unknown broker table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_broker_control_update_order_params params;
  params.price = price;
  params.amount = amount;
  params.total = 0.;
  if(zlimdb_control(zdb, brokerData.brokerTableId, orderId, meguco_user_broker_control_update_order, &params, sizeof(meguco_user_broker_control_update_order_params), (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) != 0)
  {
    if(zlimdb_errno() == 0)
      return result = false, true;
    return error = getZlimDbError(), false;
  }
  return result = true, true;
}

bool DataConnection::createSession(const QString& name, quint64 botTypeId, quint64 brokerId)
{
  if(!userTableId)
    return error = "Unknown user table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_session_entity* session = (meguco_user_session_entity*)buffer;
  QByteArray nameData = name.toUtf8();
  setEntityHeader(session->entity, 0, 0, sizeof(meguco_user_session_entity));
  if(!copyString(nameData, session->entity, session->name_size, ZLIMDB_MAX_MESSAGE_SIZE))
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_parameter);
    return error = getZlimDbError(), false;
  }
  session->bot_type_id = botTypeId;
  session->broker_id = brokerId;
  session->mode = meguco_user_session_none;
  session->state = meguco_user_session_stopped;
  if(zlimdb_control(zdb, userTableId, 0, meguco_user_control_create_session, &session->entity, session->entity.size, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE))
    return error = getZlimDbError(), false;
  uint64_t* response = (uint64_t*)zlimdb_get_response_data((zlimdb_header*)buffer, sizeof(uint64_t));
  if(!response)
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_response);
    return error = getZlimDbError(), false;
  }
  uint64_t sessionId = *response;
  SessionData& sessionData = this->sessionData[sessionId];
  QString tableName = QString("%1%2/orders").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/transactions").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/assets").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.assetsTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/log").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.logTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/properties").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.propertiesTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/markers").arg(sessionTablesPrefix, QString::number(sessionId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.markersTableId) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::removeSession(quint32 sessionId)
{
  QHash<quint32, SessionData>::Iterator it = this->sessionData.find(sessionId);
  if(it == this->sessionData.end())
    return error = "Unknown session id", false;
  if(!userTableId)
    return error = "Unknown user table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, userTableId, sessionId, meguco_user_control_remove_session, 0, 0, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE))
    return error = getZlimDbError(), false;
  this->sessionData.erase(it);
  if(selectedSessionId == sessionId)
    selectedSessionId = 0;
  return true;
}

bool DataConnection::selectSession(quint32 sessionId)
{
  if(this->selectedSessionId == sessionId)
    return true;
  if(this->selectedSessionId != 0)
  {
    QHash<quint32, SessionData>::Iterator it = this->sessionData.find(this->selectedSessionId);
    if(it != this->sessionData.end())
    {
      const SessionData& sessionData = *it;
      if(sessionData.ordersTableId != 0 && !unsubscribe(sessionData.ordersTableId))
        return false;
      if(sessionData.transactionsTableId != 0 && !unsubscribe(sessionData.transactionsTableId))
        return false;
      if(sessionData.assetsTableId != 0 && !unsubscribe(sessionData.assetsTableId))
        return false;
      if(sessionData.logTableId != 0 && !unsubscribe(sessionData.logTableId))
        return false;
      if(sessionData.propertiesTableId != 0 && !unsubscribe(sessionData.propertiesTableId))
        return false;
      if(sessionData.markersTableId != 0 && !unsubscribe(sessionData.markersTableId))
        return false;

      callback->clearSessionTransactions();
      callback->clearSessionAssets();
      callback->clearSessionProperties();
      callback->clearSessionOrders();
      callback->clearSessionLog();
      callback->clearSessionMarkers();
    }
  }
  this->selectedSessionId = 0;
  if(sessionId != 0)
  {
    QHash<quint32, SessionData>::Iterator it = this->sessionData.find(sessionId);
    if(it == this->sessionData.end())
      return error = "Unknown session id", false;
    this->selectedSessionId = sessionId;
    const SessionData& sessionData = *it;
    if(sessionData.ordersTableId != 0 && !subscribe(sessionData.ordersTableId, TableInfo::sessionOrdersTable))
      return false;
    if(sessionData.transactionsTableId != 0 && !subscribe(sessionData.transactionsTableId, TableInfo::sessionTransactionsTable))
      return false;
    if(sessionData.assetsTableId != 0 && !subscribe(sessionData.assetsTableId, TableInfo::sessionAssetsTable))
      return false;
    if(sessionData.logTableId != 0 && !subscribe(sessionData.logTableId, TableInfo::sessionLogTable))
      return false;
    if(sessionData.propertiesTableId != 0 && !subscribe(sessionData.propertiesTableId, TableInfo::sessionPropertiesTable))
      return false;
    if(sessionData.markersTableId != 0 && !subscribe(sessionData.markersTableId, TableInfo::sessionMarkersTable))
      return false;
  }
  return true;
}

bool DataConnection::controlSession(quint64 entityId, meguco_user_session_control_code controlCode, bool& result)
{
  QHash<quint32, SessionData>::Iterator it = this->sessionData.find(this->selectedSessionId);
  if(it == this->sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.sessionTableId)
    return error = "Unknown session table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, sessionData.sessionTableId, entityId, controlCode, 0, 0, (zlimdb_header*)buffer, sizeof(buffer)) != 0)
  {
    if(zlimdb_errno() == 0)
      return result = false, true;
    return error = getZlimDbError(), false;
  }
  return result = true, true;
}

bool DataConnection::createSessionAsset(meguco_user_session_asset_type type, double balanceComm, double balanceBase, double flipPrice, quint64& assetId)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.sessionTableId)
    return error = "Unknown session table id", false;

  meguco_user_session_asset_entity asset;
  setEntityHeader(asset.entity, 0, 0, sizeof(asset));
  asset.type = type;
  asset.state = meguco_user_session_asset_submitting;
  asset.price = 0.;
  asset.invest_comm = 0.;
  asset.invest_base = 0.;
  if (type == meguco_user_session_asset_buy)
  {
    asset.balance_comm = 0.;
    asset.balance_base = balanceBase;
  }
  else
  {
    asset.balance_comm = balanceComm;
    asset.balance_base = 0.;
  }
  asset.profitable_price = flipPrice;
  asset.flip_price = flipPrice;
  asset.order_id = 0;

  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  if(zlimdb_control(zdb, sessionData.sessionTableId, 0, meguco_user_session_control_create_asset, &asset.entity, asset.entity.size, (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) != 0)
  {
    if(zlimdb_errno() == 0)
      return assetId = 0, true;
    return error = getZlimDbError(), false;
  }
  uint64_t* response = (uint64_t*)zlimdb_get_response_data((zlimdb_header*)buffer, sizeof(uint64_t));
  if(!response)
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_response);
    return error = getZlimDbError(), false;
  }
  return assetId = *response, true;
}

bool DataConnection::updateSessionAsset(quint64 assetId, double flipPrice, bool& result)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.sessionTableId)
    return error = "Unknown session table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_session_control_update_asset_params params;
  params.flip_price = flipPrice;
  if(zlimdb_control(zdb, sessionData.sessionTableId, assetId, meguco_user_session_control_update_asset, &params, sizeof(meguco_user_session_control_update_asset_params), (zlimdb_header*)buffer, ZLIMDB_MAX_MESSAGE_SIZE) != 0)
  {
    if(zlimdb_errno() == 0)
      return result = false, true;
    return error = getZlimDbError(), false;
  }
  return result = true, true;
}

bool DataConnection::updateSessionProperty(quint64 propertyId, const QString& value)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.sessionTableId)
    return error = "Unknown session table id", false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_session_control_update_property_params* params = (meguco_user_session_control_update_property_params*)buffer;
  size_t size = sizeof(meguco_user_session_control_update_property_params);
  if(!copyString(value.toUtf8(), params, size, params->value_size, ZLIMDB_MAX_ENTITY_SIZE))
  {
    zlimdb_seterrno(zlimdb_local_error_invalid_parameter);
    return error = getZlimDbError(), false;
  }
  if(zlimdb_control(zdb, sessionData.sessionTableId, propertyId, meguco_user_session_control_update_property, params, size, (zlimdb_header*)buffer, sizeof(buffer)) != 0)
    return error = getZlimDbError(), false;
  return true;
}
