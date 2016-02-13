
#include "stdafx.h"

QString EMarketSubscription::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EMarketSubscription::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
