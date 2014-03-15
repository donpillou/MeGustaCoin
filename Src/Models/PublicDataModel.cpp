
#include "stdafx.h"

PublicDataModel::PublicDataModel() :
  tradeModel(*this)/*, bookModel(*this)*/, state(State::offline), lastReceivedTradeId(0),
  tickerBid(0.), tickerAsk(0.) {}

void PublicDataModel::setMarket(const QString& marketName)
{
  this->marketName = marketName;
  startTime = QDateTime::currentDateTime().toTime_t();
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

void PublicDataModel::setTrades(const QList<DataProtocol::Trade>& trades)
{
  if(trades.isEmpty())
    return;

  for(QList<DataProtocol::Trade>::ConstIterator i = trades.begin(), end = --trades.end(); i != end; ++i)
  {
    const DataProtocol::Trade& trade = *i;
    graphModel.addTrade(trade, false);
  }

  const DataProtocol::Trade& trade = trades.back();
  graphModel.addTrade(trade, true);
  tradeModel.setTrades(trades);
  lastReceivedTradeId = trade.id;
}

void PublicDataModel::addTrade(const DataProtocol::Trade& trade)
{
  tradeModel.addTrade(trade);
  graphModel.addTrade(trade, true);
  lastReceivedTradeId = trade.id;
}

void PublicDataModel::addTicker(quint64 time, double bid, double ask)
{
  tickerBid = bid;
  tickerAsk = ask;
  emit updatedTicker();
}

bool PublicDataModel::getTicker(double& bid, double& ask) const
{
  bid = tickerBid;
  ask = tickerAsk;
  return tickerAsk != 0.;
}

void PublicDataModel::setState(State state)
{
  if(this->state == state)
    return;
  this->state = state;
  emit changedState();
}

QString PublicDataModel::getStateName() const
{
  switch(state)
  {
  case PublicDataModel::State::connecting:
    return tr("connecting...");
  case PublicDataModel::State::loading:
    return tr("loading...");
  case PublicDataModel::State::offline:
    return tr("offline");
  case PublicDataModel::State::connected:
    return QString();
  }
  Q_ASSERT(false);
  return QString();
}
