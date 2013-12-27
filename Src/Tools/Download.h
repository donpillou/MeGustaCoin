

#pragma once

class Download
{
public:
  Download();
  ~Download();

  char* load(const char* url);
  char* loadPOST(const char* url, const char** fields, const char** values, unsigned int fieldCount);

  const QString getErrorString() {return error;}

private:
  char* data;
  QString error;
};

