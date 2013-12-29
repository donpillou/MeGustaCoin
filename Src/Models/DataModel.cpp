
#include "stdafx.h"

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

void DataModel::setTickerData(const Market::TickerData& tickerData)
{
  this->tickerData = tickerData;
  emit changedTickerData();
}

QString DataModel::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString DataModel::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
