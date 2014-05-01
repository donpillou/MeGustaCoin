
#pragma once

class Entity
{
public:
  class Listener
  {
  public:
    virtual void addedEntity(Entity& entity) {};
    virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity) {};
    virtual void removedEntity(Entity& entity) {};
    //virtual void addedAll(const QHash<quint32, Entity*>& entities) = 0;
    virtual void removedAll(quint32 type) {};
  };

  class Manager
  {
  public:
    void delegateEntity(Entity& entity)
    {
      quint32 id = entity.getId();
      EntityTable& table = tables[entity.getType()];
      QHash<quint32, Entity*>::iterator it = table.entities.find(id);
      if(it == table.entities.end())
      {
        table.entities.insert(id, &entity);
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->addedEntity(entity);
      }
      else
      {
        Entity* oldEntity = *it;
        *it = &entity;
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->updatedEntitiy(*oldEntity, entity);
        delete oldEntity;
      }
    }

    void updatedEntity(Entity& entity)
    {
      EntityTable& table = tables[entity.getType()];
      for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->updatedEntitiy(entity, entity);
    }
  
    void removeEntity(quint32 type, quint32 id)
    {
      EntityTable& table = tables[type];
      QHash<quint32, Entity*>::iterator it = table.entities.find(id);
      if(it != table.entities.end())
      {
        Entity* entity = *it;
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->removedEntity(*entity);
        table.entities.erase(it);
        delete entity;
      }
    }

    template <typename E> void removeEntity(E eType, quint32 id)
    {
      removeEntity((quint32)eType, id);
    }

    template <typename C> void removeEntity(quint32 id)
    {
      removeEntity((quint32)C::eType, id);
    }
  
    void removeAll(quint32 type)
    {
      EntityTable& table = tables[type];
      if(!table.entities.isEmpty())
      {
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->removedAll(type);
        qDeleteAll(table.entities);
        table.entities.clear();
      }
    }

    template <typename C> void removeAll()
    {
      removeAll((quint32)C::eType);
    }

    Entity* getEntity(quint32 type, quint32 id) const
    {
      QHash<quint32, EntityTable>::ConstIterator itTable = tables.find(type);
      if(itTable == tables.end())
        return 0;
      const EntityTable& table = *itTable;
      QHash<quint32, Entity*>::ConstIterator it = table.entities.find(id);
      if(it == table.entities.end())
        return 0;
      return *it;
    }

    template<class C> C* getEntity(quint32 id) const
    {
      return dynamic_cast<C*>(getEntity((quint32)C::eType, id));
    }

    template<class C> void getAllEntities(QList<C*>& entities) const
    {
      QHash<quint32, EntityTable>::ConstIterator itTable = tables.find((quint32)C::eType);
      if(itTable != tables.end())
      {
        const QHash<quint32, Entity*>& hashMap = itTable->entities;
        for (QHash<quint32, Entity*>::ConstIterator i = hashMap.begin(), end = hashMap.end(); i != end; ++i)
          entities.append(dynamic_cast<C*>(*i));
      }
    }

    void registerListener(quint32 type, Listener& listener)
    {
      EntityTable& table = tables[type];
      table.listeners.insert(&listener);
    }

    template<class C> void registerListener(Listener& listener)
    {
      registerListener((quint32)C::eType, listener);
    }

    void unregisterListener(quint32 type, Listener& listener)
    {
      EntityTable& table = tables[type];
      table.listeners.remove(&listener);
    }

    template<class C> void unregisterListener(Listener& listener)
    {
      unregisterListener((quint32)C::eType, listener);
    }

  private:
    struct EntityTable
    {
      QHash<quint32, Entity*> entities;
      QSet<Listener*> listeners;

      ~EntityTable()
      {
        qDeleteAll(entities);
      }
    };
    QHash<quint32, EntityTable> tables;
  };


public:
  template <typename E> Entity(E type, quint32 id) : type((quint32)type), id(id) {}
  virtual ~Entity() {}

  quint32 getType() const {return type;}
  quint32 getId() const {return id;}

private:
  quint32 type;
  quint32 id;
};
