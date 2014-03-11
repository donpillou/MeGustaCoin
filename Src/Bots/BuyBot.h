
#pragma once

class BuyBot : public Bot
{
public:

private:
  class Session : public Bot::Session
  {
  public:
    struct Parameters
    {
      double sellProfitGain;
      double buyProfitGain;
      //double sellPriceGain;
      //double buyPriceGain;
    };

    Session(Market& market);

  private:
    Market& market;

    Parameters parameters;

    double balanceUsd;
    double balanceBtc;

    virtual ~Session() {}

    virtual void setParameters(double* parameters);

    virtual void handle(const DataProtocol::Trade& trade, const Values& values);
    virtual void handleBuy(const Market::Transaction& transaction);
    virtual void handleSell(const Market::Transaction& transaction);

    bool isGoodBuy(const Values& values);
    bool isVeryGoodBuy(const Values& values);
    bool isGoodSell(const Values& values);
    bool isVeryGoodSell(const Values& values);

    void checkBuy(const DataProtocol::Trade& trade, const Values& values);
    void checkSell(const DataProtocol::Trade& trade, const Values& values);
  };

  virtual Session* createSession(Market& market) {return new Session(market);};
  virtual unsigned int getParameterCount() const {return sizeof(Session::Parameters) / sizeof(double);}
};
