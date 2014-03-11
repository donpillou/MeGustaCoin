
#include "stdafx.h"

BuyBot::Session::Session(Market& market) : market(market)
{
  memset(&parameters, 0, sizeof(Session::Parameters));

  balanceBtc = market.getBalanceComm();
  balanceUsd = market.getBalanceBase();
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
  double price = transaction.price;
  double amount = transaction.amount;

  // get sell transaction list
  QList<Market::Transaction> transactions;
  market.getSellTransactions(transactions);

  // sort sell transaction list by price
  QMap<double, Market::Transaction> sortedTransactions;
  balanceUsd = market.getBalanceBase();
  foreach(const Market::Transaction& transaction, transactions)
  {
    sortedTransactions.insert(transaction.price, transaction);
    balanceUsd -= transaction.amount * transaction.price + transaction.fee;
  }

  // iterate over sorted transaction list (ascending)
  double fee = market.getFee();
  double invest = 0.;
  for(QMap<double, Market::Transaction>::Iterator i = sortedTransactions.begin(), end = sortedTransactions.end(); i != end; ++i)
  {
    Market::Transaction& transaction = i.value();
    if(transaction.price >= price * (1. + fee * 2))
    {
      if(transaction.amount > amount)
      {
        invest += amount * transaction.price * (1. - fee);
        transaction.amount -= amount;
        market.updateTransaction(transaction.id, transaction);
        amount = 0.;
        break;
      }
      else
      {
        invest += transaction.amount * transaction.price * (1. - fee);
        market.removeTransaction(transaction.id);
        amount -= transaction.amount;
        if(amount == 0.)
          break;
      }
    }
  }
  if(amount == 0.)
    market.warning(QString("Earned %1.").arg(DataModel::formatPrice(invest - price * transaction.amount * (1. + fee))));
  else
    market.warning("Bought something without profit.");
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
  balanceBtc = market.getBalanceComm();
  foreach(const Market::Transaction& transaction, transactions)
  {
    sortedTransactions.insert(transaction.price, transaction);
    balanceBtc -= transaction.amount;
  }

  // iterate over sorted transaction list (descending)
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
  if(amount == 0.)
    market.warning(QString("Earned %1.").arg(DataModel::formatPrice(price * transaction.amount * (1. - fee) - invest)));
  else
    market.warning("Sold something without profit.");
}

bool BuyBot::Session::isGoodBuy(const Values& values)
{
  for(int i = 0; i < (int)BellRegressions::bellRegression2h; ++i)
    if(values.bellRegressions[i].incline > 0.)
      return false; // price is not falling enough
  return true;
}

bool BuyBot::Session::isVeryGoodBuy(const Values& values)
{
  for(int i = 0; i < (int)Regressions::regression24h; ++i)
    if(values.regressions[i].incline > 0.)
      return false; // price is not falling enough
  return true;
}

bool BuyBot::Session::isGoodSell(const Values& values)
{
  for(int i = 0; i < (int)BellRegressions::bellRegression2h; ++i)
    if(values.bellRegressions[i].incline < 0.)
      return false; // price is not rising enough
  return true;
}

bool BuyBot::Session::isVeryGoodSell(const Values& values)
{
  for(int i = 0; i < (int)Regressions::regression24h; ++i)
    if(values.regressions[i].incline < 0.)
      return false; // price is not rising enough
  return true;
}


void BuyBot::Session::checkBuy(const DataProtocol::Trade& trade, const Values& values)
{
  if(market.getOpenBuyOrderCount() > 0)
    return; // there is already an open buy order
  if(market.getTimeSinceLastBuy() < 60 * 60)
    return; // do not buy too often

  double fee = market.getFee();

  if(isVeryGoodBuy(values) && balanceUsd >= trade.price * 0.02 * (1. + fee))
  {
    market.buy(trade.price, 0.02, 60 * 60);
    return;
  }

  if(isGoodBuy(values))
  {
    double price = trade.price;
    double fee = market.getFee() * (1. + parameters.buyProfitGain);

    QList<Market::Transaction> transactions;
    market.getSellTransactions(transactions);
    double profitableAmount = 0.;
    foreach(const Market::Transaction& transaction, transactions)
    {
      if(price * (1. + fee * 2.) < transaction.price)
        profitableAmount += transaction.amount;
    }
    if(profitableAmount >= 0.01)
    {
      market.buy(trade.price, qMax(0.01, profitableAmount * 0.5), 60 * 60);
      return;
    }
  }
}

void BuyBot::Session::checkSell(const DataProtocol::Trade& trade, const Values& values)
{
  if(market.getOpenSellOrderCount() > 0)
    return; // there is already an open sell order
  if(market.getTimeSinceLastSell() < 60 * 60)
    return; // do not sell too often

  if(isVeryGoodSell(values) && balanceBtc >= 0.02)
  {
    market.sell(trade.price, 0.02, 60 * 60);
    return;
  }

  if(isGoodSell(values))
  {
    double price = trade.price;
    double fee = market.getFee() * (1. + parameters.sellProfitGain);

    QList<Market::Transaction> transactions;
    market.getBuyTransactions(transactions);
    double profitableAmount = 0.;
    foreach(const Market::Transaction& transaction, transactions)
    {
      if(price > transaction.price * (1. + fee * 2.))
        profitableAmount += transaction.amount;
    }
    if(profitableAmount >= 0.01)
    {
      market.sell(trade.price, qMax(0.01, profitableAmount * 0.5), 60 * 60);
      return;
    }
  }
}
