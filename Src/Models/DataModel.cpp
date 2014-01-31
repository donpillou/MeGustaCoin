
#include "stdafx.h"

DataModel::~DataModel()
{
  for(QMap<QString, PublicDataModel*>::Iterator i = publicDataModels.begin(), end = publicDataModels.end(); i != end; ++i)
  {
    PublicDataModel* publicDataModel = i.value();
    if(publicDataModel)
      delete publicDataModel;
  }
}

void DataModel::setMarket(const QString& marketName, const QString& coinCurrency, const QString& marketCurrency)
{
  this->marketName = marketName;
  this->coinCurrency = coinCurrency;
  this->marketCurrency = marketCurrency;
  emit changedMarket();
}

void DataModel::setBalance(const Market::Balance& balance)
{
  this->balance = balance;
  emit changedBalance();
}

//void DataModel::setTickerData(const Market::TickerData& tickerData)
//{
//  this->tickerData = tickerData;
//  emit changedTickerData();
//}

QString DataModel::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString DataModel::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}

void DataModel::addDataChannel(const QString& channelName)
{
  if(!publicDataModels.contains(channelName))
    publicDataModels.insert(channelName, 0);
}

void DataModel::clearDataChannel(const QString& channelName)
{
  PublicDataModel*& publicDataModel = publicDataModels[channelName];
  if(publicDataModel)
  {
    delete publicDataModel;
    publicDataModel = 0;
  }
}

PublicDataModel& DataModel::getDataChannel(const QString& channelName)
{
  PublicDataModel*& publicDataModel = publicDataModels[channelName];
  if(!publicDataModel)
  {
    publicDataModel = new PublicDataModel;
    publicDataModel->setMarket(channelName);
  }
  return *publicDataModel;
}
