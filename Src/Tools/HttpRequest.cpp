
#include "stdafx.h"

#include <curl/curl.h>
//#include <cstdlib>
//#include <cstring>

class Curl
{
public:
  Curl()
  {
    curl_global_init(CURL_GLOBAL_ALL);
  }
  ~Curl()
  {
    curl_global_cleanup();
  }

} curl;

HttpRequest::HttpRequest() : curl(0) {}

HttpRequest::~HttpRequest()
{
  if(curl)
    curl_easy_cleanup(curl);
}

bool HttpRequest::get(const QString& url, QByteArray& data)
{
  if(!curl)
  {
    curl = curl_easy_init();
    if(!curl)
    {
      error = "Could not initialize curl.";
      return false;
    }
  }
  else
    curl_easy_reset(curl);

  struct WriteResult
  {
    static size_t writeResponse(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      WriteResult* result = (struct WriteResult *)stream;
      size_t newBytes = size * nmemb;
      size_t totalSize = result->buffer->size() + newBytes;
      if(totalSize > (size_t)result->buffer->capacity())
        result->buffer->reserve(totalSize * 2);
      result->buffer->append((const char*)ptr, newBytes);
      return newBytes;
    }

    QByteArray* buffer;
  } writeResult = { &data };

  curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResult::writeResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeResult);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);

  data.clear();
  data.reserve(1500);

  CURLcode status = curl_easy_perform(curl);
  if(status != 0)
  {
    error = QString(curl_easy_strerror(status)) + ".";
    return false;
  }

  long code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if(code != 200)
  {
    error = QString("Server responded with code %1.").arg(code);
    return false;
  }

  error.clear();
  return true;
}

bool HttpRequest::post(const QString& url, const QMap<QString, QString>& formData, QByteArray& data)
{
  if(!curl)
  {
    curl = curl_easy_init();
    if(!curl)
    {
      error = "Could not initialize curl.";
      return false;
    }
  }
  else
    curl_easy_reset(curl);

  struct curl_httppost *formpost = NULL;
  struct curl_httppost *lastptr = NULL;
  for(QMap<QString, QString>::const_iterator i = formData.begin(), end = formData.end(); i != end; ++i)
    curl_formadd(&formpost, &lastptr, CURLFORM_COPYNAME, i.key().toUtf8().constData(),
      CURLFORM_COPYCONTENTS, i.value().toUtf8().constData(), CURLFORM_END);

  struct WriteResult
  {
    static size_t writeResponse(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      WriteResult* result = (struct WriteResult *)stream;
      size_t newBytes = size * nmemb;
      size_t totalSize = result->buffer->size() + newBytes;
      if(totalSize > (size_t)result->buffer->capacity())
        result->buffer->reserve(totalSize * 2);
      result->buffer->append((const char*)ptr, newBytes);
      return newBytes;
    }

    QByteArray* buffer;
  } writeResult = { &data };

  curl_easy_setopt(curl, CURLOPT_URL, url.toUtf8().constData());
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResult::writeResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeResult);
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);
  curl_easy_setopt(curl, CURLOPT_TIMEOUT, 40);

  data.clear();
  data.reserve(1500);

  CURLcode status = curl_easy_perform(curl);
  if(status != 0)
  {
    error = QString(curl_easy_strerror(status)) + ".";
    curl_formfree(formpost);
    return false;
  }

  long code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if(code != 200)
  {
    error = QString("Server responded with code %1.").arg(code);
    curl_formfree(formpost);
    return false;
  }

  error.clear();
  curl_formfree(formpost);
  return true;
}
