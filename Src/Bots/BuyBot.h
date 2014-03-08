
#pragma once

class BuyBot : public Bot
{
public:

private:
  class Session : public Bot::Session
  {
  public:
    Session(Market& market) : market(market) {}

  private:
    Market& market;

    virtual ~Session() {}

    virtual void handle(const DataProtocol::Trade& trade, double* values);
  };

  virtual Session* createSession(Market& market) {return new Session(market);};

};
