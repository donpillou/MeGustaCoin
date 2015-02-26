
#pragma once

class Entity
{
public:
  class Listener
  {
  public:
    virtual void addedEntity(Entity& entity) {};
    virtual void addedEntity(Entity& entity, Entity& replacedEntity) {};
    virtual void updatedEntitiy(Entity& oldEntity, Entity& newEntity) {};
    virtual void removedEntity(Entity& entity) {};
    virtual void removedEntity(Entity& entity, Entity& newEntity) {};
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

    void delegateEntity(Entity& entity, Entity& replacedEntity)
    {
      quint32 replacedId = replacedEntity.getId();
      EntityTable& replacedTable = tables[replacedEntity.getType()];
      QHash<quint32, Entity*>::iterator itReplaced = replacedTable.entities.find(replacedId);
      if(itReplaced != replacedTable.entities.end())
      {
        replacedTable.entities.erase(itReplaced);
        for(QSet<Listener*>::iterator i = replacedTable.listeners.begin(), end = replacedTable.listeners.end(); i != end; ++i)
          (*i)->removedEntity(replacedEntity, entity);
      }

      quint32 newId = entity.getId();
      EntityTable& newTable = tables[entity.getType()];
      QHash<quint32, Entity*>::iterator it = newTable.entities.find(newId);
      if(it == newTable.entities.end())
      { // move replacing entity to new entity
        newTable.entities.insert(newId, &entity);
        for(QSet<Listener*>::iterator i = newTable.listeners.begin(), end = newTable.listeners.end(); i != end; ++i)
          (*i)->addedEntity(entity, replacedEntity);
        delete &replacedEntity;
      }
      else
      {
        // remove existing new entity, move old entity to new entity
        Entity* oldEntity = *it;
        newTable.entities.erase(it);
        for(QSet<Listener*>::iterator i = newTable.listeners.begin(), end = newTable.listeners.end(); i != end; ++i)
          (*i)->removedEntity(*oldEntity);
        newTable.entities.insert(newId, &entity);
        for(QSet<Listener*>::iterator i = newTable.listeners.begin(), end = newTable.listeners.end(); i != end; ++i)
          (*i)->addedEntity(entity, replacedEntity);
        delete oldEntity;
        delete &replacedEntity;
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
        table.entities.erase(it);
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->removedEntity(*entity);
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
        QHash<quint32, Entity*> entities;
        entities.swap(table.entities);
        for(QSet<Listener*>::iterator i = table.listeners.begin(), end = table.listeners.end(); i != end; ++i)
          (*i)->removedAll(type);
        qDeleteAll(entities);
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

    template<class C> quint32 getNewEntityId()
    {
      EntityTable& table = tables[(quint32)C::eType];
      if(table.entities.isEmpty())
        return 1;
      quint32 id = (--table.entities.end()).value()->getId();
      while(table.entities.contains(id))
        id += qrand() % 10;
      return id;
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
        //qDeleteAll(entities);
        for(QHash<quint32, Entity*>::Iterator i = entities.begin(), end = entities.end(); i != end; ++i)
        {
          Entity* e = *i;
          delete e;
        }
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
