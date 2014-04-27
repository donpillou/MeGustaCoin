
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
    createEntity,
    requestEntities,
  };
  
  enum EntityType
  {
    error,
    session,
    engine,
    marketAdapter,
    sessionTransaction,
    sessionOrder,
    market,
  };

#pragma pack(push, 1)
  struct Header
  {
    quint32 size; // including header
    quint16 messageType; // MessageType
    quint16 entityType; // EntityType
    quint32 entityId;
  };

  struct Error
  {
    char errorMessage[129];
  };

  struct LoginRequest
  {
    char username[33];
  };

  struct LoginResponse
  {
    unsigned char userkey[32];
    unsigned char loginkey[32];
  };

  struct AuthRequest
  {
    unsigned char signature[32];
  };

  struct Session
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
  };

  struct Engine
  {
    char name[33];
  };

  struct MarketAdapter
  {
    char name[33];
    char currencyBase[33];
    char currencyComm[33];
  };

  struct Transaction
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

  struct Order
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

  struct Market
  {
    enum State
    {
      stopped,
      starting,
      running,
    };

    quint32 marketAdapterId;
    quint8 state;
  };

  struct RegisterBotRequest
  {
    quint32 pid;
  };
  
  struct RegisterBotResponse
  {
    quint8 isSimulation;
    double balanceBase;
    double balanceComm;
  };

  struct CreateSessionArgs
  {
    char name[33];
    quint32 engineId;
    quint32 marketId;
    double balanceBase;
    double balanceComm;
  };

  struct ControlSessionArgs
  {
    enum Command
    {
      startSimulation,
      stop,
      select,
    };

    quint8 cmd;
  };

  struct CreateMarketArgs
  {
    quint32 marketAdapterId;
    char username[33];
    char key[65];
    char secret[65];
  };

#pragma pack(pop)

};
