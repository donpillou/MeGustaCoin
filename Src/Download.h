

#pragma once

struct Download
{
  Download();
  ~Download();

  char* load(const char* url);
  char* loadPOST(const char* url, const char** fields, const char** values, unsigned int fieldCount);

private:
  char* data;
};

