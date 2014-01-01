

#pragma once

class HttpRequest
{
public:
  HttpRequest();
  ~HttpRequest();

  const QString& getLastError() {return error;}

  bool get(const QString& url, QByteArray& data);
  bool post(const QString& url, const QMap<QString, QString>& formData, QByteArray& data);

private:
  void* curl;
  QString error;
};
