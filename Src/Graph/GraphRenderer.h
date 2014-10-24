
#pragma once

class GraphRenderer
{
public:
  enum Data
  {
    trades = 0x01,
    tradeVolume = 0x02,
    orderBook = 0x04,
    regressionLines = 0x08,
    otherMarkets = 0x10,
    //estimates = 0x20,
    expRegressionLines = 0x20,
    all = 0xffff,
  };

  struct TradeSample
  {
    quint64 time;
    double min;
    double max;
    double first;
    double last;
    double amount;
  };

public:
  GraphRenderer();
  ~GraphRenderer();

  void enable(bool enable) {enabled = enable;}
  void setSize(const QSize& size);
  void setMaxAge(int maxAge);
  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

  void addTradeData(const QList<DataProtocol::Trade>& data);
  void addSessionMarker(const EBotSessionMarker& marker);
  void clearSessionMarker();

  bool isEnabled() const {return enabled && width != 0 && height != 0;}
  bool isUpToDate() const {return upToDate;}
  void setUpToDate(bool value) {upToDate = value;}

  QImage& render(const QMap<QString, GraphRenderer*>& graphDataByName);

private:
  class PolylineData
  {
  public:
    QPointF* polyData;
    unsigned int polyDataSize;
    unsigned int polyDataCount;
    QPoint* volumeData;
    unsigned int volumeDataSize;
    unsigned int volumeDataCount;
    QColor color;
    bool updated;
    bool removeThis;

    PolylineData() : polyData(0), polyDataSize(0), polyDataCount(0), volumeData(0), volumeDataSize(0), volumeDataCount(0), updated(false), removeThis(false) {}

    ~PolylineData()
    {
      delete polyData;
      delete volumeData;
    }
  };

private:
  TradeHandler tradeHandler;
  QList<TradeSample> tradeSamples;
  QList<TradeSample> lowResTradeSamples;
  QMultiMap<quint64, EBotSessionMarker::Type> markers;
  TradeHandler::Values* values;
  bool enabled;
  bool upToDate;

  unsigned int enabledData;
  int height;
  int width;
  QImage* image;

  quint64 time;
  quint64 ownTime;
  int maxAge;
  double totalMin;
  double totalMax;
  double volumeMax;

  QHash<const GraphRenderer*, PolylineData> polylineData;

private:
  void prepareTradePolyline(const QRect& rect, double ymin, double ymax, double lastVolumeMax, const GraphRenderer& graphModel, int enabledData, double scale, const QColor& color);
  void drawAxesLables(QPainter& painter, const QRect& rect, double vmin, double vmax, const QSize& priceSize);
  void drawTradePolylines(QPainter& painter);
  void drawRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax);
  void drawExpRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax);
  void drawMarkers(QPainter& painter, const QRect& rect, double vmin, double vmax);

  inline void addToMinMax(double price)
  {
    if(price > totalMax)
      totalMax = price;
    if(price < totalMin)
      totalMin = price;
  }

  QString formatPrice(double price) const
  {
    return QLocale::system().toString(price, 'f', 2);
  }
};
