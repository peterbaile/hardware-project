#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

#include "ns3/netanim-module.h"
#include "ns3/mobility-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("custom");

Ptr<DropTailQueue<Packet>> qptr;

void TraceEnqueue(Ptr<Packet> packet) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " q " << "enqueue");
}

void TraceDequeue(Ptr<Packet> packet) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " q " << "dequeue");
}

void TraceDrop(Ptr<Packet> packet) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " q " << "drop");
}

void TraceQueue(uint32_t oldValue, uint32_t newValue) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " q " << newValue);
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " qdrop " << qptr->GetTotalDroppedPackets());
} 

void TraceQueuePackets(uint32_t oldValue, uint32_t newValue) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " qbytes " << newValue);
}

bool TraceBottleneck(Ptr<NetDevice> nd, Ptr<const Packet> packet, uint16_t protocol, const Address& sender, const Address& receiver, NetDevice::PacketType type) {
    FlowIdTag tag;
    PacketIdTag pTag;
    if (packet->PeekPacketTag(tag) == true && packet->PeekPacketTag(pTag)) {
        uint32_t tagId = (uint32_t) tag.GetFlowId();
        uint32_t pTagId = (uint32_t) pTag.GetPacketId();
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " Link " << 0 << " " << tagId << " " << packet->GetSize() << " " << pTagId);
        return true;
    }
    return false;
}

void DropTrace(Ptr<Packet const> p) {
    NS_LOG_INFO("bonobo");
}

std::vector<Ptr<DataParallel>> w;
uint32_t counter = 0;
uint32_t neededVotes;

void CheckSync(Ptr<DataParallel> worker) {
    counter += 1;
    //NS_LOG_INFO(counter << " " << neededVotes << " this is the counter");
    if (counter == neededVotes) {
        for (uint32_t i = 0; i < neededVotes; i++) {
            w.at(i)->EndSyncFuncPhase();
        }
        counter = 0;
    }
}

uint32_t iterCounts;
uint32_t iter;
Time prevLayer;
void endIteration(Ptr<DataParallel> worker) {
    worker->EndSyncFuncPhase();
    //NS_LOG_INFO(Simulator::Now().GetSeconds() << " IterEnd " << Simulator::Now().GetSeconds() - prevLayer.GetSeconds());
    NS_LOG_INFO(iterCounts << "," << Simulator::Now().GetSeconds() - prevLayer.GetSeconds());
    prevLayer = Simulator::Now();
    iterCounts += 1;
    /*
    if (iterCounts == neededVotes) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " IterEnd " << iter << " \n";
        iterCounts = 0;
        for (int i = 0; i < neededVotes; i++) {
            w.at(i)->EndSyncFuncPhase();
        }
    }
    */
}

std::vector<std::string> readLine(std::istream& str) {
    std::vector<std::string> result;
    std::string line;
    std::getline(str, line);

    std::stringstream lineStream(line);
    std::string cell;

    while (std::getline(lineStream, cell, ',')) {
        result.push_back(cell);
    }
    return result;
}
struct LayerParams {
    double time;
    uint32_t size;
};

struct TPModel {
    uint32_t numWorkers;
    std::vector<struct LayerParams> layers;
};

std::string modelFile;
TPModel loadTPModel() {
    std::ifstream infile(modelFile);
    //std::ifstream infile("layers.csv");
    std::string line;
    std::vector<std::string> results = readLine(infile);
    uint32_t numWorkers = std::stoi(results.at(0).c_str()); 
    results = readLine(infile);
    std::vector<LayerParams> layers;
    struct TPModel model;
    while (results.size() > 0) {
        double time = std::stod(results.at(0).c_str());
        uint32_t size = std::stoi(results.at(1).c_str());
        results = readLine(infile);
        struct LayerParams params;
        params.time = time;
        params.size = size;
        layers.push_back(params);
    }
    model.numWorkers = numWorkers;
    model.layers = layers;
    return model;
}


