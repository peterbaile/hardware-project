// based on on off application

#include "ml-scheduler.h"
#include "ns3/log.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/socket.h"
#include "ns3/simulator.h"
#include "ns3/data-parallel.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/flow-id-tag.h"

#include <sstream>

NS_LOG_COMPONENT_DEFINE("MLScheduler");

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(MLScheduler);

TypeId MLScheduler::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::MLScheduler")
    .SetParent<Application>()
    .AddConstructor<MLScheduler>()
  ; 
  return tid;
}


MLScheduler::MLScheduler() {
    NS_LOG_FUNCTION_NOARGS();
    m_nextJobId = 0;
}

MLScheduler::~MLScheduler() {
    NS_LOG_FUNCTION_NOARGS();
}

void MLScheduler::DoDispose(void) {
    NS_LOG_FUNCTION_NOARGS();
     // chain up
    Application::DoDispose();
}

// Application Methods
void MLScheduler::StartApplication() {
    NS_LOG_FUNCTION_NOARGS();
    if (m_workers.size() > 0) {
        ScheduleNextJob();
    }
}

void MLScheduler::StopApplication() {
    NS_LOG_FUNCTION_NOARGS();
    CancelEvents();
}

void MLScheduler::CancelEvents() {
    Simulator::Cancel(m_nextArrival);
}

// TODO: the fact that this isnt in the header file and isnt static bothers me
void MLScheduler::NotifyCompletion(Ptr<DataParallel> worker) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << "MLWorkerFinish " << worker->GetAddress());
    uint32_t jobId = m_workersJobMap[worker];
    m_jobWorkersMap[jobId] -= 1;
    if (m_jobWorkersMap[jobId] == 0) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " MLJobFinish " << jobId);
        m_jobWorkersMap.erase(jobId);
        m_jobMap.erase(jobId);
        m_jobSyncMap.erase(jobId); // this needs to be changed
    }
}

void MLScheduler::NotifySync(Ptr<DataParallel> worker) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " MLWorkerSync " << worker->GetAddress());
    uint32_t jobId = m_workersJobMap[worker];
    m_jobSyncMap[jobId] += 1;
    if (m_jobSyncMap[jobId] == m_jobMap[jobId].size()) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " MLJobFinishSync " << jobId);
        std::vector<Ptr<DataParallel>> workers = m_jobMap[jobId];
        for (uint32_t i = 0; i < workers.size(); i++) {
            workers.at(i)->EndSyncFuncPhase();
        }
        m_jobSyncMap[jobId] = 0;
    }
    //uint32_t jobId = m_workersJobMap[worker];
    //m_jobSyncMap[jobId] += 1;
    //if (m_jobSyncMap[jobId] == numWorkers) {
        // tell all workers to continue
    //}
}

void MLScheduler::AddWorkerPool(ApplicationContainer workers) {
    for (uint32_t i = 0; i < workers.GetN(); i++) {
        Ptr<DataParallel> dp = DynamicCast<DataParallel>(workers.Get(i));
        if (dp) {
            dp->SetNotifyCallback(MakeCallback(&MLScheduler::NotifyCompletion, this));
            m_workers.push_back(dp);
        }
    }
}

void MLScheduler::ScheduleNextJob() {
    //Ptr<MLJob> job = GetBasicJob(); // TODO: make this actually sample the philly trace
    Ptr<MLJob> roberta = GetRobertaJob(5, 10);
    Ptr<MLJob> camembert = GetCamembertJob(5, 10);
    std::vector<uint32_t> vec = {0, 1, 2, 3, 4};
    std::vector<uint32_t> vec2 = {7, 8, 9, 10, 11};
    StartWorkers(vec, roberta);
    StartWorkers(vec2, camembert);
    
    //ProcessQueue();
    //m_nextArrival = Simulator::Schedule(Seconds(1), &MLScheduler::ScheduleNextJob, this);
}

void MLScheduler::ProcessQueue() {
    while (m_jobQueue.size() > 0 && AssignWorkers(m_jobQueue.at(0))) {
        m_jobQueue.pop_front();
    }
}

std::vector<uint32_t> MLScheduler::FindWorkers(uint32_t numWorkers) {
    std::vector<uint32_t> assignedWorkers;
    bool workers[m_workers.size()];
    for (uint32_t i = 0; i < m_workers.size(); i++) {
        workers[i] = !(m_workers.at(i)->Busy());
    }

    for (uint32_t i = 0; i < m_workers.size() - numWorkers; i++) {
        bool valid = true;
        for (uint32_t j = i; j < i + numWorkers; j++) {
            if (!workers[j]) {
                valid = false;
            }
        }
        if (valid) {
            for (uint32_t j = i; j < i + numWorkers; j++) {
                assignedWorkers.push_back(j);
            }
            return assignedWorkers;
        }
    }
    //TODO: do best effort assignment

    return assignedWorkers; 

}

bool MLScheduler::AssignWorkers(Ptr<MLJob> job) {
    uint32_t num_workers = job->NumNodes();
    std::vector<uint32_t> assignedWorkers = FindWorkers(num_workers);
    if (assignedWorkers.size() == 0) {
        return false;
    }
    StartWorkers(assignedWorkers, job);
    return true;
}

