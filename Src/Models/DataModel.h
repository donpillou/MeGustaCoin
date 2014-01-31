
#pragma once

class DataModel : public QObject
{
  Q_OBJECT

public:
  OrderModel orderModel;
  TransactionModel transactionModel;
  LogModel logModel;

  DataModel() : orderModel(*this), transactionModel(*this) {}
  ~DataModel();

  void setMarket(const QString& marketName, const QString& coinCurrency, const QString& marketCurrency);
  void setBalance(const Market::Balance& balance);

  void addDataChannel(const QString& channel);
  void clearDataChannel(const QString& channel);
  const QMap<QString, PublicDataModel*>& getDataChannels() {return publicDataModels;}
  PublicDataModel& getDataChannel(const QString& channel);

  const QString& getMarketName() const {return marketName;}
  const QString& getCoinCurrency() const {return coinCurrency;}
  const QString& getMarketCurrency() const {return marketCurrency;}
  const Market::Balance& getBalance() const {return balance;}

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

signals:
  void changedMarket();
  void changedBalance();

private:
  QString marketName;
  QString coinCurrency;
  QString marketCurrency;
  Market::Balance balance;
  QMap<QString, PublicDataModel*> publicDataModels;
};
