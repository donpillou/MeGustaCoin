
#include "stdafx.h"

#include <curl/curl.h>
#include <cstdlib>
#include <cstring>

Download::Download() : data(0) {}

Download::~Download()
{
  if(data)
    free(data);
}

char* Download::load(const char* url)
{
  error.clear();

  CURL* curl = curl_easy_init();
  if(!curl)
  {
    error = "Could not initialize curl.";
    return 0;
  }

  struct WriteResult
  {
    char *data;
    size_t size;

    static size_t writeResponse(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      WriteResult* result = (struct WriteResult *)stream;
      size_t newBytes = size * nmemb;
      size_t totalSize = result->size + newBytes;

      result->data = (char*) realloc(result->data, totalSize + 1);
      if(!result->data)
        return 0;

      memcpy(result->data + result->size, ptr, newBytes);
      result->size = totalSize;

      return newBytes;
    }
  } writeResult = { (char*) malloc(512), 0 };

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResult::writeResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeResult);

  CURLcode status = curl_easy_perform(curl);
  if(status != 0)
  {
    //fprintf(stderr, "error: unable to request data from %s:\n", url);
    //fprintf(stderr, "%s\n", curl_easy_strerror(status));
    error = curl_easy_strerror(status);
    goto error;
  }

  long code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if(code != 200)
  {
    //fprintf(stderr, "error: server responded with code %ld\n", code);
    error.sprintf("Server responsed with code %d.", code);
    goto error;
  }

  if(!writeResult.data)
  {
    //printf(stderr, "error: not enough memory\n");
    error = "Could not allocate enough memory.";
    goto error;
  }

  writeResult.data[writeResult.size] = '\0';
  this->data = writeResult.data;

  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return writeResult.data;

error:
  if(curl)
    curl_easy_cleanup(curl);
  curl_global_cleanup();
  return 0;
}

char* Download::loadPOST(const char* url, const char** fields, const char** values, unsigned int fieldCount)
{
  error.clear();

  curl_global_init(CURL_GLOBAL_ALL);

  struct curl_httppost *formpost=NULL;
  struct curl_httppost *lastptr=NULL;

  for(unsigned int i = 0; i < fieldCount; ++i)
    curl_formadd(&formpost,
                 &lastptr,
                 CURLFORM_COPYNAME, fields[i],
                 CURLFORM_COPYCONTENTS, values[i],
                 CURLFORM_END);

  CURL* curl = curl_easy_init();
  if(!curl)
  {
    curl_global_cleanup();
    error = "Could not initialize curl.";
    return 0;
  }

  struct WriteResult
  {
    char *data;
    size_t size;

    static size_t writeResponse(void *ptr, size_t size, size_t nmemb, void *stream)
    {
      WriteResult* result = (struct WriteResult *)stream;
      size_t newBytes = size * nmemb;
      size_t totalSize = result->size + newBytes;

      result->data = (char*) realloc(result->data, totalSize + 1);
      if(!result->data)
        return 0;

      memcpy(result->data + result->size, ptr, newBytes);
      result->size = totalSize;

      return newBytes;
    }
  } writeResult = { (char*) malloc(512), 0 };

  curl_easy_setopt(curl, CURLOPT_URL, url);
  curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteResult::writeResponse);
  curl_easy_setopt(curl, CURLOPT_WRITEDATA, &writeResult);
  curl_easy_setopt(curl, CURLOPT_HTTPPOST, formpost);

  CURLcode status = curl_easy_perform(curl);
  if(status != 0)
  {
    //fprintf(stderr, "error: unable to request data from %s:\n", url);
    //fprintf(stderr, "%s\n", curl_easy_strerror(status));
    error = curl_easy_strerror(status);
    goto error;
  }

  long code;
  curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &code);
  if(code != 200)
  {
    //fprintf(stderr, "error: server responded with code %ld\n", code);
    error.sprintf("Server responsed with code %d.", code);
    goto error;
  }

  if(!writeResult.data)
  {
    //printf(stderr, "error: not enough memory\n");
    error = "Could not allocate enough memory.";
    goto error;
  }

  writeResult.data[writeResult.size] = '\0';
  this->data = writeResult.data;

  curl_formfree(formpost);
  curl_easy_cleanup(curl);
  curl_global_cleanup();

  return writeResult.data;

error:
  if(formpost)
    curl_formfree(formpost);
  if(curl)
    curl_easy_cleanup(curl);

  curl_global_cleanup();
  return 0;
}
