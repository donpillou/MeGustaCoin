
#include "stdafx.h"

#include <cassert>

#include <limits>
#include <cstdlib>

#include "ParticleSwarm.h"

static inline double randomFloat() {return double(rand()) * (1.f / static_cast<double>(RAND_MAX));}


ParticleSwarm::ParticleSwarm(unsigned int particleCount,
                             double velocityFactor, double bestPositionFactor,
                             double globalBestPositionFactor, double randomPositionFactor) :
  particleCount(particleCount), velocityFactor(velocityFactor), bestPositionFactor(bestPositionFactor),
  globalBestPositionFactor(globalBestPositionFactor), randomPositionFactor(randomPositionFactor) {}

bool ParticleSwarm::isRunning() const
{
  return !particles.empty();
}

void ParticleSwarm::addDimension(double& variable, double min, double max)
{
  assert(particles.empty());
  assert(max > min);
  dimensions.push_back(Dimension(&variable, min, max));
}

void ParticleSwarm::start()
{
  assert(particles.empty());
  particles.resize(particleCount);
  for(unsigned int i = 0; i < particles.size(); ++i)
  {
    Particle& particle(particles[i]);
    particle.position.resize(dimensions.size());
    particle.velocity.resize(dimensions.size());
    for(unsigned int j = 0; j < dimensions.size(); ++j)
    {
      const Dimension& dimension = dimensions[j];
      double random = i == 0 ? 0.f : randomFloat(); // the first particles consists of the given start values
      particle.position[j] = *dimension.variable + (random >= 0.5f ?
                             (dimension.max - *dimension.variable) * (random - 0.5f) * 2.f : (dimension.min - *dimension.variable) * random * 2.f);
    }
    particle.bestPosition = particle.position;
    particle.bestFitness = std::numeric_limits<double>::max();
    particle.rated = false;
  }
  bestParticleIndex = 0;
  currentParticleIndex = particles.size() - 1;
}

void ParticleSwarm::next()
{
  currentParticleIndex = (currentParticleIndex + 1) % particles.size();
  Particle& particle = particles[currentParticleIndex];
  updateParticle(particle);
  for(int i = 0, count = dimensions.size(); i < count; ++i)
      *dimensions[i].variable = particle.position[i];
}

void ParticleSwarm::setRating(double rating)
{
  Particle& particle(particles[currentParticleIndex]);
  if(rating < particle.bestFitness)
  {
    particle.bestFitness = rating;
    particle.bestPosition = particle.position;
  }
  particle.rated = true;

  Particle& bestParticle(particles[bestParticleIndex]);
  if(rating < bestParticle.bestFitness)
    bestParticleIndex = currentParticleIndex;
}

double ParticleSwarm::getBestRating() const
{
  return particles[bestParticleIndex].bestFitness;
}

void ParticleSwarm::updateParticle(Particle& particle)
{
  if(!particle.rated)
    return;
  Particle& bestParticle(particles[bestParticleIndex]);
  for(unsigned int i = 0; i < particle.position.size(); ++i)
  {
    particle.velocity[i] = velocityFactor * particle.velocity[i]
                           + bestPositionFactor * randomFloat() * (particle.bestPosition[i] - particle.position[i])
                           + globalBestPositionFactor * randomFloat() * (bestParticle.bestPosition[i] - particle.position[i])
                           + randomPositionFactor * randomFloat() * ((dimensions[i].min + randomFloat() * (dimensions[i].max - dimensions[i].min)) - particle.position[i]);
    if(dimensions[i].isInside(particle.position[i] + particle.velocity[i]))
      particle.position[i] += particle.velocity[i];
    else
    {
      particle.position[i] = dimensions[i].limit(particle.position[i] + particle.velocity[i]);
      particle.velocity[i] = 0.;
    }
  }
  particle.rated = false;
}
