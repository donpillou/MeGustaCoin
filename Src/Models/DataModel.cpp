
#include "stdafx.h"

DataModel::DataModel() : orderModel(*this), transactionModel(*this), botOrderModel(*this), botTransactionModel(*this), publicDataModel(0) {}

DataModel::~DataModel()
{
  for(QMap<QString, PublicDataModel*>::Iterator i = publicDataModels.begin(), end = publicDataModels.end(); i != end; ++i)
  {
    PublicDataModel* publicDataModel = i.value();
    if(publicDataModel)
      delete publicDataModel;
  }
}

void DataModel::setLoginData(const QString& userName, const QString& key, const QString& secret)
{
  this->userName = userName;
  this->key = key;
  this->secret = secret;
}

void DataModel::getLoginData(QString& userName, QString& key, QString& secret)
{
  userName = this->userName;
  key = this->key;
  secret = this->secret;
}

void DataModel::setMarket(const QString& marketName, const QString& coinCurrency, const QString& marketCurrency)
{
  this->marketName = marketName;
  this->coinCurrency = coinCurrency;
  this->marketCurrency = marketCurrency;
  this->publicDataModel = marketName.isEmpty() ? 0 : &getDataChannel(marketName);
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

QString DataModel::formatAmount(double amount)
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString DataModel::formatPrice(double price)
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
