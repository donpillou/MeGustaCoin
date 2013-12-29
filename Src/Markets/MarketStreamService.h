
#pragma once

class MarketStreamService : public QObject
{
  Q_OBJECT

public:
  MarketStreamService(QObject* parent, DataModel& dataModel, PublicDataModel& publicDataModel, const QString& marketName);
  ~MarketStreamService();

  //const QString& getMarketName() const {return marketName;}

  void subscribe();
  void unsubscribe();

private:
  DataModel& dataModel;
  PublicDataModel& publicDataModel;
  QString marketName;
  MarketStream* marketStream;
  QThread* thread;
};
