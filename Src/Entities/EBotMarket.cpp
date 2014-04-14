
#include "stdafx.h"

QString EBotMarket::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EBotMarket::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