void MLScheduler::StartWorkers(std::vector<uint32_t> assignedWorkers, Ptr<MLJob> job) {
    std::vector<Ipv4Address> addresses;
    std::vector<Ptr<DataParallel>> vector;
    for (uint32_t i = 0; i < assignedWorkers.size(); i++) {
        Ptr<DataParallel> worker = m_workers.at(assignedWorkers.at(i));
        addresses.push_back(worker->GetAddress());
        m_workersJobMap[worker] = m_nextJobId;
        vector.push_back(worker);
    }
    job->Reassign(addresses);
    for (uint32_t i = 0; i < assignedWorkers.size(); i++) {
        Ptr<DataParallel> worker = m_workers.at(assignedWorkers.at(i));
        worker->AddSchedule(job->GetSchedule(addresses.at(i)));
    }
    m_jobWorkersMap[m_nextJobId] = job->NumNodes();
    m_jobSyncMap[m_nextJobId] = 0;
    m_jobMap[m_nextJobId] = vector;

    std::ostringstream oss;
    oss << Simulator::Now().GetSeconds() << " MLJobStart " << m_nextJobId << " " << job->Name();
    for (uint32_t i = 0; i < assignedWorkers.size(); i++) {
        oss << " " << assignedWorkers.at(i);
    }
    oss << " IP";
    for (uint32_t i = 0; i < addresses.size(); i++) {
        oss << " " << addresses.at(i);
    }
    
    NS_LOG_INFO(oss.str());
    m_nextJobId += 1;
}

Ptr<MLJob> MLScheduler::GetBasicJob() {
    Time comp_time = Time("400ms"); 
    uint32_t comm_size = 10000;
    uint32_t iters = 10;
    uint32_t ip0 = 0;
    uint32_t ip1 = 1;

    std::set<Ipv4Address> addresses;
    addresses.insert(Ipv4Address(ip0));
    addresses.insert(Ipv4Address(ip1));

    Ptr<MLSchedule> schedule0 = Create<MLSchedule>(addresses, Ipv4Address(ip0), Ipv4Address(ip0), iters);
    Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(comp_time);
    std::list<Ipv4Address> list0;
    list0.push_back(Ipv4Address(ip1));
    Ptr<MLPhaseComm> commPhase0 = Create<MLPhaseComm>(comm_size, list0, list0);
    schedule0->AddPhase(compPhase);
    schedule0->AddPhase(commPhase0);

    Ptr<MLSchedule> schedule1 = Create<MLSchedule>(addresses, Ipv4Address(ip1), Ipv4Address(ip0), iters);
    std::list<Ipv4Address> list1;
    list1.push_back(Ipv4Address(ip0));
    Ptr<MLPhaseComm> commPhase1 = Create<MLPhaseComm>(comm_size, list1, list1);
    schedule1->AddPhase(compPhase);
    schedule1->AddPhase(commPhase1);

    Ptr<MLJob> job = Create<MLJob>("BasicJob", addresses);
    job->InsertSchedule(Ipv4Address(ip0), schedule0);
    job->InsertSchedule(Ipv4Address(ip1), schedule1);
    return job;
}

Ptr<MLJob> MLScheduler::GetBasic(uint32_t numWorkers, uint32_t iters) {
    return GetDataParallelJob(numWorkers, Time("100ms"), 25000, iters, "Basic");
}

Ptr<MLJob> MLScheduler::GetCamembertJob(uint32_t numWorkers, uint32_t iters) {
    return GetDataParallelJob(numWorkers, Time("130ms"), 844000000, iters, "Camembert");
}

Ptr<MLJob> MLScheduler::GetRobertaJob(uint32_t numWorkers, uint32_t iters) {
    return GetDataParallelJob(numWorkers, Time("139ms"), 845000000, iters, "Roberta");
}

Ptr<MLJob> MLScheduler::GetVggJob(uint32_t numWorkers, uint32_t iters) {
    return GetDataParallelJob(numWorkers, Time("29ms"), 987000000, iters, "Vgg16");
}

Ptr<MLJob> MLScheduler::GetResNetJob(uint32_t numWorkers, uint32_t iters) {
    return GetDataParallelJob(numWorkers, Time("160ms"), 149000000, iters, "ResNet");
}

Ptr<MLJob> MLScheduler::GetDataParallelJob(uint32_t numWorkers, Time compTime, uint32_t commSize, uint32_t iters, std::string name) {
    std::vector<Ipv4Address> ips; 
    for (uint32_t i = 0; i < numWorkers; i++) {
        ips.push_back(Ipv4Address(i));
    }
    std::set<Ipv4Address> addresses(ips.begin(), ips.end());

    Ptr<MLJob> job = Create<MLJob>(name, addresses);
    for (uint32_t i = 0; i < numWorkers; i++) {
        Ptr<MLSchedule> schedule = Create<MLSchedule>(addresses, ips.at(i), ips.at(0), iters);
        Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(compTime);
        schedule->AddPhase(compPhase);
        Ptr<MLPhaseSyncFunc> syncPhase = Create<MLPhaseSyncFunc>(MakeCallback(&MLScheduler::NotifySync, this));
        //schedule->AddPhase(syncPhase);
        uint32_t totalCommSteps = 2 * (numWorkers - 1);
        for (uint32_t j = 0; j < totalCommSteps; j++) {
            uint32_t stepSize = commSize / totalCommSteps;
            std::list<Ipv4Address> rcv;
            rcv.push_back(Ipv4Address(ips.at(((i + numWorkers) - 1) % numWorkers)));
            std::list<Ipv4Address> send;
            send.push_back(Ipv4Address(ips.at((i + 1) % numWorkers)));
            Ptr<MLPhaseComm> commPhase = Create<MLPhaseComm>(stepSize, send, rcv);
            schedule->AddPhase(syncPhase);
            schedule->AddPhase(commPhase);
        }
        job->InsertSchedule(Ipv4Address(ips[i]), schedule);
    }
    return job;
}

} // Namespace ns3
