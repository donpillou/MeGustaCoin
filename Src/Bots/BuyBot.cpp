
#include "stdafx.h"

BuyBot::Session::Session(Market& market) : market(market)
{
  memset(&parameters, 0, sizeof(Session::Parameters));
}

void BuyBot::Session::setParameters(double* parameters)
{
  memcpy(&this->parameters, parameters, sizeof(Session::Parameters));
}

void BuyBot::Session::handle(const DataProtocol::Trade& trade, const Values& values)
{
  checkBuy(trade, values);
  checkSell(trade, values);
}

void BuyBot::Session::handleBuy(const Market::Transaction& transaction)
{
}

void BuyBot::Session::handleSell(const Market::Transaction& transaction)
{
  double price = transaction.price;
  double amount = transaction.amount;

  // get buy transaction list
  QList<Market::Transaction> transactions;
  market.getBuyTransactions(transactions);

  // sort buy transaction list by price
  QMap<double, Market::Transaction> sortedTransactions;
  foreach(const Market::Transaction& transaction, transactions)
    sortedTransactions.insert(transaction.price, transaction);

  // iterator over sorted transaction list (descending)
  double fee = market.getFee();
  double invest = 0.;
  if(!sortedTransactions.isEmpty())
  {
    for(QMap<double, Market::Transaction>::Iterator i = --sortedTransactions.end(), begin = sortedTransactions.begin(); ; --i)
    {
      Market::Transaction& transaction = i.value();
      if(price >= transaction.price * (1. + fee * 2))
      {
        if(transaction.amount > amount)
        {
          invest += amount * transaction.price * (1. + fee);
          transaction.amount -= amount;
          market.updateTransaction(transaction.id, transaction);
          amount = 0.;
          break;
        }
        else
        {
          invest += transaction.amount * transaction.price * (1. + fee);
          market.removeTransaction(transaction.id);
          amount -= transaction.amount;
          if(amount == 0.)
            break;
        }
      }
      if(i == begin)
        break;
    }
  }
  market.removeTransaction(transaction.id);
  if(amount > 0.)
    market.warning("Sold something without profit.");
  else
    market.warning(QString("Earned %1.").arg(DataModel::formatPrice(price * transaction.amount * (1. - fee) - invest)));
}

void BuyBot::Session::checkBuy(const DataProtocol::Trade& trade, const Values& values)
{
  if(market.getOpenBuyOrderCount() > 0)
    return; // there is already an open buy order
  if(market.getTimeSinceLastBuy() < 60 * 30)
    return; // do not buy too often
  //const double minFall = 0.000005;
  for(int i = 0; i < (int)BellRegressions::numOfBellRegressions; ++i)
    if(values.bellRegressions[i].incline / values.bellRegressions[i].price > -parameters.minFall[i] * 0.00001)
      return; // price is not falling enough

  // try to buy something
  market.buy(trade.price * (1. + parameters.buyGain * 0.01), 0.01, 60 * 60);
}

void BuyBot::Session::checkSell(const DataProtocol::Trade& trade, const Values& values)
{
  if(market.getOpenSellOrderCount() > 0)
    return; // there is already an open sell order
  if(market.getTimeSinceLastSell() < 60 * 30)
    return; // do not sell too often
  for(int i = 0; i < (int)BellRegressions::numOfBellRegressions; ++i)
    if(values.bellRegressions[i].incline / values.bellRegressions[i].price < parameters.minRise[i] * 0.00001)
      return; // price is not rising enough
  
  double price = trade.price;
  double fee = market.getFee();
  /*
  fee *= 1. + parameters.minProfit * 2.;

  QList<Market::Transaction> transactions;
  market.getBuyTransactions(transactions);
  double profitableAmount = 0.;
  foreach(const Market::Transaction& transaction, transactions)
  {
    if(price > transaction.price * (1. + fee * 2.))
      profitableAmount += transaction.amount;
  }
  if(profitableAmount < 0.01)
    return; // selling would not be profitable
  */
  // try to sell something
  market.sell(trade.price * (1. + parameters.sellGain * 0.01), 0.01/*profitableAmount*/, 60 * 60);
}
