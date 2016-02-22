
#pragma once

class Json
{
public:
  static QVariant parse(const QByteArray& byteArray);
  static QByteArray generate(const QVariant& variant);
};
