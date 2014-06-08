
#include "stdafx.h"

QString EDataSubscription::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EDataSubscription::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
