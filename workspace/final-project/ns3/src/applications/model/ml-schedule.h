// made by Anton Zabreyko

#ifndef ML_SCHEDULE_H
#define ML_SCHEDULE_H

#include "ns3/object.h"
#include "ns3/address.h"
#include "ns3/application.h"
#include "data-parallel.h"

#include <map>

namespace ns3 {

class DataParallel;

class MLPhase : public Object {

public:
    static TypeId GetTypeId(void);
    
    MLPhase();
    virtual ~MLPhase();


};

class MLPhaseComp : public MLPhase {

public:

    MLPhaseComp(Time);
    static TypeId GetTypeId(void);

    Time GetCompTime();

private:
    Time m_compTime;

};

class MLPhaseComm : public MLPhase {

public:
    
    MLPhaseComm(uint32_t, std::list<Ipv4Address>, std::list<Ipv4Address>);
    static TypeId GetTypeId(void);

    uint32_t GetCommData();
    std::list<Ipv4Address> GetRcvList();
    std::list<Ipv4Address> GetSendList();
    void Reassign(std::list<Ipv4Address>, std::list<Ipv4Address>); //TODO: different permissions for this function?

private:
    uint32_t m_commData;
    std::list<Ipv4Address> m_rcv;
    std::list<Ipv4Address> m_send;
};

class MLPhaseSync : public MLPhase {

public:

    MLPhaseSync();
    static TypeId GetTypeId(void);

};

class MLPhaseSyncFunc : public MLPhase {

public:
    MLPhaseSyncFunc(Callback<void, Ptr<DataParallel>>);
    static TypeId GetTypeId(void);    

    Callback<void, Ptr<DataParallel>> GetNotifyFunc();

private:
    Callback<void, Ptr<DataParallel>> m_callback;

};

class MLSchedule : public Object {

public:
    static TypeId GetTypeId(void);

    MLSchedule();
    MLSchedule(std::set<Ipv4Address>, Ipv4Address, Ipv4Address, uint32_t); // ip addresses of all workers, iters really ought to be at the job level
    virtual ~MLSchedule();

    void AddPhase(Ptr<MLPhase>);
    void SetLeader(Ipv4Address); // this should perhaps be set at the worker level??

    Ptr<MLPhase> GetPhase(int);
    int Size();
    uint32_t Iters();
    uint32_t NumWorkers();
    std::set<Ipv4Address> GetPeers();
    std::set<Ipv4Address> GetDestinations();

    Time CalculateCompTime();
    uint32_t CalculateCommSize(); // TODO: comm size is probably a better name
    Ipv4Address Leader();
    bool AmLeader();

    void SetLocal(Ipv4Address); // should only be called by data parallel
    void SetWorkers(std::vector<Ipv4Address>);

private:
    std::set<Ipv4Address> m_workers;
    std::vector<Ptr<MLPhase>> m_phases;
    uint32_t m_iters;
    uint32_t m_numWorkers;
    Ipv4Address m_local; // should only be configured by the worker the schedule is given to
    Ipv4Address m_leader;

};

class MLJob : public Object {

public:
    static TypeId GetTypeId(void);
    
    MLJob();
    MLJob(std::string, std::set<Ipv4Address>);
    virtual ~MLJob();

    Ptr<MLSchedule> GetSchedule(Ipv4Address);
    void InsertSchedule(Ipv4Address, Ptr<MLSchedule> schedule);
    std::vector<Ipv4Address> GetWorkers();
    void Reassign(std::vector<Ipv4Address>);
    uint32_t NumNodes();
    std::string Name();

private:
    std::string m_name;
    std::set<Ipv4Address> m_workers;
    std::map<Ipv4Address, Ptr<MLSchedule>> m_scheduleMap;

};

} // namespace ns3

#endif /* ML_SCHEDULE_H */
