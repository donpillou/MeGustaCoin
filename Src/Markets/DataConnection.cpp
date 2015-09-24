
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
  brokerPrefix = "users/" + userName + "/brokers/";
  sessionPrefix = "users/" + userName + "/sessions/";
  this->callback = &callback;

  // subscribe to tables table
  if(zlimdb_subscribe(zdb, zlimdb_table_tables, zlimdb_query_type_all, 0) != 0)
    return error = getZlimDbError(), false;
  TableInfo& tableInfo = this->tableInfo[zlimdb_table_tables];
  tableInfo.type = TableInfo::tablesTable;
  QList<QByteArray> tables;
  {
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    uint32_t size = sizeof(buffer);
    QString tableName;
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const zlimdb_table_entity* table; table = (const zlimdb_table_entity*)zlimdb_get_entity(sizeof(zlimdb_table_entity), &data, &size);)
      {
        tables.append(QByteArray());
        tables.back().append((const char*)table, table->entity.size);
      }
    if(zlimdb_errno() != 0)
      return error = getZlimDbError(), false;
  }
  for(QList<QByteArray>::ConstIterator i = tables.begin(), end = tables.end(); i != end; ++i)
    if(!addedTable(*(const zlimdb_table_entity*)i->constData()))
      return false;
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
  lastBrokerId = 0;
  lastSessionId = 0;
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
  else
    return QString(zlimdb_strerror(err)) + ".";
}

void DataConnection::zlimdbCallback(const zlimdb_header& message)
{
  switch(message.message_type)
  {
  case zlimdb_message_add_request:
    if(message.size >= sizeof(zlimdb_add_request) + sizeof(zlimdb_entity))
    {
      const zlimdb_add_request* addRequest = (zlimdb_add_request*)&message;
      const zlimdb_entity* entity = (const zlimdb_entity*)(addRequest + 1);
      if(sizeof(zlimdb_add_request) + entity->size <= message.size)
        addedEntity(addRequest->table_id, *entity);
    }
    break;
  case zlimdb_message_update_request:
    if(message.size >= sizeof(zlimdb_update_request) + sizeof(zlimdb_entity))
    {
      const zlimdb_update_request* updateRequest = (zlimdb_update_request*)&message;
      const zlimdb_entity* entity = (const zlimdb_entity*)(updateRequest + 1);
      if(sizeof(zlimdb_add_request) + entity->size <= message.size)
        updatedEntity(updateRequest->table_id, *entity);
    }
    break;
  case zlimdb_message_remove_request:
    if(message.size >= sizeof(zlimdb_remove_request))
    {
      const zlimdb_remove_request* removeRequest = (zlimdb_remove_request*)&message;
      removedEntity(removeRequest->table_id, removeRequest->id);
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
  case TableInfo::tradesTable:
    if(entity.size >= sizeof(meguco_trade_entity))
      callback->receivedTrade(tableId, *(const meguco_trade_entity*)&entity, tableInfo.timeOffset);
    break;
  case TableInfo::brokerTable:
    if(entity.id == 1 && entity.size >= sizeof(meguco_user_broker_entity))
      callback->receivedBroker(tableInfo.nameId, *(const meguco_user_broker_entity*)&entity);
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
  // todo: add handlers for the other entity types
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
    if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0) != 0)
      return error = getZlimDbError(), false;
    TableInfo& tableInfo = this->tableInfo[table.entity.id];
    tableInfo.type = TableInfo::processesTable;
    char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
    uint32_t size = sizeof(buffer);
    QString cmd;
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_process_entity* process; process = (const meguco_process_entity*)zlimdb_get_entity(sizeof(meguco_process_entity), &data, &size);)
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
    uint32_t size = sizeof(buffer);
    QString name;
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_broker_type_entity* brokerType; brokerType = (const meguco_broker_type_entity*)zlimdb_get_entity(sizeof(meguco_broker_type_entity), &data, &size);)
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
    uint32_t size = sizeof(buffer);
    QString name;
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_bot_type_entity* botType; botType = (const meguco_bot_type_entity*)zlimdb_get_entity(sizeof(meguco_bot_type_entity), &data, &size);)
      {
        if(!getString(botType->entity, sizeof(*botType), botType->name_size, name))
          return zlimdb_seterrno(zlimdb_local_error_invalid_message_data), error = getZlimDbError(), false;
        callback->receivedBotType(*botType, name);
      }
  }
  else if(tableName.startsWith("markets/") && tableName.endsWith("/trades"))
    callback->receivedMarket(table.entity.id, tableName.mid(8, tableName.length() - (8 + 7)));
  else if(tableName.startsWith(brokerPrefix))
  {
    quint64 brokerId = tableName.mid(brokerPrefix.length(), tableName.lastIndexOf('/') - brokerPrefix.length()).toULongLong();
    if(brokerId > lastBrokerId)
      lastBrokerId = brokerId;
    BrokerData& brokerData = this->brokerData[brokerId];
    if(tableName.endsWith("/broker"))
    {
      brokerData.brokerTableId = table.entity.id;
      TableInfo& tableInfo = this->tableInfo[table.entity.id];
      tableInfo.type = TableInfo::brokerTable;
      tableInfo.nameId = brokerId;
      if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0) != 0)
        return error = getZlimDbError(), false;
      char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
      uint32_t size = sizeof(buffer);
      for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
        for(const meguco_user_broker_entity* broker; broker = (const meguco_user_broker_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_entity), &data, &size);)
        {
          if(broker->entity.id != 1)
            continue;
          callback->receivedBroker(brokerId, *broker);
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
  }
  else if(tableName.startsWith(sessionPrefix))
  {
    quint64 sessionId = tableName.mid(sessionPrefix.length(), tableName.lastIndexOf('/') - sessionPrefix.length()).toULongLong();
    if(sessionId > lastSessionId)
      lastSessionId = sessionId;
    SessionData& sessionData = this->sessionData[sessionId];
    if(tableName.endsWith("/session"))
    {
      sessionData.sessionTableId = table.entity.id;
      TableInfo& tableInfo = this->tableInfo[table.entity.id];
      tableInfo.type = TableInfo::sessionTable;
      tableInfo.nameId = sessionId;
      if(zlimdb_subscribe(zdb, table.entity.id, zlimdb_query_type_all, 0) != 0)
        return error = getZlimDbError(), false;
      char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
      uint32_t size = sizeof(buffer);
      for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
        for(const meguco_user_session_entity* session; session = (const meguco_user_session_entity*)zlimdb_get_entity(sizeof(meguco_user_session_entity), &data, &size);)
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
  default:
    break;
  }
}

