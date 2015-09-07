
#include "stdafx.h"

QString EBrokerType::formatAmount(double amount) const
{
  return QLocale::system().toString(fabs(amount), 'f', 8);
}

QString EBrokerType::formatPrice(double price) const
{
  return QLocale::system().toString(price, 'f', 2);
}
