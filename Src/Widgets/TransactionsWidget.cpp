
#include "stdafx.h"

TransactionsWidget::TransactionsWidget(QWidget* parent, QSettings& settings, DataModel& dataModel, MarketService& marketService) :
  QWidget(parent),
  dataModel(dataModel), transactionModel(dataModel.transactionModel),
  marketService(marketService)
{
  connect(&dataModel, SIGNAL(changedMarket()), this, SLOT(updateToolBarButtons()));

  QToolBar* toolBar = new QToolBar(this);
  toolBar->setStyleSheet("QToolBar { border: 0px }");
  toolBar->setIconSize(QSize(16, 16));
  toolBar->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
  refreshAction = toolBar->addAction(QIcon(":/Icons/arrow_refresh.png"), tr("&Refresh"));
  refreshAction->setEnabled(false);
  refreshAction->setShortcut(QKeySequence(QKeySequence::Refresh));
  connect(refreshAction, SIGNAL(triggered()), this, SLOT(refresh()));

  class TransactionSortProxyModel : public QSortFilterProxyModel
  {
  public:
    TransactionSortProxyModel(QObject* parent, TransactionModel& transactionModel) : QSortFilterProxyModel(parent), transactionModel(transactionModel) {}

  private:
    TransactionModel& transactionModel;

    virtual bool lessThan(const QModelIndex& left, const QModelIndex& right) const
    {
      const TransactionModel::Transaction* leftTransaction = transactionModel.getTransaction(left);
      const TransactionModel::Transaction* rightTransaction = transactionModel.getTransaction(right);
      switch((TransactionModel::Column)left.column())
      {
      case TransactionModel::Column::date:
        return leftTransaction->date.msecsTo(rightTransaction->date) > 0;
      case TransactionModel::Column::value:
        return leftTransaction->amount * leftTransaction->price < rightTransaction->amount * rightTransaction->price;
      case TransactionModel::Column::amount:
        return leftTransaction->amount < rightTransaction->amount;
      case TransactionModel::Column::price:
        return leftTransaction->price < rightTransaction->price;
      case TransactionModel::Column::fee:
        return leftTransaction->fee < rightTransaction->fee;
      case TransactionModel::Column::total:
        return leftTransaction->total < rightTransaction->total;
      }
      return QSortFilterProxyModel::lessThan(left, right);
    }
  };

  transactionView = new QTreeView(this);
  proxyModel = new TransactionSortProxyModel(this, transactionModel);
  proxyModel->setDynamicSortFilter(true);
  transactionView->setModel(proxyModel);
  transactionView->setSortingEnabled(true);
  transactionView->setRootIsDecorated(false);
  transactionView->setAlternatingRowColors(true);
  //transactionView->setSelectionMode(QAbstractItemView::ExtendedSelection);

  QVBoxLayout* layout = new QVBoxLayout;
  layout->setMargin(0);
  layout->setSpacing(0);
  layout->addWidget(toolBar);
  layout->addWidget(transactionView);
  setLayout(layout);

  proxyModel->setSourceModel(&transactionModel);
  QHeaderView* headerView = transactionView->header();
  headerView->resizeSection(0, 50);
  headerView->resizeSection(1, 110);
  headerView->resizeSection(2, 85);
  headerView->resizeSection(3, 100);
  headerView->resizeSection(4, 85);
  headerView->resizeSection(5, 75);
  headerView->resizeSection(6, 85);
  headerView->setStretchLastSection(false);
  headerView->setResizeMode(0, QHeaderView::Stretch);
  transactionView->sortByColumn(1);
  headerView->restoreState(settings.value("TransactionHeaderState").toByteArray());
}

void TransactionsWidget::saveState(QSettings& settings)
{
  settings.setValue("TransactionHeaderState", transactionView->header()->saveState());
}

void TransactionsWidget::updateToolBarButtons()
{
  bool hasMarket = marketService.isReady();

  refreshAction->setEnabled(hasMarket);
}

void TransactionsWidget::refresh()
{
  marketService.loadTransactions();
}
