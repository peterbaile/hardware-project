// based on the on off application

#ifndef MLSCHEDULER_H
#define MLSCHEDULER_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/ml-schedule.h"
#include "ns3/data-parallel.h"
#include "ns3/application-container.h"

#include <deque>

namespace ns3 {

class MLScheduler : public Application {
public:
    static TypeId GetTypeId(void);

    MLScheduler();

    virtual ~MLScheduler();

    void AddWorkerPool(ApplicationContainer);
    void NotifyCompletion(Ptr<DataParallel>);
    void NotifySync(Ptr<DataParallel>);

protected:
    virtual void DoDispose(void);
private:
    virtual void StartApplication(void);    // Called at time specified by Start
    virtual void StopApplication(void);     // Called at time specified by Stop
    void CancelEvents();

    void ScheduleNextJob();
    void ProcessQueue();
    std::vector<uint32_t> FindWorkers(uint32_t);
    void StartWorkers(std::vector<uint32_t>, Ptr<MLJob>);
    bool AssignWorkers(Ptr<MLJob>);

    Ptr<MLJob> GetBasicJob();
    Ptr<MLJob> GetBasic(uint32_t, uint32_t);
    Ptr<MLJob> GetCamembertJob(uint32_t, uint32_t);
    Ptr<MLJob> GetRobertaJob(uint32_t, uint32_t);
    Ptr<MLJob> GetVggJob(uint32_t, uint32_t);
    Ptr<MLJob> GetResNetJob(uint32_t, uint32_t);
    Ptr<MLJob> GetDataParallelJob(uint32_t, Time, uint32_t, uint32_t, std::string);

private:
    std::vector<Ptr<DataParallel>> m_workers;
    //ApplicationContainer m_workers;
    std::deque<Ptr<MLJob>> m_jobQueue;
    EventId         m_nextArrival;     // Event id for next start or stop event
    uint32_t        m_nextJobId;
    std::map<Ptr<DataParallel>, uint32_t> m_workersJobMap;
    std::map<uint32_t, uint32_t>          m_jobWorkersMap;
    std::map<uint32_t, uint32_t>          m_jobSyncMap;
    std::map<uint32_t, std::vector<Ptr<DataParallel>>>        m_jobMap;
};

} // namespace ns3

#endif /* MLSCHEDULER_H */
