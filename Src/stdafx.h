
#pragma once

#include <QtGui>
#include <QTabFramework.h>
#include <megucoprotocol.h>

#include "Tools/JobQueue.h"
#include "Tools/Entity.h"

#include "Markets/DataConnection.h"

#include "Entities/EType.h"
#include "Entities/EUserSessionOrder.h"
#include "Entities/EUserSessionMarker.h"
#include "Entities/EUserSessionTransaction.h"
#include "Entities/EUserSessionAsset.h"
#include "Entities/EUserSessionAssetDraft.h"
#include "Entities/EUserSessionLogMessage.h"
#include "Entities/EUserSessionProperty.h"
#include "Entities/EBotType.h"
#include "Entities/EUserSession.h"
#include "Entities/EUserBroker.h"
#include "Entities/EUserBrokerOrder.h"
#include "Entities/EUserBrokerOrderDraft.h"
#include "Entities/EUserBrokerTransaction.h"
#include "Entities/EBrokerType.h"
#include "Entities/EUserBrokerBalance.h"
#include "Entities/ELogMessage.h"
#include "Entities/EConnection.h"
#include "Entities/EMarket.h"
#include "Entities/EMarketTradeData.h"
#include "Entities/EMarketTickerData.h"
#include "Entities/EMarketSubscription.h"
#include "Entities/EProcess.h"

#include "Bots/TradeHandler.h"
#include "Bots/BotDialog.h"

#include "Graph/GraphRenderer.h"
#include "Graph/GraphModel.h"
#include "Graph/GraphView.h"
#include "Graph/GraphService.h"

#include "Models/UserSessionOrdersModel.h"
#include "Models/UserSessionTransactionsModel.h"
#include "Models/UserSessionAssetsModel.h"
#include "Models/UserSessionPropertiesModel.h"
#include "Models/UserSessionLogModel.h"
#include "Models/UserBrokerOrdersModel.h"
#include "Models/UserBrokerTransactionsModel.h"
#include "Models/UserSessionsModel.h"
#include "Models/UserBrokersModel.h"
#include "Models/MarketTradesModel.h"
#include "Models/LogModel.h"
#include "Models/ProcessesModel.h"

#include "Markets/DataService.h"

#include "Widgets/BrokersWidget.h"
#include "Widgets/OrdersWidget.h"
#include "Widgets/BotSessionsWidget.h"
#include "Widgets/BotLogWidget.h"
#include "Widgets/BotOrdersWidget.h"
#include "Widgets/BotTransactionsWidget.h"
#include "Widgets/UserSessionAssetsWidget.h"
#include "Widgets/BotPropertiesWidget.h"
#include "Widgets/TransactionsWidget.h"
#include "Widgets/TradesWidget.h"
#include "Widgets/GraphWidget.h"
#include "Widgets/LogWidget.h"
#include "Widgets/ProcessesWidget.h"

#include "MarketDialog.h"
#include "OptionsDialog.h"
#include "MainWindow.h"
