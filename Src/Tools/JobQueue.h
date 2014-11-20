
#pragma once

template <class T> class JobQueue
{
public:
  void append(const T& job, bool* wasEmpty)
  {
    QMutexLocker sync(&jobMutex);
    if(wasEmpty)
      *wasEmpty = jobs.empty();
    jobs.append(job);
    createdJobCondition.wakeAll();
  }

  void prepend(const T& job, bool* wasEmpty)
  {
    QMutexLocker sync(&jobMutex);
    if(wasEmpty)
      *wasEmpty = jobs.empty();
    jobs.append(job);
    createdJobCondition.wakeAll();
  }

  bool get(T& t, unsigned long timeout = ULONG_MAX)
  {
    QMutexLocker sync(&jobMutex);
    for(;;)
    {
      if(jobs.isEmpty())
      {
        if(timeout == 0)
          return false;
        createdJobCondition.wait(&jobMutex, timeout);
        if(jobs.isEmpty())
          continue;
      }
      t = jobs.front();
      jobs.pop_front();
      return true;
    }
  }

  QList<T> getAll()
  {
    QList<T> result;
    {
      QMutexLocker sync(&jobMutex);
      result.swap(jobs);
    }
    return result;
  }

private:
  QMutex jobMutex;
  QWaitCondition createdJobCondition;
  QList<T> jobs;
};
