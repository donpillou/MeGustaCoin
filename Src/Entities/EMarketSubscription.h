
#pragma once

class EMarketSubscription : public Entity
{
public:
  static const EType eType = EType::marketSubscription;

public:
  enum class State
  {
    subscribing,
    subscribed,
    unsubscribing,
    unsubscribed,
  };

public:
  EMarketSubscription(const QString& baseCurrency, const QString& commCurrency) : Entity(eType, 0)
  {
    this->state = State::unsubscribed;
    this->baseCurrency = baseCurrency;
    this->commCurrency = commCurrency;
  }

  //const QString& getChannelName() const {return channelName;}

  State getState() const {return state;}
  void setState(State state) {this->state = state;}

  QString getStateName() const
  {
    switch(state)
    {
    case State::subscribing:
      return QObject::tr("subscribing...");
    case State::subscribed:
      return QString();
    case State::unsubscribing:
    case State::unsubscribed:
      return QString();
    default:
      break;
    }
    Q_ASSERT(false);
    return QString();
  }

  QString formatAmount(double amount) const;
  QString formatPrice(double price) const;

  const QString& getBaseCurrency() const {return baseCurrency;}
  const QString& getCommCurrency() const {return commCurrency;}

private:
  State state;
  QString baseCurrency;
  QString commCurrency;
};
