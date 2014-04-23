
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
    transaction,
    order,
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
      inactive,
      active,
      simulating,
    };

    char name[33];
    quint32 engineId;
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
    char name[33];
    char currencyBase[33];
    char currencyComm[33];
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

#pragma pack(pop)

};
