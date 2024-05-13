// made by anton zabreyko


#include "ml-schedule.h"
#include "ns3/uinteger.h"

NS_LOG_COMPONENT_DEFINE("MLSchedule");

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(MLPhase);
NS_OBJECT_ENSURE_REGISTERED(MLPhaseComp);
NS_OBJECT_ENSURE_REGISTERED(MLPhaseComm);
NS_OBJECT_ENSURE_REGISTERED(MLPhaseSync);
NS_OBJECT_ENSURE_REGISTERED(MLPhaseSyncFunc);
NS_OBJECT_ENSURE_REGISTERED(MLSchedule);

TypeId MLSchedule::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLSchedule")
        .SetParent<Object>()
        .AddConstructor<MLSchedule>()
    ;
    return tid;
}

MLSchedule::MLSchedule() {
    NS_LOG_FUNCTION_NOARGS();
    m_iters = 0;
}

MLSchedule::MLSchedule(std::set<Ipv4Address> workers, Ipv4Address local, Ipv4Address leader, uint32_t iters) {
    m_workers = workers;
    m_local = local;
    m_leader = leader;
    m_iters = iters;
}

MLSchedule::~MLSchedule() {
    NS_LOG_FUNCTION_NOARGS();
}

void MLSchedule::AddPhase(Ptr<MLPhase> phase) {
    m_phases.push_back(phase);
}

void MLSchedule::SetLeader(Ipv4Address leader) {
    m_leader = leader; // TODO: check that the leader is in the list and throw exception if not
}

void MLSchedule::SetLocal(Ipv4Address local) {
    m_local = local; // TODO: same as above, and also let us privatize these
}

void MLSchedule::SetWorkers(std::vector<Ipv4Address> addresses) {
    m_workers = std::set<Ipv4Address>(addresses.begin(), addresses.end());
}

Ipv4Address MLSchedule::Leader() {
    return m_leader;
}

bool MLSchedule::AmLeader() {
    return m_leader == m_local;
}

uint32_t MLSchedule::NumWorkers() {
    return m_workers.size();
}

Ptr<MLPhase> MLSchedule::GetPhase(int index) {
    return m_phases.at(index);
}

int MLSchedule::Size() {
    return m_phases.size();
}

uint32_t MLSchedule::Iters() {
    return m_iters;
}

std::set<Ipv4Address> MLSchedule::GetPeers() {
    std::set<Ipv4Address> peers(m_workers.begin(), m_workers.end());
    peers.erase(m_local);
    return peers;
}

std::set<Ipv4Address> MLSchedule::GetDestinations() {
    if (Leader() == m_local) {
        return GetPeers();
    }
    std::set<Ipv4Address> destinations;
    for (int i = 0; i < this->Size(); i++) {
        Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(this->GetPhase(i));
        if (commPhase) {
            std::list<Ipv4Address> sendList = commPhase->GetSendList();
            for (Ipv4Address addr : sendList) {
                destinations.insert(addr);
            }
        }
    }
    destinations.insert(Leader());
    return destinations;
}

Time MLSchedule::CalculateCompTime() {
    Time compTime = Time("0s");
    for (int i = 0; i < Size(); i++) {
        Ptr<MLPhaseComp> compPhase = DynamicCast<MLPhaseComp>(GetPhase(i));
        if (compPhase) {
            if (compPhase->GetCompTime() > compTime) {
                compTime = compPhase->GetCompTime();
            }
        }
    }
    return compTime;
}

uint32_t MLSchedule::CalculateCommSize() {
    uint32_t commSize = 0;
    for (int i = 0; i < Size(); i++) {
        Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(GetPhase(i));
        if (commPhase) {
            commSize += commPhase->GetCommData();
        }
    }
    return commSize;
}

TypeId MLPhase::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhase")
        .SetParent<Object>()
        .AddConstructor<MLPhase>()
    ;
    return tid;
}

MLPhase::MLPhase() {
    NS_LOG_FUNCTION_NOARGS();
}

MLPhase::~MLPhase() {
    NS_LOG_FUNCTION_NOARGS();
}


MLPhaseComp::MLPhaseComp(Time compTime) {
    m_compTime = compTime;
}

Time MLPhaseComp::GetCompTime() {
    return m_compTime;
}

MLPhaseComm::MLPhaseComm(uint32_t commData, std::list<Ipv4Address> send, std::list<Ipv4Address> rcv) {
    m_commData = commData;
    m_send = send;
    m_rcv = rcv;
}

uint32_t MLPhaseComm::GetCommData() {
    return m_commData;
}

std::list<Ipv4Address> MLPhaseComm::GetRcvList() {
    return m_rcv;
}

