
#pragma once

class BotProtocol
{
public:
  enum MessageType
  {
    pingRequest,
    pingResponse, // a.k.a. pong
    loginRequest,
    loginResponse,
    authRequest,
    authResponse,
    registerBotRequest,
    registerBotResponse,
    registerMarketRequest,
    registerMarketResponse,
    updateEntity,
    removeEntity,
    controlEntity,
    controlEntityResponse,
    createEntity,
    createEntityResponse,
  };
  
  enum EntityType
  {
    error,
    session,
    engine,
    marketAdapter,
    sessionTransaction,
    sessionOrder,
    sessionLogMessage,
    market,
    marketTransaction,
    marketOrder,
    marketBalance,
  };

#pragma pack(push, 1)
  struct Header
  {
    quint32 size; // including header
    quint16 messageType; // MessageType
  };

  struct Entity
  {
    quint16 entityType; // EntityType
    quint32 entityId;
  };

  struct CreateEntityResponse : public Entity
  {
    quint32 id; // id of the created entity
    quint8 success;
  };

  struct LoginRequest
  {
    char userName[33];
  };

  struct LoginResponse
  {
    unsigned char userKey[32];
    unsigned char loginKey[32];
  };

  struct AuthRequest
  {
    unsigned char signature[32];
  };

  struct Error : public Entity
  {
    char errorMessage[129];
  };

  struct Session : public Entity
  {
    enum State
    {
      stopped,
      starting,
      running,
      simulating,
    };

    char name[33];
    quint32 botEngineId;
    quint32 marketId;
    quint8 state;
    double balanceBase;
    double balanceComm;
  };

  struct BotEngine : public Entity
  {
    char name[33];
  };

  struct MarketAdapter : public Entity
  {
    char name[33];
    char currencyBase[33];
    char currencyComm[33];
  };

  struct Transaction : public Entity
  {
    enum Type
    {
      buy,
      sell
    };

    quint8 type;
    qint64 date;
    double price;
    double amount;
    double fee;
  };

  struct Order : public Entity
  {
    enum Type
    {
      buy,
      sell,
    };

    quint8 type;
    qint64 date;
    double price;
    double amount;
    double fee;
  };

  struct Market : public Entity
  {
    enum State
    {
      stopped,
      starting,
      running,
    };

    quint32 marketAdapterId;
    quint8 state;
    char userName[33];
    char key[65];
    char secret[65];
  };

  struct MarketBalance : public Entity
  {
    double reservedUsd;
    double reservedBtc;
    double availableUsd;
    double availableBtc;
    double fee;
  };

  struct SessionLogMessage : public Entity
  {
    qint64 date;
    char message[129];
  };

  struct ControlSession : public Entity
  {
    enum Command
    {
      startSimulation,
      stop,
      select,
    };

    quint8 cmd;
  };

  struct ControlSessionResponse : public Entity
  {
    quint8 cmd;
    quint8 success;
  };

  struct ControlMarket : public Entity
  {
    enum Command
    {
      select,
      refreshTransactions,
      refreshOrders,
    };

    quint8 cmd;
  };

  struct ControlMarketResponse : public Entity
  {
    quint8 cmd;
    quint8 success;
  };

#pragma pack(pop)

  template<size_t N> static void setString(char(&str)[N], const QString& value)
  {
    QByteArray buf = value.toUtf8();
    size_t size = buf.length() + 1;
    if(size > N - 1)
      size = N - 1;
    qMemCopy(str, buf.constData(), size);
    str[N - 1] = '\0';
  }
  
  template<size_t N> static QString getString(char(&str)[N])
  {
    str[N - 1] = '\0';
    return str;
  }
};
