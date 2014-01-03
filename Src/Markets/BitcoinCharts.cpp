
#include "stdafx.h"

QDateTime BitcoinCharts::lastRequest;
QHash<QString, BitcoinCharts::Data> BitcoinCharts::cachedData;
QString BitcoinCharts::lastError;
QMutex BitcoinCharts::mutex;