std::list<Ipv4Address> MLPhaseComm::GetSendList() {
    return m_send;
}

void MLPhaseComm::Reassign(std::list<Ipv4Address> send, std::list<Ipv4Address> rcv) {
    m_send = send;
    m_rcv = rcv;
}

TypeId MLPhaseComp::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhaseComp")
        .SetParent<MLPhase>()
        .AddConstructor<MLPhase>()
        .AddAttribute("CompTime", "Time spent doing computation.",
            TimeValue(Time("1s")),
            MakeTimeAccessor(&MLPhaseComp::m_compTime),
            MakeTimeChecker())
    ;
    return tid;
}

TypeId MLPhaseComm::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhaseComm")
        .SetParent<MLPhase>()
        .AddConstructor<MLPhase>()
        .AddAttribute("CommData", "Data to transmit in communication.",
            UintegerValue(1000),
            MakeUintegerAccessor(&MLPhaseComm::m_commData),
            MakeUintegerChecker<uint32_t>())
    ;
    return tid;
}

MLPhaseSync::MLPhaseSync() {
    NS_LOG_FUNCTION_NOARGS();
}

TypeId MLPhaseSync::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhaseSync")
        .SetParent<MLPhase>()
        .AddConstructor<MLPhase>()
    ;
    return tid;
}

MLPhaseSyncFunc::MLPhaseSyncFunc(Callback<void, Ptr<DataParallel>> callback) {
    m_callback = callback;
}

TypeId MLPhaseSyncFunc::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhaseSyncFunc")
        .SetParent<MLPhase>()
        .AddConstructor<MLPhase>()
    ;
    return tid; // TODO: add proper attributes
}

Callback<void, Ptr<DataParallel>> MLPhaseSyncFunc::GetNotifyFunc() {
    return m_callback;
}

TypeId MLJob::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::MLPhaseJob")
        .SetParent<MLJob>()
        .AddConstructor<MLJob>() // TODO: what should this actually be
    ;
    return tid;
}

MLJob::MLJob() {
    NS_LOG_FUNCTION_NOARGS();
    m_name = "DefaultJob";
}

MLJob::MLJob(std::string name, std::set<Ipv4Address> workers) {
    m_name = name;
    m_workers = workers;
}

MLJob::~MLJob() {
    NS_LOG_FUNCTION_NOARGS();
}

uint32_t MLJob::NumNodes() {
    return m_workers.size();
}

std::string MLJob::Name() {
    return m_name;
}

Ptr<MLSchedule> MLJob::GetSchedule(Ipv4Address address) {
    return m_scheduleMap[address];
}

void MLJob::InsertSchedule(Ipv4Address addr, Ptr<MLSchedule> schedule) {
    m_scheduleMap[addr] = schedule; // TODO: throw exception if ip address is not in the set
}

std::vector<Ipv4Address> MLJob::GetWorkers() {
    std::vector<Ipv4Address> workers(m_workers.begin(), m_workers.end());
    return workers;
}


// TODO: possibly change this to be done in the schedule class
void MLJob::Reassign(std::vector<Ipv4Address> newAddresses) {
    // need to change local and leader and the set as well
    std::vector<Ipv4Address> allNodes = GetWorkers();
    std::map<Ipv4Address, Ipv4Address> conversionMap;
    for (size_t i = 0; i < allNodes.size(); i++) {
        conversionMap[allNodes.at(i)] = newAddresses.at(i);
    }
    for (size_t i = 0; i < allNodes.size(); i++) {
        m_scheduleMap[conversionMap[allNodes.at(i)]] = m_scheduleMap[allNodes.at(i)];
        m_scheduleMap.erase(allNodes.at(i));
        Ptr<MLSchedule> schedule = m_scheduleMap[conversionMap[allNodes.at(i)]];
        for (int j = 0; j < schedule->Size(); j++) {
            Ptr<MLPhase> phase = schedule->GetPhase(j);
            Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(phase);
            if (commPhase) {
                std::list<Ipv4Address> newRcv;
                for (Ipv4Address addr : commPhase->GetRcvList()) {
                    newRcv.push_back(conversionMap[addr]);
                }
                std::list<Ipv4Address> newSend;
                for (Ipv4Address addr : commPhase->GetSendList()) {
                    newSend.push_back(conversionMap[addr]);
                }
                commPhase->Reassign(newSend, newRcv);
            }
        }
        schedule->SetLeader(newAddresses.at(0));
        schedule->SetLocal(newAddresses.at(i));
        schedule->SetWorkers(newAddresses);
    }
}

} // Namespace ns3
