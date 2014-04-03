
#include "stdafx.h"

QString EMarket::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EMarket::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