bool DataConnection::subscribe(quint32 tableId, quint64 lastReceivedTradeId)
{
  qint64 tableTime, timeOffset;
  {
    qint64 serverTime;
    if(zlimdb_sync(zdb, tableId, &serverTime, &tableTime) != 0)
      return error = getZlimDbError(), false;
    timeOffset = QDateTime::currentMSecsSinceEpoch() - tableTime;
  }

  if(lastReceivedTradeId != 0)
  {
    if(zlimdb_subscribe(zdb, tableId, zlimdb_query_type_since_id, lastReceivedTradeId) != 0)
      return error = getZlimDbError(), false;
  }
  else
  {
    if(zlimdb_subscribe(zdb, tableId, zlimdb_query_type_since_time, tableTime - 7ULL * 24ULL * 60ULL * 60ULL * 1000ULL) != 0)
      return error = getZlimDbError(), false;
  }

  TableInfo& tableInfo = this->tableInfo[tableId];
  tableInfo.type = TableInfo::tradesTable;
  tableInfo.timeOffset = timeOffset;

  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  uint32_t size = sizeof(buffer);
  for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
    for(const meguco_trade_entity* trade; trade = (const meguco_trade_entity*)zlimdb_get_entity(sizeof(meguco_trade_entity), &data, &size);)
      callback->receivedTrade(tableId, *trade, timeOffset);
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

bool DataConnection::createBroker(quint64 brokerTypeId, const QString& userName, const QString& key, const QString& secret)
{
  ++lastBrokerId;
  BrokerData& brokerData = this->brokerData[lastBrokerId];
  QString tableName = QString("%1%2/broker").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.brokerTableId) != 0)
    return error = getZlimDbError(), false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_broker_entity* broker = (meguco_user_broker_entity*)buffer;
  QByteArray userNameData = userName.toUtf8(), keyData = key.toUtf8(), secretData = secret.toUtf8();
  setEntityHeader(broker->entity, 0, 0, sizeof(*broker) + userNameData.size() + keyData.size() + secretData.size());
  broker->broker_type_id = brokerTypeId;
  setString(broker->entity, broker->user_name_size, sizeof(*broker), userNameData);
  setString(broker->entity, broker->key_size, sizeof(*broker) + userNameData.size(), keyData);
  setString(broker->entity, broker->secret_size, sizeof(*broker) + userNameData.size() + keyData.size(), secretData);
  uint64_t id;
  if(zlimdb_add(zdb, brokerData.brokerTableId, &broker->entity, &id))
    return error = getZlimDbError(), false;

  tableName = QString("%1%2/balance").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.balanceTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/orders").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/transactions").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  //tableName = QString("%1%2/assets").arg(brokerPrefix, QString::number(lastBrokerId));
  //if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.assetsTableId) != 0)
  //  return error = getZlimDbError(), false;
  //tableName = QString("%1%2/log").arg(brokerPrefix, QString::number(lastBrokerId));
  //if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &brokerData.logTableId) != 0)
  //  return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::removeBroker(quint32 brokerId)
{
  QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(brokerId);
  if(it == this->brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(brokerData.brokerTableId != 0 && zlimdb_remove_table(zdb, brokerData.brokerTableId) != 0)
    return error = getZlimDbError(), false;
  if(brokerData.balanceTableId != 0 && zlimdb_remove_table(zdb, brokerData.balanceTableId) != 0)
    return error = getZlimDbError(), false;
  if(brokerData.ordersTableId != 0 && zlimdb_remove_table(zdb, brokerData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  if(brokerData.transactionsTableId != 0 && zlimdb_remove_table(zdb, brokerData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  this->brokerData.erase(it);
  if(selectedBrokerId == brokerId)
    selectedBrokerId = 0;
  return true;
}

bool DataConnection::subscribe(quint32 tableId, TableInfo::Type type)
{
  if(zlimdb_subscribe(zdb, tableId, zlimdb_query_type_all, 0) != 0)
    return error = getZlimDbError(), false;

  TableInfo& tableInfo = this->tableInfo[tableId];
  tableInfo.type = type;

  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  uint32_t size = sizeof(buffer);
  switch(type)
  {
  case TableInfo::brokerBalanceTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_broker_balance_entity* balance; balance = (const meguco_user_broker_balance_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_balance_entity), &data, &size);)
        callback->receivedBrokerBalance(*balance);
    break;
  case TableInfo::brokerOrdersTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_broker_order_entity* order; order = (const meguco_user_broker_order_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_order_entity), &data, &size);)
        callback->receivedBrokerOrder(*order);
    break;
  case TableInfo::brokerTransactionsTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_broker_transaction_entity* transaction; transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_transaction_entity), &data, &size);)
        callback->receivedBrokerTransaction(*transaction);
    break;
  case TableInfo::sessionOrdersTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_broker_order_entity* order; order = (const meguco_user_broker_order_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_order_entity), &data, &size);)
        callback->receivedSessionOrder(*order);
    break;
  case TableInfo::sessionTransactionsTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_broker_transaction_entity* transaction; transaction = (const meguco_user_broker_transaction_entity*)zlimdb_get_entity(sizeof(meguco_user_broker_transaction_entity), &data, &size);)
        callback->receivedSessionTransaction(*transaction);
    break;
  case TableInfo::sessionAssetsTable:
    for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
      for(const meguco_user_session_asset_entity* asset; asset = (const meguco_user_session_asset_entity*)zlimdb_get_entity(sizeof(meguco_user_session_asset_entity), &data, &size);)
        callback->receivedSessionAsset(*asset);
    break;
  case TableInfo::sessionLogTable:
    {
      QString message;
      for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
        for(const meguco_log_entity* log; log = (const meguco_log_entity*)zlimdb_get_entity(sizeof(meguco_log_entity), &data, &size);)
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
      for(void* data; zlimdb_get_response(zdb, (zlimdb_entity*)(data = buffer), &size) == 0; size = sizeof(buffer))
        for(const meguco_user_session_property_entity* property; property = (const meguco_user_session_property_entity*)zlimdb_get_entity(sizeof(meguco_user_session_property_entity), &data, &size);)
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
  default:
    Q_ASSERT(false);
    break;
  }
  if(zlimdb_errno() != 0)
    return error = getZlimDbError(), false;
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
  }
  return true;
}

