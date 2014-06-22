
#pragma once

#include <QtGui>

#include "Tools/SocketConnection.h"
#include "Tools/Sha256.h"
#include "Tools/JobQueue.h"
#include "Tools/Entity.h"
#include "QTabFramework.h"

#include "Markets/DataProtocol.h"
#include "Bots/BotProtocol.h"

#include "Entities/EType.h"
#include "Entities/EBotSessionOrder.h"
#include "Entities/EBotSessionMarker.h"
#include "Entities/EBotSessionTransaction.h"
#include "Entities/EBotSessionLogMessage.h"
#include "Entities/EBotSessionBalance.h"
#include "Entities/EBotEngine.h"
#include "Entities/EBotSession.h"
#include "Entities/EBotMarket.h"
#include "Entities/EBotMarketOrder.h"
#include "Entities/EBotMarketOrderDraft.h"
#include "Entities/EBotMarketTransaction.h"
#include "Entities/EBotService.h"
#include "Entities/EBotMarketAdapter.h"
#include "Entities/EBotMarketBalance.h"
#include "Entities/ELogMessage.h"
#include "Entities/EDataService.h"
#include "Entities/EDataMarket.h"
#include "Entities/EDataTradeData.h"
#include "Entities/EDataTickerData.h"
#include "Entities/EDataSubscription.h"

#include "Markets/DataConnection.h"

#include "Bots/BotConnection.h"
#include "Bots/TradeHandler.h"
#include "Bots/BotDialog.h"

#include "Models/GraphModel.h"
#include "Models/SessionOrderModel.h"
#include "Models/SessionTransactionModel.h"
#include "Models/SessionLogModel.h"
#include "Models/MarketOrderModel.h"
#include "Models/MarketTransactionModel.h"
#include "Models/BotSessionModel.h"
#include "Models/BotMarketModel.h"
#include "Models/TradeModel.h"
#include "Models/LogModel.h"

#include "Bots/BotService.h"
#include "Markets/DataService.h"

#include "Widgets/MarketsWidget.h"
#include "Widgets/OrdersWidget.h"
#include "Widgets/BotsWidget.h"
#include "Widgets/TransactionsWidget.h"
#include "Widgets/TradesWidget.h"
#include "Widgets/GraphView.h"
#include "Widgets/GraphWidget.h"
#include "Widgets/LogWidget.h"

#include "MarketDialog.h"
#include "OptionsDialog.h"
#include "MainWindow.h"
