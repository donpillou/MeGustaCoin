
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
    return true;
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
    if(token.token == '}')
      break;
    if(token.token != ',')
      return QVariant();
    if(!nextToken(data, token))
      return QVariant();
  } 
  if(!nextToken(data, token)) // skip }
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
    if(token.token == ']')
      break;
    if(token.token != ',')
      return QVariant();
    if(!nextToken(data, token))
      return QVariant();
  }
  if(!nextToken(data, token)) // skip ]
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
/*
QVariantList Json::parseList(const QByteArray& byteArray)
{
  QVariantList result;
  const char* data = byteArray.constData();
  Token token;
  QVariant var;
  if(!nextToken(data, token))
    return result;
  while(token.token != '\0')
  {
    var = parseValue(data, token);
    if(!var.isValid())
      continue;
    result.push_back(var);
  }
  return result;
}
*/
QVariant Json::parse(const QByteArray& byteArray)
{
  const char* data = byteArray.constData();
  Token token;
  if(!nextToken(data, token))
    return QVariant();
  QVariant var = parseValue(data, token);
  if(!var.isValid())
    return var;
  return var;
}

QByteArray Json::generate(const QVariant& variant)
{
  struct Generator
  {
    static bool generateString(const QByteArray& str, QByteArray& result)
    {
      size_t strLen = str.length();
      result.reserve(result.length() + 2 + strLen * 2);
      result += '"';
      for(const char* start = str, * p = start;;)
      {
        const char* e = strpbrk(p, "\"\\");
        if(!e)
        {
          result.append(p, strLen - (p - start));
          break;
        }
        if(e > p)
          result.append(p, e - p);
        switch(*e)
        {
        case '"':
          result += "\\\"";
          break;
        case '\\':
          result += "\\\\";
          break;
        }
        p = e + 1;
      }
      result += '"';
      return true;
    }

    static bool generate(const QByteArray& space, const QVariant& variant, QByteArray& result)
    {
      switch(variant.type())
      {
        case QVariant::Invalid:
          result.append("null", 4);
          return true;
        case QVariant::Bool:
        case QVariant::Double:
        case QVariant::Int:
        case QVariant::UInt:
        case QVariant::LongLong:
        case QVariant::ULongLong:
        {
          result.append(variant.toString().toAscii());
          return true;
        }
        case QVariant::String:
        {
          if(!generateString(variant.toString().toUtf8().constData(), result))
          return true;
        }
        case QVariant::Map:
        {
          const QVariantMap map = variant.toMap();
          if(map.isEmpty())
            result.append("{}", 2);
          else
          {
            result.append("{\n", 2);
            QByteArray newSpace = space;
            newSpace.append("  ", 2);
            for(QVariantMap::ConstIterator i = map.begin(), end = map.end();;)
            {
              result.append(newSpace);
              if(!generateString(i.key().toUtf8(), result))
                return false;
              result.append(": ", 2);
              if(!generate(newSpace, i.value(), result))
                return false;
              if(++i == end)
                break;
              result.append(",\n", 2);
            }
            result.append("\n", 1);
            result.append(space);
            result.append("}", 1);
          }
          return true;
        }
        case QVariant::List:
        {
          const QVariantList list = variant.toList();
          if(list.isEmpty())
            result.append("[]", 2);
          else
          {
            result.append("[\n", 2);
            QByteArray newSpace = space;
            newSpace.append("  ", 2);
            for(QVariantList::ConstIterator i = list.begin(), end = list.end();;)
            {
              result.append(newSpace);
              if(!generate(newSpace, *i, result))
                return false;
              if(++i == end)
                break;
              result.append(",\n", 2);
            }
            result.append("\n", 1);
            result.append(space);
            result.append("]", 1);
          }
          return true;
        }
        default:
          return false;
      }
    }
  };
  QByteArray result;
  if(!Generator::generate(QByteArray(), variant, result))
    return QByteArray();
  result.append("\n");
  return result;
}
