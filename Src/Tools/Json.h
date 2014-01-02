
#pragma once

class Json
{
public:
  //static QVariantList parseList(const QByteArray& byteArray);
  static QVariant parse(const QByteArray& byteArray);
};
