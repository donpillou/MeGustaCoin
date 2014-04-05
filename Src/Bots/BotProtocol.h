
#pragma once

class BotProtocol
{
public:
  enum MessageType
  {
    loginRequest,
    loginResponse,
    authRequest,
    authResponse,
    createSessionRequest,
    createSessionResponse,
    registerBotRequest,
    registerBotResponse,
    updateEntity,
    removeEntity,
  };
  
  enum EntityType
  {
    error,
    session,
    engine,
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

  struct CreateSessionRequest
  {
    char name[33];
    char engine[33];
    double balanceBase;
    double balanceComm;
  };

  struct CreateSessionResponse
  {
    quint32 id;
  };
  
  struct Session
  {
    enum State
    {
      inactive,
      active,
    };

    char name[33];
    char engine[33];
    unsigned char state;
  };

  struct Engine
  {
    char name[33];
  };

  struct RegisterBotRequest
  {
    quint32 pid;
  };
  
  struct RegisterBotResponse
  {
    unsigned char isSimulation;
    double balanceBase;
    double balanceComm;
  };

#pragma pack(pop)

};
