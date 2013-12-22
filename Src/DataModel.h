
#pragma once

class DataModel
{
public:
  OrderModel orderModel;
  TransactionModel transactionModel;
  GraphModel graphModel;
  TradeModel tradeModel;
  BookModel bookModel;
  LogModel logModel;

  DataModel() : /*tradeModel(graphModel),*/ bookModel(graphModel) {}
};