bool DataConnection::controlBroker(meguco_user_broker_control_code code)
{
  QHash<quint32, BrokerData>::Iterator it = this->brokerData.find(this->selectedBrokerId);
  if(it == this->brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.brokerTableId)
    return error = "Unknown broker table id", false;
  if(zlimdb_control(zdb, brokerData.brokerTableId, 1, code, 0, 0) != 0)
      return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::createBrokerOrder(meguco_user_broker_order_type type, double price, double amount)
{
  QHash<quint32, BrokerData>::ConstIterator it = brokerData.find(this->selectedBrokerId);
  if(it == brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.ordersTableId)
    return error = "Unknown broker orders table id", false;

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

  quint64 id;
  if(zlimdb_add(zdb, brokerData.ordersTableId, &order.entity, &id) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::controlBrokerOrder(quint64 orderId, meguco_user_broker_order_control_code code, const void* data, size_t size)
{
  QHash<quint32, BrokerData>::ConstIterator it = brokerData.find(this->selectedBrokerId);
  if(it == brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.ordersTableId)
    return error = "Unknown broker orders table id", false;
  if(zlimdb_control(zdb, brokerData.ordersTableId, orderId, code, data, size) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::removeBrokerOrder(quint64 orderId)
{
  QHash<quint32, BrokerData>::ConstIterator it = brokerData.find(this->selectedBrokerId);
  if(it == brokerData.end())
    return error = "Unknown broker id", false;
  const BrokerData& brokerData = *it;
  if(!brokerData.ordersTableId)
    return error = "Unknown broker orders table id", false;
  if(zlimdb_remove(zdb, brokerData.ordersTableId, orderId) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::createSession(const QString& name, quint64 botTypeId, quint64 brokerId)
{
  ++lastSessionId;
  SessionData& sessionData = this->sessionData[lastSessionId];
  QString tableName = QString("%1%2/session").arg(sessionPrefix, QString::number(lastSessionId));
  quint32 tableId;
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &tableId) != 0)
    return error = getZlimDbError(), false;
  char buffer[ZLIMDB_MAX_MESSAGE_SIZE];
  meguco_user_session_entity* session = (meguco_user_session_entity*)buffer;
  QByteArray nameData = name.toUtf8();
  setEntityHeader(session->entity, 0, 0, sizeof(*session));
  setString(session->entity, session->name_size, sizeof(*session), nameData);
  session->bot_type_id = botTypeId;
  session->broker_id = brokerId;
  session->mode = meguco_user_session_none;
  session->state = meguco_user_session_stopped;
  uint64_t id;
  if(zlimdb_add(zdb, tableId, &session->entity, &id))
    return error = getZlimDbError(), false;

  tableName = QString("%1%2/orders").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/transactions").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/assets").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.assetsTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/log").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.logTableId) != 0)
    return error = getZlimDbError(), false;
  tableName = QString("%1%2/properties").arg(brokerPrefix, QString::number(lastBrokerId));
  if(zlimdb_add_table(zdb, tableName.toUtf8().constData(), &sessionData.propertiesTableId) != 0)
    return error = getZlimDbError(), false;

  return true;
}

bool DataConnection::removeSession(quint32 sessionId)
{
  QHash<quint32, SessionData>::Iterator it = this->sessionData.find(sessionId);
  if(it == this->sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(sessionData.sessionTableId != 0 && zlimdb_remove_table(zdb, sessionData.sessionTableId) != 0)
    return error = getZlimDbError(), false;
  if(sessionData.ordersTableId != 0 && zlimdb_remove_table(zdb, sessionData.ordersTableId) != 0)
    return error = getZlimDbError(), false;
  if(sessionData.transactionsTableId != 0 && zlimdb_remove_table(zdb, sessionData.transactionsTableId) != 0)
    return error = getZlimDbError(), false;
  if(sessionData.assetsTableId != 0 && zlimdb_remove_table(zdb, sessionData.assetsTableId) != 0)
    return error = getZlimDbError(), false;
  if(sessionData.logTableId != 0 && zlimdb_remove_table(zdb, sessionData.logTableId) != 0)
    return error = getZlimDbError(), false;
  if(sessionData.propertiesTableId != 0 && zlimdb_remove_table(zdb, sessionData.propertiesTableId) != 0)
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
  }
  return true;
}

bool DataConnection::controlSession(quint32 sessionId, meguco_user_session_control_code code)
{
  QHash<quint32, SessionData>::Iterator it = this->sessionData.find(this->selectedSessionId);
  if(it == this->sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.sessionTableId)
    return error = "Unknown session table id", false;
  if(zlimdb_control(zdb, sessionData.sessionTableId, 1, code, 0, 0) != 0)
      return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::createSessionAsset(meguco_user_session_asset_type type, double balanceComm, double balanceBase, double flipPrice)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.assetsTableId)
    return error = "Unknown session assets table id", false;

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

  quint64 id;
  if(zlimdb_add(zdb, sessionData.assetsTableId, &asset.entity, &id) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::controlSessionAsset(quint64 assetId, meguco_user_session_asset_control_code code, const void* data, size_t size)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.assetsTableId)
    return error = "Unknown session assets table id", false;
  if(zlimdb_control(zdb, sessionData.assetsTableId, assetId, code, data, size) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::removeSessionAsset(quint64 assetId)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.assetsTableId)
    return error = "Unknown session assets table id", false;
  if(zlimdb_remove(zdb, sessionData.assetsTableId, assetId) != 0)
    return error = getZlimDbError(), false;
  return true;
}

bool DataConnection::controlSessionProperty(quint64 propertyId, meguco_user_session_property_control_code code, const void* data, size_t size)
{
  QHash<quint32, SessionData>::ConstIterator it = sessionData.find(this->selectedSessionId);
  if(it == sessionData.end())
    return error = "Unknown session id", false;
  const SessionData& sessionData = *it;
  if(!sessionData.propertiesTableId)
    return error = "Unknown session properties table id", false;
  if(zlimdb_control(zdb, sessionData.propertiesTableId, propertyId, code, data, size) != 0)
    return error = getZlimDbError(), false;
  return true;
}
