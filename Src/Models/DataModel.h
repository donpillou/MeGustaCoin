
#pragma once

class DataModel : public QObject
{
  Q_OBJECT

public:
  OrderModel orderModel;
  TransactionModel transactionModel;
  //GraphModel graphModel;
  //TradeModel tradeModel;
  //BookModel bookModel;
  LogModel logModel;

  DataModel() : orderModel(*this), transactionModel(*this)/*, tradeModel(*this), bookModel(*this) */ {}

  void setMarket(const QString& marketName, const QString& coinCurrency, const QString& marketCurrency);
  void setBalance(const Market::Balance& balance);
  //void setTickerData(const Market::TickerData& tickerData);

  const QString& getMarketName() const {return marketName;}
  const QString& getCoinCurrency() const {return coinCurrency;}
  const QString& getMarketCurrency() const {return marketCurrency;}
  //const Market::TickerData& getTickerData() const {return tickerData;}
  const Market::Balance& getBalance() const {return balance;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

signals:
  void changedMarket();
  void changedBalance();
  //void changedTickerData();

private:
  QString marketName;
  QString coinCurrency;
  QString marketCurrency;
  Market::Balance balance;
};
