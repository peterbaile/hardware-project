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

NetDeviceContainer createLink(NodeContainer n, std::string dataRate, std::string latency, std::string queueSize) {
    NetDeviceContainer n0n2Device;
    PointToPointHelper pp02;
    pp02.SetDeviceAttribute("DataRate", StringValue(dataRate));
    pp02.SetChannelAttribute("Delay", StringValue(latency));
    pp02.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(queueSize));
    pp02.DisableFlowControl();
    n0n2Device = pp02.Install(n);
    return n0n2Device; 
}

void TraceQueue(uint32_t num, uint32_t oldValue, uint32_t newValue) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " q " << num << " " << newValue << " " << oldValue);
    //NS_LOG_INFO(Simulator::Now().GetSeconds() << " qdrop " << num << " " << qptr->GetTotalDroppedPackets());
} 

int main (int argc, char* argv[]) 
{
    Time::SetResolution(Time::NS);
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::MLTcpReno"));
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpLinuxReno"));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 30));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 30));
    Config::SetDefault("ns3::TcpSocket::SegmentSize", UintegerValue(9000));
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
	LogComponentEnable("DataParallel", LOG_LEVEL_INFO);
    LogComponentEnable("MLScheduler", LOG_LEVEL_INFO);
    LogComponentEnable("MLSchedule", LOG_LEVEL_INFO);
    LogComponentEnable("MLTcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("custom", LOG_LEVEL_INFO);

    NS_LOG_INFO("Building device topology");
	const uint32_t k = 6;
    const uint32_t oversub = 2;

    const uint32_t numSections = k;
    const uint32_t sizeRack = (float) oversub/(oversub + 1) * k;
    const uint32_t sizeSection = k - sizeRack;
    const uint32_t numCore = sizeSection * k / 2; //k * k / 4;
    //const uint32_t width = 4;
    //const uint32_t serversPerRack = 4;

    bool animation = false;

    NodeContainer allNodes;

    NodeContainer core;
    core.Create(numCore);
    allNodes.Add(core);

    std::vector<NodeContainer> sectionArray;
    std::vector<vector<NodeContainer>> racks;
    NodeContainer allRacks;
    for (uint32_t i = 0; i < numSections; i++) {
        NodeContainer section;
        section.Create(sizeSection);
        allNodes.Add(section);
        sectionArray.push_back(section);
        std::vector<NodeContainer> rackArray;
        racks.push_back(rackArray);
        for (uint32_t j = 0; j < k / 2; j++) {
            NodeContainer rack;
            rack.Create(sizeRack + 1);
            allNodes.Add(rack);
            racks.at(i).push_back(rack);
            //allRacks.Add(rack);
            for (uint32_t m = 0; m < sizeRack; m++) {
                allRacks.Add(rack.Get(m));
            }
        }
    }
    // Connection between core switches and section switches
    std::string coreDataRate = "50Gbps";
    std::string coreLatency = "5us";
    std::string coreQueueSize = "300MB";
    std::vector<NetDeviceContainer> coreLinks;
    for (uint32_t i = 0; i < core.GetN(); i++) {
        for (uint32_t j = 0; j < numSections; j++) {
            NodeContainer section = sectionArray.at(j);
            NodeContainer connection;
            connection.Add(core.Get(i));
            connection.Add(section.Get(i / (k / 2)));
            coreLinks.push_back(createLink(connection, coreDataRate, coreLatency, coreQueueSize));
        }
    }
    // Connection between rack switch and section switches
    std::string dataRate = "50Gbps";
    std::string delay = "5us";
    std::string queueSize = "300MB";
    std::vector<NetDeviceContainer> sectionLinks;
    for (uint32_t i = 0; i < numSections; i++) {
        NodeContainer section = sectionArray.at(i);
        std::vector<NodeContainer> rackArray = racks.at(i);
        for (uint32_t j = 0; j < sizeSection; j++) {
            for (uint32_t m = 0; m < k / 2; m++) {
                NodeContainer rack = rackArray.at(m);
                NodeContainer connection;
                connection.Add(section.Get(j));
                connection.Add(rack.Get(sizeRack));
                sectionLinks.push_back(createLink(connection, dataRate, delay, queueSize));
            }
        }
    }
    // Connection between rack servers and rack switch
    std::string switchRate = "50Gbps";
    std::string switchLatency = "5us";
    std::string switchQueueSize = "300MB";
    std::vector<NetDeviceContainer> serverLinks;
    uint32_t linkCounter = 0;
    for (uint32_t i = 0; i < k; i++) {
        std::vector<NodeContainer> rackArray = racks.at(i);
        for (uint32_t j = 0; j < k / 2; j++) {
            NodeContainer rack = rackArray.at(j);
            for (uint32_t m = 0; m < sizeRack; m++) {
                NodeContainer connection;
                connection.Add(rack.Get(m));
                connection.Add(rack.Get(sizeRack));
                NetDeviceContainer device = createLink(connection, switchRate, switchLatency, switchQueueSize);
                serverLinks.push_back(device);
                //Ptr<DropTailQueue<Packet>> queue = DynamicCast<DropTailQueue<Packet>>(DynamicCast<PointToPointNetDevice>(device.Get(1))->GetQueue());
                //queue->TraceConnectWithoutContext("BytesInQueue", MakeBoundCallback(TraceQueue, linkCounter));
                //DynamicCast<DropTailQueue<Packet>>(DynamicCast<PointToPointNetDevice>(device.Get(0))->GetQueue())->TraceConnectWithoutContext("BytesInQueue", MakeBoundCallback(TraceQueue, linkCounter + 1));
                linkCounter += 2;
                //queue->TraceConnectWithoutContext("BytesInQueue", MakeCallback(TraceQueue));

            }
        }
    }

    InternetStackHelper internet;
    internet.Install(allNodes);

    NS_LOG_INFO("Assigning IP addresses");
    Ipv4AddressHelper address;
    for (uint32_t i = 0; i < coreLinks.size(); i++) {
        std::string addr = "10.200."  + to_string(i) + ".0";
        //char* char_array = new char[string_addr.length() + 1];
        //strcpy(char_array, addr);
        address.SetBase(addr.c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interface = address.Assign(coreLinks.at(i));
    }
    for (uint32_t i = 0; i < sectionLinks.size(); i++) {
        std::string addr = "10.190."  + to_string(i) + ".0";
        address.SetBase(addr.c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interface = address.Assign(sectionLinks.at(i));
    }
    std::vector<Ipv4Address> addresses;
    for (uint32_t i = 0; i < serverLinks.size(); i++) {
        uint32_t section = i / (k / 2 * sizeRack);
        uint32_t num = i % (k / 2 * sizeRack);
        std::string addr = "10." + to_string(section) + "." + to_string(num) + ".0";
        address.SetBase(addr.c_str(), "255.255.255.0");
        Ipv4InterfaceContainer interface = address.Assign(serverLinks.at(i));
        addresses.push_back(interface.GetAddress(0));
    }
    
    NS_LOG_INFO(addresses.at(0));
    NS_LOG_INFO(addresses.at(1));
    
	uint16_t port = 9;
    Time start_time = Seconds(0);
    Time end_time = Seconds(2000);

    
    NS_LOG_INFO("Starting data parallel applications");
    ApplicationContainer appContainer;
    
    for (uint32_t i = 0; i < addresses.size(); i++) {
        DataParallelHelper dataParallelHelper("ns3::TcpSocketFactory", nullptr, InetSocketAddress(addresses.at(i), port));
        //int nodesPerSection = (k * k / 4);
        //int section = i / nodesPerSection;
        //int rack = (i - section * nodesPerSection) / (k / 2);
        //int number = 
        ApplicationContainer app = dataParallelHelper.Install(allRacks.Get(i));
        app.Start(start_time);
        app.Stop(end_time);
        appContainer.Add(app);
    }
    
    NS_LOG_INFO("count " << appContainer.GetN());
    
    NodeContainer scheduleNode;
    scheduleNode.Create(1);
    
    Ptr<MLScheduler> scheduler = Create<MLScheduler>();
    ApplicationContainer schedulerContainer;
    schedulerContainer.Add(scheduler);
    scheduler->AddWorkerPool(appContainer);
    scheduleNode.Get(0)->AddApplication(scheduler);
    schedulerContainer.Start(start_time);
    schedulerContainer.Stop(end_time);
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
    if (animation) {
        MobilityHelper mobility;
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(allNodes);
        AnimationInterface anim("animation.xml");

        for (uint32_t i = 0; i < core.GetN(); i++) {
            Ptr<ConstantPositionMobilityModel> obj = core.Get(i)->GetObject<ConstantPositionMobilityModel>();
            obj->SetPosition(Vector(i * 20 + 5, 5.0, 0.0));
        }

        for (uint32_t i = 0; i < sectionArray.size(); i++) {
            NodeContainer section = sectionArray.at(i);
            for (uint32_t j = 0; j < section.GetN(); j++) {
                Ptr<ConstantPositionMobilityModel> obj = section.Get(j)->GetObject<ConstantPositionMobilityModel>();
                obj->SetPosition(Vector(i * 30 + j * 5, 15.0, 0.0));
            }
        }

        for (uint32_t i = 0; i < racks.size(); i++) {
            std::vector<NodeContainer> rackArray = racks.at(i);
            for (uint32_t j = 0; j < rackArray.size(); j++) {
                NodeContainer rack = rackArray.at(j);
                for (uint32_t m = 0; m < rack.GetN(); m++) {
                    Ptr<ConstantPositionMobilityModel> obj = rack.Get(m)->GetObject<ConstantPositionMobilityModel>();
                    if (m == rack.GetN() - 1) {
                        obj->SetPosition(Vector(i * 30 + j * 10 + m, 40.0, 0.0));
                    } else {
                        obj->SetPosition(Vector(i * 30 + j * 10 + m * 2, 50.0, 0.0));
                    }
                }
            }
        }
    }
    /*
    Ipv4GlobalRoutingHelper g;
    Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper>
    ("dynamic-global-routing.routes", std::ios::out);
    g.PrintRoutingTableAllAt (Seconds (0), routingStream);
    */

	NS_LOG_INFO("Run Simulation");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Finished simulation");

	return 0;
}
