
#include "stdafx.h"

TradeModel::TradeModel(PublicDataModel& publicDataModel) :
  publicDataModel(publicDataModel),
  upIcon(QIcon(":/Icons/arrow_diag.png")), downIcon(QIcon(":/Icons/arrow_diag_red.png")), neutralIcon(QIcon(":/Icons/bullet_grey.png"))
{
  connect(&publicDataModel, SIGNAL(changedMarket()), this, SLOT(updateHeader()));
}

TradeModel::~TradeModel()
{
  qDeleteAll(trades);
}

void TradeModel::updateHeader()
{
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void TradeModel::reset()
{
  beginResetModel();
  qDeleteAll(trades);
  trades.clear();
  endResetModel();
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void TradeModel::clearAbove(int tradeCount)
{
  int tradesToRemove = trades.size() - tradeCount;
  if(tradesToRemove > 0)
  {
    beginRemoveRows(QModelIndex(), tradeCount, tradeCount + tradesToRemove - 1);
    for(int i = trades.size() - 1;; --i)
    {
      delete trades.back();
      trades.removeLast();
      if(i == tradeCount)
        break;
    }
    endRemoveRows();
  }
}

void TradeModel::setTrades(const QList<DataProtocol::Trade>& tradesData)
{
  reset();

  int tradesToInsert = qMin(tradesData.size(), 100);
  if(tradesToInsert == 0)
    return;

  beginInsertRows(QModelIndex(), 0, tradesToInsert - 1);
  trades.reserve(tradesToInsert);
  for(int i = 0; i < tradesToInsert; ++i)
    trades.append(new Trade);

  double lastPrice = 0.;
  for(int i = tradesData.size() - tradesToInsert, end = tradesData.size(), j = tradesToInsert - 1; i < end; ++i, --j)
  {
    const DataProtocol::Trade& tradeData = tradesData[i];
    Trade* trade = trades[j];
    trade->id = tradeData.id;
    trade->time = tradeData.time / 1000ULL;
    trade->price = tradeData.price;
    trade->amount = tradeData.amount;
    trade->icon = Trade::Icon::neutral;
    if(lastPrice != 0.)
    {
      if(trade->price > lastPrice)
        trade->icon = Trade::Icon::up;
      else if(trade->price < lastPrice)
        trade->icon = Trade::Icon::down;
    }
    lastPrice = tradeData.price;
  }
  endInsertRows();
}

void TradeModel::addTrade(const DataProtocol::Trade& tradeData)
{
  double lastPrice = trades.size() > 0 ? trades.front()->price : 0.;

  beginInsertRows(QModelIndex(), 0, 0);
  Trade* trade = new Trade;
  trade->id = tradeData.id;
  trade->time = tradeData.time / 1000ULL;
  trade->price = tradeData.price;
  trade->amount = tradeData.amount;
  trade->icon = Trade::Icon::neutral;
  if(lastPrice != 0.)
  {
    if(trade->price > lastPrice)
      trade->icon = Trade::Icon::up;
    else if(trade->price < lastPrice)
      trade->icon = Trade::Icon::down;
  }
  trades.prepend(trade);
  endInsertRows();
}

QModelIndex TradeModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex TradeModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int TradeModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : trades.size();
}

int TradeModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant TradeModel::data(const QModelIndex& index, int role) const
{
  if(!index.isValid())
    return QVariant();

  if(role == Qt::TextAlignmentRole)
    switch((Column)index.column())
    {
    case Column::price:
    case Column::amount:
      return (int)Qt::AlignRight | (int)Qt::AlignVCenter;
    default:
      return (int)Qt::AlignLeft | (int)Qt::AlignVCenter;
    }

  int row = index.row();
  if(row < 0 || row >= trades.size())
    return QVariant();
  const Trade& trade = *trades[row];

  switch(role)
  {
  case Qt::DecorationRole:
    if((Column)index.column() == Column::price)
      switch(trade.icon)
      {
      case Trade::Icon::up:
        return upIcon;
      case Trade::Icon::down:
        return downIcon;
      case Trade::Icon::neutral:
        return neutralIcon;
      }
    break;
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::date:
      {
        quint64 timeSinceTrade = QDateTime::currentDateTime().toTime_t() - trade.time;
        if(timeSinceTrade < 60)
          return QString("a few seconds ago");
        if(timeSinceTrade < 120)
          return QString("1 minute ago");
        return QString("%1 minutes ago").arg(timeSinceTrade / 60);
      }
    case Column::amount:
      return publicDataModel.formatAmount(trade.amount);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.amount, 'f', 8), market->getCoinCurrency());
      //return QString().sprintf("%.08f %s", transaction.amount, market->getCoinCurrency());
    case Column::price:
      return publicDataModel.formatPrice(trade.price);
      //return QString("%1 %2").arg(QLocale::system().toString(transaction.price, 'f', 2), market->getMarketCurrency());
      //return QString().sprintf("%.02f %s", transaction.price, market->getMarketCurrency());
    }
  }
  return QVariant();
}

QVariant TradeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
  if(orientation != Qt::Horizontal)
    return QVariant();
  switch(role)
  {
  case Qt::TextAlignmentRole:
    switch((Column)section)
    {
    case Column::price:
    case Column::amount:
      return Qt::AlignRight;
    default:
      return Qt::AlignLeft;
    }
  case Qt::DisplayRole:
    switch((Column)section)
    {
      case Column::date:
        return tr("Date");
      case Column::amount:
        return tr("Amount %1").arg(publicDataModel.getCoinCurrency());
      case Column::price:
        return tr("Price %1").arg(publicDataModel.getMarketCurrency());
    }
  }
  return QVariant();
}
