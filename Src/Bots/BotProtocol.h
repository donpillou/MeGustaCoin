
#pragma once

class BotProtocol
{
public:
  enum MessageType
  {
    errorResponse,
    loginRequest,
    loginResponse,
    authRequest,
    authResponse,
    createSimSessionRequest,
    createSimSessionResponse,
    createSessionRequest,
    createSessionResponse,
    //simSessionMessage,
    //simSessionRemoveMessage,
    //sessionMessage,
    //sessionRemoveMessage,

    registerBotRequest,
    registerBotResponse,
  };

#pragma pack(push, 1)
  struct Header
  {
    quint32 size; // including header
    quint64 source; // client id
    quint64 destination; // client id
    quint16 messageType; // MessageType
  };

  struct ErrorResponse
  {
    quint16 messageType;
    char errorMessage[129];
  };

  struct LoginRequest
  {
    char username[33];
  };

  struct LoginResponse
  {
    unsigned char userkey[64];
    unsigned char loginkey[64];
  };

  struct AuthRequest
  {
    unsigned char signature[64];
  };

  struct CreateSimSessionRequest
  {
    char name[33];
    char engine[33];
    double balanceBase;
    double balanceComm;
  };

  struct CreateSimSessionResponse
  {
    quint32 id;
  };

  struct CreateSessionRequest
  {
    quint32 simSessionId;
    double balanceBase;
    double balanceComm;
  };

  struct CreateSessionResponse
  {
    quint32 id;
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
