
#pragma once

class GraphModel : public QObject
{
  Q_OBJECT

public:
  class Entry
  {
  public:
    quint64 time;
    double min;
    double max;

    Entry() : min(0), max(0) {}
  };

  double totalMin; // todo: do i really need this?
  double totalMax;

  QMap<quint64, Entry> trades;

  GraphModel() : totalMin(0), totalMax(0) {}

  void addTrade(quint64 time, double price);

signals:
  void dataAdded();
};
