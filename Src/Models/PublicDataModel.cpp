
#include "stdafx.h"

PublicDataModel::PublicDataModel(QObject* parent, const QColor& color) : QObject(parent),
  tradeModel(*this), bookModel(*this), color(color) {}

void PublicDataModel::setMarket(const QString& marketName, int features)
{
  this->marketName = marketName;
  this->features = features;
}

void PublicDataModel::setMarket(const QString& coinCurrency, const QString& marketCurrency)
{
  this->coinCurrency = coinCurrency;
  this->marketCurrency = marketCurrency;
  emit changedMarket();
}

QString PublicDataModel::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString PublicDataModel::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}

void PublicDataModel::addTrade(const MarketStream::Trade& trade)
{
  tradeModel.addTrade(trade);
  graphModel.addTrade(trade);
}
