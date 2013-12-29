
#include "stdafx.h"

class Token
{
public:
  char token;
  QVariant value;
};

static bool nextToken(const char*& data, Token& token)
{
  while(isspace(*data))
    ++data;
  token.token = *data;
  switch(token.token)
  {
  case '\0':
    return false;
  case '{':
  case '}':
  case '[':
  case ']':
  case ',':
  case ':':
    ++data;
    return true;
  case '"':
    {
      ++data;
      QString value;
      for(;;)
        switch(*data)
        {
        case 0:
          return false;
        case '\\':
          {
            ++data;
            switch(*data)
            {
            case '"':
            case '\\':
            case '/':
              value.append(*data);
              ++data;
              break;
            case 'b':
              value.append('\b');
              ++data;
              break;
            case 'f':
              value.append('\f');
              ++data;
              break;
            case 'n':
              value.append('\n');
              ++data;
              break;
            case 'r':
              value.append('\r');
              ++data;
              break;
            case 't':
              value.append('\t');
              ++data;
              break;
            case 'u':
              {
                ++data;
                QString k;
                k.reserve(4);
                for(int i = 0; i < 4; ++i)
                  if(*data)
                  {
                    k.append(*data);
                    ++data;
                  }
                  else
                    break;
                bool ok;
                int i = k.toInt(&ok, 16);
                if(ok)
                  value.append(QChar(i));
                break;
              }
              break;
            default:
              value.append('\\');
              value.append(*data);
              ++data;
              break;
            }
          }
          break;
        case '"':
          ++data;
          token.value = value;
          return true;
        default:
          value.append(*data);
          ++data;
          break;
        }
    }
    return false;
  case 't':
    if(strncmp(data, "true", 4) == 0)
    {
      data += 4;
      token.value = true;
      return true;
    }
    return false;
  case 'f':
    if(strncmp(data, "false", 5) == 0)
    {
      data += 5;
      token.value = false;
      return true;
    }
    return false;
  case 'n':
    if(strncmp(data, "null", 4) == 0)
    {
      data += 4;
      token.value = QString(); // creates a null variant
      return true;
    }
    return false;
  default:
    token.token = '#';
    if(*data == '-' || isdigit(*data))
    {
      QString n;
      bool isDouble = false;
      for(;;)
        switch(*data)
        {
        case 'E':
        case 'e':
        case '-':
        case '+':
          n.append(*data);
          ++data;
          break;
        case '.':
          isDouble = true;
          n.append(*data);
          ++data;
          break;
        default:
          if(isdigit(*data))
          {
            n.append(*data);
            ++data;
            break;
          }
          goto scanNumber;
        }
    scanNumber:
      if(isDouble)
      {
        token.value = n.toDouble();
        return true;
      }
      else
      {
        bool ok = false;
        int result = n.toInt(&ok);
        if(ok)
          token.value = result;
        else
          token.value = n.toLongLong();
        return true;
      }
    }
    return false;
  }
}

static QVariant parseObject(const char*& data, Token& token);
static QVariant parseValue(const char*& data, Token& token);
static QVariant parseArray(const char*& data, Token& token);

static QVariant parseObject(const char*& data, Token& token)
{
  if(token.token != '{')
    return QVariant();
  if(!nextToken(data, token))
    return QVariant();
  QVariantMap object;
  QString key;
  while(token.token != '}')
  {
    if(token.token != '"')
      return QVariant();
    key = token.value.toString();
    if(!nextToken(data, token))
      return QVariant();
    if(token.token != ':')
      return QVariant();
    if(!nextToken(data, token))
      return QVariant();
    object[key] = parseValue(data, token);
    if(token.token != ',')
      break;
    if(!nextToken(data, token))
      return QVariant();
  } 
  if(token.token != '}')
    return QVariant();
  if(!nextToken(data, token))
    return QVariant();
  return object;
}

static QVariant parseArray(const char*& data, Token& token)
{
  if(token.token != '[')
    return QVariant();
  if(!nextToken(data, token))
    return QVariant();
  QVariantList list;
  QVariant var;
  while(token.token != ']')
  {
    var = parseValue(data, token);
    list.push_back(var);
    if(token.token != ',')
      break;
    if(!nextToken(data, token))
      return QVariant();
  }
  if(token.token != ']')
    return QVariant();
  if(!nextToken(data, token))
    return QVariant();
  return list;
}

static QVariant parseValue(const char*& data, Token& token)
{
  switch(token.token)
  {
  case '"':
  case '#':
  case 't':
  case 'f':
  case 'n':
    {
      QVariant var = token.value;
      if(!nextToken(data, token))
        return QVariant();
      return var;
    }
  case '[':
    return parseArray(data, token);
  case '{':
    return parseObject(data, token);
  }
  return QVariant();
}

QVariantList Json::parse(const QByteArray& byteArray)
{
  QVariantList result;
  const char* data = byteArray.constData();
  Token token;
  QVariant var;
  if(!nextToken(data, token))
    return result;
  while(token.token == '{')
  {
    var = parseObject(data, token);
    if(var.isValid())
      result.push_back(var);
  }
  return result;
}
