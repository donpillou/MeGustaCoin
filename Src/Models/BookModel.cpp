
#include "stdafx.h"

BookModel::BookModel(PublicDataModel& publicDataModel) :
  askModel(publicDataModel), bidModel(publicDataModel),
  publicDataModel(publicDataModel), graphModel(publicDataModel.graphModel),
  time(0)
{
  connect(&publicDataModel, SIGNAL(changedMarket()), this, SLOT(updateHeader()));
}

BookModel::ItemModel::ItemModel(PublicDataModel& publicDataModel) : publicDataModel(publicDataModel)
{
}

BookModel::ItemModel::~ItemModel()
{
  qDeleteAll(items);
}

void BookModel::updateHeader()
{
  askModel.updateHeader();
  bidModel.updateHeader();
}

void BookModel::setData(quint64 time, const QList<Market::OrderBookEntry>& askItems, const QList<Market::OrderBookEntry>& bidItems)
{
  if(time == this->time)
    return;

  this->time = time;
  askModel.setData(askItems);
  bidModel.setData(bidItems);

  enum SumType
  {
    sum50,
    sum100,
    sum250,
    sum500,
    numOfSumType
  };
  double sumMax[numOfSumType] = {50., 100., 250., 500.};

  double askSum[numOfSumType] = {};  
  double askSumMass[numOfSumType] = {};
  for(int i = askItems.size() - 1; i >= 0; --i)
  {
    const Market::OrderBookEntry& item = askItems[i];
    for(int i = 0; i < numOfSumType; ++i)
      if(askSum[i] < sumMax[i])
      {
        double amount = qMin(item.amount, sumMax[i] - askSum[i]);
        askSumMass[i] += item.price * amount;
        askSum[i] += amount;
      }
    if(askSum[numOfSumType - 1] >= sumMax[numOfSumType - 1])
      break;
  }

  double bidSum[numOfSumType] = {};  
  double bidSumMass[numOfSumType] = {};
  for(int i = bidItems.size() - 1; i >= 0; --i)
  {
    const Market::OrderBookEntry& item = bidItems[i];
    for(int i = 0; i < numOfSumType; ++i)
      if(bidSum[i] < sumMax[i])
      {
        double amount = qMin(item.amount, sumMax[i] - bidSum[i]);
        bidSumMass[i] += item.price * amount;
        bidSum[i] += amount;
      }
    if(bidSum[numOfSumType - 1] >= sumMax[numOfSumType - 1])
      break;
  }


  GraphModel::BookSample summary;
  summary.time = time;
  summary.ask = askItems.isEmpty() ? 0 : askItems.back().price;
  summary.bid = bidItems.isEmpty() ? 0 : bidItems.back().price;
  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice100] = askSum[sum50] >= sumMax[sum50] && bidSum[sum50] >= sumMax[sum50] ? (askSumMass[sum50] + bidSumMass[sum50]) / (askSum[sum50] + bidSum[sum50]) : 0;
  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice200] = askSum[sum100] >= sumMax[sum100] && bidSum[sum100] >= sumMax[sum100] ? (askSumMass[sum100] + bidSumMass[sum100]) / (askSum[sum100] + bidSum[sum100]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice100];
  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice500] = askSum[sum250] >= sumMax[sum250] && bidSum[sum250] >= sumMax[sum250] ? (askSumMass[sum250] + bidSumMass[sum250]) / (askSum[sum250] + bidSum[sum250]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice200];
  summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice1000] = askSum[sum500] >= sumMax[sum500] && bidSum[sum500] >= sumMax[sum500] ? (askSumMass[sum500] + bidSumMass[sum500]) / (askSum[sum500] + bidSum[sum500]) : summary.comPrice[(int)GraphModel::BookSample::ComPrice::comPrice500];
  graphModel.addBookSample(summary);
}

void BookModel::reset()
{
  askModel.reset();
  bidModel.reset();
}

void BookModel::ItemModel::updateHeader()
{
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void BookModel::ItemModel::reset()
{
  emit beginResetModel();
  items.clear();
  emit endResetModel();
  emit headerDataChanged(Qt::Horizontal, (int)Column::first, (int)Column::last);
}

void BookModel::ItemModel::setData(const QList<Market::OrderBookEntry>& newData)
{
  int oldCount = items.size();
  int newCount = qMin(newData.size(), 100);
  int destIndex = 0, srcIndex = newData.size() - newCount;
  for(int count = qMin(oldCount, newCount); destIndex < count; ++destIndex, ++srcIndex)
  {
    *items[destIndex] = newData[srcIndex];
  }
  if(destIndex > 0)
    emit dataChanged(createIndex(0, (int)Column::first, 0), createIndex(destIndex, (int)Column::last, 0));
  if(destIndex < newCount)
  { // add items
    beginInsertRows(QModelIndex(), oldCount, oldCount + (newCount - destIndex) - 1);
    for(int count = newCount; destIndex < count; ++destIndex, ++srcIndex)
    {
      Market::OrderBookEntry* newItem = new Market::OrderBookEntry;
      *newItem = newData[srcIndex];
      items.push_back(newItem);
    }
    endInsertRows();
  }
  else if(items.size() > newCount)
  { // remove items
    beginRemoveRows(QModelIndex(), destIndex, items.size() - 1);
    for(int j = items.size() - 1; j >= destIndex; --j)
    {
      delete items[j];
      items.removeAt(j);
    }
    endRemoveRows();
  }

  //graphModel.
}

QModelIndex BookModel::ItemModel::index(int row, int column, const QModelIndex& parent) const
{
  return createIndex(row, column, 0);
}

QModelIndex BookModel::ItemModel::parent(const QModelIndex& child) const
{
  return QModelIndex();
}

int BookModel::ItemModel::rowCount(const QModelIndex& parent) const
{
  return parent.isValid() ? 0 : items.size();
}

int BookModel::ItemModel::columnCount(const QModelIndex& parent) const
{
  return (int)Column::last + 1;
}

QVariant BookModel::ItemModel::data(const QModelIndex& index, int role) const
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
  if(row < 0 || row >= items.size())
    return QVariant();
  const Market::OrderBookEntry& item = *items[row];

  switch(role)
  {
  case Qt::DisplayRole:
    switch((Column)index.column())
    {
    case Column::amount:
      return publicDataModel.formatAmount(item.amount);
    case Column::price:
      return publicDataModel.formatPrice(item.price);
    }
  }
  return QVariant();
}

QVariant BookModel::ItemModel::headerData(int section, Qt::Orientation orientation, int role) const
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
      case Column::amount:
        return tr("Amount %1").arg(publicDataModel.getCoinCurrency());
      case Column::price:
        return tr("Price %1").arg(publicDataModel.getMarketCurrency());
    }
  }
  return QVariant();
}
