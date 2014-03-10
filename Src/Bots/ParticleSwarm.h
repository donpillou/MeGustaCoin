
#pragma once

#include <vector>

class ParticleSwarm
{
public:
  ParticleSwarm(unsigned int particleCount = 8,
                double velocityFactor = 0.6f, double bestPositionFactor = 0.8f,
                double globalBestPositionFactor = 1.f, double randomPositionFactor = 0.02f);

  void addDimension(double& variable, double min, double max);

  void start();

  bool isRunning() const;

  void setRating(double rating);

  double getBestRating() const;

  void next();

private:
  class Particle
  {
  public:
    Particle() {}

    std::vector<double> position;
    std::vector<double> velocity;

    std::vector<double> bestPosition;
    double bestFitness;

    bool rated;
  };

  class Dimension
  {
  public:
    double* variable;
    double min;
    double max;

    Dimension(double* variable, double min, double max) : variable(variable), min(min), max(max) {}

    bool isInside(double t) const {return min <= max ? t >= min && t <= max : t >= min || t <= max;}
    double limit(double t) const {return t < min ? min : t > max ? max : t;}
  };

  unsigned int particleCount;
  double velocityFactor;
  double bestPositionFactor;
  double globalBestPositionFactor;
  double randomPositionFactor;

  std::vector<Dimension> dimensions;

  std::vector<Particle> particles;
  unsigned int bestParticleIndex;
  unsigned int currentParticleIndex;

  void updateParticle(Particle& particle);
};
