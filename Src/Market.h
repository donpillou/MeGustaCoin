
#pragma once

class Market
{
public:
  virtual ~Market() {};

  virtual void loadOrders() = 0;

  QAbstractItemModel& getOrderModel() {return orderModel;};

protected:
  OrderModel orderModel;
};
