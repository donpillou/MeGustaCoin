
#pragma once

class BotProtocol
{
public:
  enum MessageType
  {
    errorResponse,
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
    updateEntityResponse,
    removeEntity,
    removeEntityResponse,
    controlEntity,
    controlEntityResponse,
    createEntity,
    createEntityResponse,
    removeAllEntities,
  };
  
  enum EntityType
  {
    none,
    session,
    botEngine,
    marketAdapter,
    sessionTransaction,
    sessionOrder,
    sessionMarker,
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
    quint32 requestId;
  };

  struct Entity
  {
    quint16 entityType; // EntityType
    quint32 entityId;
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

  struct ErrorResponse : public Entity
  {
    quint16 messageType;
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
    qint64 timeout;
  };

  struct Marker : public Entity
  {
    enum Type
    {
      buy,
      sell,
      buyAttempt,
      sellAttempt,
    };

    quint8 type;
    qint64 date;
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
      startLive,
      stop,
      select,
      requestTransactions,
      requestOrders,
    };

    quint8 cmd;
  };

  struct ControlSessionResponse : public Entity
  {
    quint8 cmd;
  };

  struct ControlMarket : public Entity
  {
    enum Command
    {
      select,
      refreshTransactions,
      refreshOrders,
      refreshBalance,
    };

    quint8 cmd;
  };

  struct ControlMarketResponse : public Entity
  {
    quint8 cmd;
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
