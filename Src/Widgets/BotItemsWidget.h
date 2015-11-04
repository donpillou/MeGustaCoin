
#pragma once

class BotItemsWidget : public QWidget, public Entity::Listener
{
  Q_OBJECT

public:
  BotItemsWidget(QTabFramework& tabFramework, QSettings& settings, Entity::Manager& entityManager, BotService& botService, DataService& dataService);
  ~BotItemsWidget();

  void saveState(QSettings& settings);

private slots:
  void newBuyItem();
  void newSellItem();
  void submitItem();
  void cancelItem();
  void itemSelectionChanged();
  void itemDataChanged(const QModelIndex& topLeft, const QModelIndex& bottomRight);
  void editedItemFlipPrice(const QModelIndex& index, double price);

private:
  class DecimalDelegate : public QStyledItemDelegate
  {
  public:
    DecimalDelegate(int prec, QObject* parent) : QStyledItemDelegate(parent), prec(prec) {}
    QWidget* createEditor(QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
    {
      QWidget* widget = QStyledItemDelegate::createEditor(parent, option, index);
      QDoubleSpinBox* spinBox = qobject_cast<QDoubleSpinBox*>(widget);
      if(spinBox)
        spinBox->setDecimals(prec);
      return widget;
    }
  private:
    int prec;
  };

private:
  QTabFramework& tabFramework;
  Entity::Manager& entityManager;
  BotService& botService;
  DataService& dataService;

  SessionItemModel itemModel;

  QTreeView* itemView;
  QSortFilterProxyModel* proxyModel;

  QSet<EBotSessionItem*> selection;

  QAction* buyAction;
  QAction* sellAction;
  QAction* submitAction;
  QAction* cancelAction;

private:
  void addSessionItemDraft(EBotSessionItem::Type type);
  void updateToolBarButtons();

private: // Entity::Listener
  virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity);
};
