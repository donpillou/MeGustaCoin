
#pragma once

class GraphView : public QWidget, public Entity::Listener
{
public:
  GraphView(QWidget* parent, const PublicDataModel* publicDataModel, const QMap<QString, PublicDataModel*>& publicDataModels, Entity::Manager& entityManager);
  ~GraphView();

  void setFocusPublicDataModel(const PublicDataModel* publicDataModel);

  void setMaxAge(int maxAge);
  int getMaxAge() const {return maxAge;}

  enum class Data
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

  void setEnabledData(unsigned int data);
  unsigned int getEnabledData() const {return enabledData;}

  virtual QSize sizeHint() const;

private:
  const PublicDataModel* publicDataModel;
  const QMap<QString, PublicDataModel*>& publicDataModels;
  const GraphModel* graphModel;
  Entity::Manager& entityManager;

  unsigned int enabledData;
  bool drawSessionMarkers;

  quint64 time;
  int maxAge;
  double totalMin;
  double totalMax;
  double volumeMax;

  class GraphModelData
  {
  public:
    QPointF* polyData;
    unsigned int polyDataSize;
    unsigned int polyDataCount;
    QPoint* volumeData;
    unsigned int volumeDataSize;
    unsigned int volumeDataCount;
    QColor color;
    bool drawn;

    GraphModelData() : polyData(0), polyDataSize(0), polyDataCount(0), volumeData(0), volumeDataSize(0), volumeDataCount(0), drawn(false) {}

    ~GraphModelData()
    {
      delete polyData;
      delete volumeData;
    }
  };
  QHash<const GraphModel*, GraphModelData> graphModelData;

  virtual void paintEvent(QPaintEvent* );

  void drawAxesLables(QPainter& painter, const QRect& rect, double vmin, double vmax, const QSize& priceSize);
  void prepareTradePolyline(const QRect& rect, double vmin, double vmax, double lastVolumeMax, const GraphModel& graphModel, int enabledData, double scale, const QColor& color);
  void drawTradePolylines(QPainter& painer);
  void drawBookPolyline(QPainter& painter, const QRect& rect, double vmin, double vmax);
  void drawRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax);
  void drawExpRegressionLines(QPainter& painter, const QRect& rect, double vmin, double vmax);
  //void drawEstimates(QPainter& painter, const QRect& rect, double vmin, double vmax);
  void drawMarkers(QPainter& painter, const QRect& rect, double vmin, double vmax);

  inline void addToMinMax(double price)
  {
    if(price > totalMax)
      totalMax = price;
    if(price < totalMin)
      totalMin = price;
  }

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