int main (int argc, char* argv[]) 
{
    Time::SetResolution(Time::NS);
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpLinuxReno")); 
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::MLTcpReno"));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 30));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 30));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(9000));
	//LogComponentEnable("DataParallel", LOG_LEVEL_INFO);
    //LogComponentEnable("TcpLinuxReno", LOG_LEVEL_INFO);
    LogComponentEnable("custom", LOG_LEVEL_INFO);
    
    modelFile = argv[1]; 
    struct TPModel model = loadTPModel();
    uint32_t numWorkers = model.numWorkers;
    neededVotes = numWorkers;
    prevLayer = Simulator::Now();
	
    //NS_LOG_INFO("Building device topology");
	NodeContainer allNodes;
	allNodes.Create(numWorkers + 1);
	InternetStackHelper internet;
	internet.Install(allNodes);

    std::string dataRate = argv[2]; //"1Mbps";
    std::string latency = argv[3]; //"10ms";
    std::string bufferSize = "1MB";
    std::string bottleBufferSize = "1MB";

    std::vector<NetDeviceContainer> containerVector;
    Ipv4AddressHelper address;
    address.SetBase("10.1.1.0", "255.255.255.0");

    std::vector<Address> addressVector;
    std::vector<Ipv4Address> ipVector;
    std::set<Ipv4Address> addresses;
	uint16_t port = 9;

    for (uint32_t i = 0; i < numWorkers; i++) {
        NodeContainer nc;
        nc.Add(allNodes.Get(i));
        nc.Add(allNodes.Get(numWorkers));
        NetDeviceContainer device;
        PointToPointHelper pointToPoint;
        pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
        pointToPoint.SetChannelAttribute("Delay", StringValue(latency));
        pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bottleBufferSize));
        pointToPoint.DisableFlowControl();
        device = pointToPoint.Install(nc);
        Ipv4InterfaceContainer interface = address.Assign(device);
        address.NewNetwork();
        Ipv4Address ip = interface.GetAddress(0);
        ipVector.push_back(ip);
        addresses.insert(ip);
        Address address(InetSocketAddress(ip, port));
        addressVector.push_back(address);
    }
    Ipv4Address leader = ipVector.at(0);
    
    //NS_LOG_INFO("Starting data parallel applications");
    //Time comp_time = Time("5s");
    //uint32_t comm_size = 25000000;
    uint32_t end_time = 10000;
    uint32_t iters = 1;
    //uint32_t iters = 10;

    std::vector<Ptr<MLSchedule>> scheduleVector;
    for (uint32_t i = 0; i < numWorkers; i++) {
        Ptr<MLSchedule> schedule = Create<MLSchedule>(addresses, ipVector.at(i), leader, iters);
        for (uint32_t k = 0; k < model.layers.size(); k++) {
            struct LayerParams params = model.layers.at(k);
            std::ostringstream oss;
            oss << params.time << "s";
            Time comp_time = Time(oss.str());
            double comm_size = params.size;
            Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(comp_time);
            schedule->AddPhase(compPhase);
            //std::cout << "layer " << oss.str() << " " << comm_size << " " << numWorkers << "\n";
            for (uint32_t j = 0; j < 2 * (numWorkers - 1); j++) {
                Ptr<MLPhaseSyncFunc> syncPhase = Create<MLPhaseSyncFunc>(MakeCallback<void, Ptr<DataParallel>>(CheckSync));
                schedule->AddPhase(syncPhase);
                std::list<Ipv4Address> sendList;
                sendList.push_back(ipVector.at((i + 1) % numWorkers));
                std::list<Ipv4Address> rcvList;
                uint32_t prev = i - 1;
                if (i == 0) {
                    prev = numWorkers - 1;
                }
                rcvList.push_back(ipVector.at(prev));
                Ptr<MLPhaseComm> commPhase = Create<MLPhaseComm>(comm_size / numWorkers, sendList, rcvList);
                schedule->AddPhase(commPhase);
            }
            if (i == 0) {
                Ptr<MLPhaseSyncFunc> syncPhase = Create<MLPhaseSyncFunc>(MakeCallback<void, Ptr<DataParallel>>(endIteration));
                schedule->AddPhase(syncPhase);
            }
        }
        scheduleVector.push_back(schedule);
    }

    for (uint32_t i = 0; i < numWorkers; i++) {
        DataParallelHelper dataParallelHelper("ns3::TcpSocketFactory", scheduleVector.at(i), addressVector.at(i));
        dataParallelHelper.SetAttribute("FlowIdTag", UintegerValue(i));
        ApplicationContainer app = dataParallelHelper.Install(allNodes.Get(i));
        app.Start(Seconds(0.0));
        app.Stop(Seconds(end_time));
        w.push_back(DynamicCast<DataParallel>(app.Get(0)));
    }

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
	//NS_LOG_INFO("Run Simulation");
    NS_LOG_INFO("Layer #,Time");
	Simulator::Run();
	Simulator::Destroy();
	//NS_LOG_INFO("Finished simulation");

	return 0;
}
