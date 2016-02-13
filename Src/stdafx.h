
#pragma once

#include <QtGui>
#include <QTabFramework.h>
#include <megucoprotocol.h>

#include "Tools/JobQueue.h"
#include "Tools/Entity.h"

#include "Markets/DataConnection.h"

#include "Entities/EType.h"
#include "Entities/EBotSessionOrder.h"
#include "Entities/EBotSessionMarker.h"
#include "Entities/EBotSessionTransaction.h"
#include "Entities/EUserSessionAsset.h"
#include "Entities/EUserSessionAssetDraft.h"
#include "Entities/EUserSessionLogMessage.h"
#include "Entities/EBotSessionProperty.h"
#include "Entities/EBotType.h"
#include "Entities/EUserSession.h"
#include "Entities/EUserBroker.h"
#include "Entities/EUserBrokerOrder.h"
#include "Entities/EUserBrokerOrderDraft.h"
#include "Entities/EUserBrokerTransaction.h"
#include "Entities/EBrokerType.h"
#include "Entities/EUserBrokerBalance.h"
#include "Entities/ELogMessage.h"
#include "Entities/EDataService.h"
#include "Entities/EDataMarket.h"
#include "Entities/EDataTradeData.h"
#include "Entities/EDataTickerData.h"
#include "Entities/EDataSubscription.h"
#include "Entities/EProcess.h"

#include "Bots/TradeHandler.h"
#include "Bots/BotDialog.h"

#include "Graph/GraphRenderer.h"
#include "Graph/GraphModel.h"
#include "Graph/GraphView.h"
#include "Graph/GraphService.h"

#include "Models/SessionOrderModel.h"
#include "Models/SessionTransactionModel.h"
#include "Models/SessionItemModel.h"
#include "Models/SessionPropertyModel.h"
#include "Models/SessionLogModel.h"
#include "Models/MarketOrderModel.h"
#include "Models/MarketTransactionModel.h"
#include "Models/BotSessionModel.h"
#include "Models/UserBrokersModel.h"
#include "Models/TradeModel.h"
#include "Models/LogModel.h"
#include "Models/ProcessModel.h"

#include "Markets/DataService.h"

#include "Widgets/BrokersWidget.h"
#include "Widgets/OrdersWidget.h"
#include "Widgets/BotSessionsWidget.h"
#include "Widgets/BotLogWidget.h"
#include "Widgets/BotOrdersWidget.h"
#include "Widgets/BotTransactionsWidget.h"
#include "Widgets/BotItemsWidget.h"
#include "Widgets/BotPropertiesWidget.h"
#include "Widgets/TransactionsWidget.h"
#include "Widgets/TradesWidget.h"
#include "Widgets/GraphWidget.h"
#include "Widgets/LogWidget.h"
#include "Widgets/ProcessesWidget.h"

#include "MarketDialog.h"
#include "OptionsDialog.h"
#include "MainWindow.h"
