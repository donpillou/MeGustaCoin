
#pragma once

#include <QtGui>

#include "Tools/HttpRequest.h"
#include "Tools/SocketConnection.h"
#include "Tools/Sha256.h"
#include "Tools/Json.h"
#include "Tools/JobQueue.h"
#include "Tools/Entity.h"

#include "Markets/DataProtocol.h"
#include "Bots/BotProtocol.h"

#include "Entities/EType.h"
#include "Entities/EOrder.h"
#include "Entities/ETransaction.h"
#include "Entities/EBotEngine.h"
#include "Entities/EBotSession.h"
#include "Entities/EBotMarket.h"
#include "Entities/EBotService.h"
#include "Entities/EBotMarketAdapter.h"

#include "Markets/DataConnection.h"
#include "Markets/Market.h"
#include "Markets/BitstampMarket.h"

#include "Bots/BotConnection.h"
#include "Bots/Bot.h"
#include "Bots/BuyBot.h"
#include "Bots/ParticleSwarm.h"
#include "Bots/BotDialog.h"

#include "Models/GraphModel.h"
#include "Models/OrderModel.h"
#include "Models/OrderModel2.h"
#include "Models/TransactionModel.h"
#include "Models/TransactionModel2.h"
#include "Models/BotSessionModel.h"
#include "Models/BotMarketModel.h"
#include "Models/TradeModel.h"
#include "Models/BookModel.h"
#include "Models/LogModel.h"
#include "Models/PublicDataModel.h"
#include "Models/BotsModel.h"
#include "Models/DataModel.h"

#include "Bots/BotService.h"
#include "Markets/MarketService.h"
#include "Markets/DataService.h"

#include "Widgets/MarketsWidget.h"
#include "Widgets/OrdersWidget.h"
#include "Widgets/BotsWidget.h"
#include "Widgets/TransactionsWidget.h"
#include "Widgets/TradesWidget.h"
#include "Widgets/BookWidget.h"
#include "Widgets/GraphView.h"
#include "Widgets/GraphWidget.h"
#include "Widgets/LogWidget.h"

#include "LoginDialog.h"
#include "MarketDialog.h"
#include "OptionsDialog.h"
#include "MainWindow.h"
