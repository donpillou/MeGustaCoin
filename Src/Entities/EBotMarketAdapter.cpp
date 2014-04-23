
#include "stdafx.h"

QString EBotMarketAdapter::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EBotMarketAdapter::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
