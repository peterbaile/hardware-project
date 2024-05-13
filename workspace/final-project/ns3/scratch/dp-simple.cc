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

std::vector<Ptr<DataParallel>> w1;
std::vector<Ptr<DataParallel>> w2;
uint32_t counter1 = 0;
uint32_t counter2 = 0;

void CheckSync(Ptr<DataParallel> worker) {
    if (std::find(w1.begin(), w1.end(), worker) != w1.end()) {
        counter1 += 1;
        if (counter1 == 2) {
            for (int i = 0; i < 2; i++) {
                w1.at(i)->EndSyncFuncPhase();
            }
            counter1 = 0;
        }
    } else {
        counter2 += 1;
        if (counter2 == 2) {
            for (int i = 0; i < 2; i++) {
                w2.at(i)->EndSyncFuncPhase();
            }
            counter2 = 0;
        }
    }

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
	LogComponentEnable("DataParallel", LOG_LEVEL_INFO);
    LogComponentEnable("MLTcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("custom", LOG_LEVEL_INFO);
	
    NS_LOG_INFO("Building device topology");
	NodeContainer allNodes;
	allNodes.Create(6);

    NS_LOG_INFO("Adding link between switches");
    std::string dataRate = "100Mbps";
    std::string latency = "10ms";
    std::string bufferSize = "1MB";
    std::string bottleBufferSize = "500KB";

    NodeContainer s1s2;
    s1s2.Add(allNodes.Get(2));
    s1s2.Add(allNodes.Get(3));
    NetDeviceContainer s1s2Device;
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint.SetChannelAttribute("Delay", StringValue(latency));
    //pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bottleBufferSize));
    pointToPoint.DisableFlowControl();
    s1s2Device = pointToPoint.Install(s1s2);
    Ptr<DropTailQueue<Packet>> queue = DynamicCast<DropTailQueue<Packet>>(DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->GetQueue());
    //queue->TraceConnectWithoutContext("BytesInQueue", MakeCallback(TraceQueue));
    
    //queue->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(TraceQueuePackets));
    qptr = queue;
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->SetPromiscReceiveCallback(MakeCallback(&TraceBottleneck));
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeCallback(&DropTrace));
    //queue->SetAttribute("MaxPackets", UintegerValue(1));
    //queue = DynamicCast<DropTailQueue<Packet>>(DynamicCast<PointToPointNetDevice>(s1s2.Get(1))->GetQueue());
    //queue->SetAttribute("MaxPackets", UintegerValue(1));

    NodeContainer n0s1;
    n0s1.Add(allNodes.Get(0));
    n0s1.Add(allNodes.Get(2));
    NetDeviceContainer n0s1Device;
	PointToPointHelper pointToPoint01;
	pointToPoint01.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint01.SetChannelAttribute("Delay", StringValue(latency));
    //pointToPoint01.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint01.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint01.DisableFlowControl();
    n0s1Device = pointToPoint.Install(n0s1);

    NodeContainer n1s1;
    n1s1.Add(allNodes.Get(1));
    n1s1.Add(allNodes.Get(2));
    NetDeviceContainer n1s1Device;
	PointToPointHelper pointToPoint11;
	pointToPoint11.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint11.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint11.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    //pointToPoint11.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint11.DisableFlowControl();
    n1s1Device = pointToPoint.Install(n1s1);

    NodeContainer n4s2;
    n4s2.Add(allNodes.Get(4));
    n4s2.Add(allNodes.Get(3));
    NetDeviceContainer n4s2Device;
	PointToPointHelper pointToPoint42;
	pointToPoint42.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint42.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint42.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    //pointToPoint42.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint42.DisableFlowControl();
    n4s2Device = pointToPoint.Install(n4s2);

    NodeContainer n5s2;
    n5s2.Add(allNodes.Get(5));
    n5s2.Add(allNodes.Get(3));
    NetDeviceContainer n5s2Device;
	PointToPointHelper pointToPoint56;
	pointToPoint56.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint56.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint56.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    //pointToPoint56.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint56.DisableFlowControl();
    n5s2Device = pointToPoint.Install(n5s2);
	
    NS_LOG_INFO("Installing Internet stack on all nodes");
	InternetStackHelper internet;
	internet.Install(allNodes);

	NS_LOG_INFO("Assigning IP addresses");
	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer n0s1Interface = address.Assign(n0s1Device);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer n1s1Interface = address.Assign(n1s1Device);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer s1s2Interface = address.Assign(s1s2Device);
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer n4s2Interface = address.Assign(n4s2Device);
    address.SetBase("10.1.5.0", "255.255.255.0");
    Ipv4InterfaceContainer n5s2Interface = address.Assign(n5s2Device);
    

    NS_LOG_INFO("Starting data parallel applications");
	uint16_t port = 9;
    Time comp_time = Time("5s");
    uint32_t comm_size = 25000000;
    uint32_t end_time = 1000;
    uint32_t iters = 10;

    // Creating the addresses for the applications
    Ipv4Address ip0 = n0s1Interface.GetAddress(0);
    Ipv4Address ip1 = n1s1Interface.GetAddress(0);
    Ipv4Address ip4 = n4s2Interface.GetAddress(0);
    Ipv4Address ip5 = n5s2Interface.GetAddress(0);
    std::set<Ipv4Address> addresses1 = {ip0, ip4};
    std::set<Ipv4Address> addresses2 = {ip1, ip5};
    Ipv4Address leader1 = ip0;
    Ipv4Address leader2 = ip1;

    Address address0(InetSocketAddress(ip0, port));
    Address address1(InetSocketAddress(ip1, port));
    Address address4(InetSocketAddress(ip4, port));
    Address address5(InetSocketAddress(ip5, port));

    bool sync = false;

    // Creating ML schedules
    Ptr<MLSchedule> schedule0 = Create<MLSchedule>(addresses1, ip0, leader1, iters);
    Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(comp_time);
    //Ptr<MLPhaseSyncFunc> syncPhase = Create<MLPhaseSyncFunc>(MakeCallback<void, Ptr<DataParallel>>(CheckSync));
    Ptr<MLPhaseSync> syncPhase = Create<MLPhaseSync>();
    std::list<Ipv4Address> list0;
    list0.push_back(InetSocketAddress::ConvertFrom(address4).GetIpv4());
    Ptr<MLPhaseComm> commPhase0 = Create<MLPhaseComm>(comm_size, list0, list0);
    schedule0->AddPhase(compPhase);
    if (sync) {
        schedule0->AddPhase(syncPhase);
    }
    schedule0->AddPhase(commPhase0);

    Ptr<MLSchedule> schedule1 = Create<MLSchedule>(addresses2, ip1, leader2, iters);
    std::list<Ipv4Address> list1;
    list1.push_back(InetSocketAddress::ConvertFrom(address5).GetIpv4());
    Ptr<MLPhaseComm> commPhase1 = Create<MLPhaseComm>(comm_size, list1, list1);
    schedule1->AddPhase(compPhase);
    if (sync) {
        schedule1->AddPhase(syncPhase);
    }
    schedule1->AddPhase(commPhase1);

    Ptr<MLSchedule> schedule4 = Create<MLSchedule>(addresses1, ip4, leader1, iters);
    std::list<Ipv4Address> list4;
    list4.push_back(InetSocketAddress::ConvertFrom(address0).GetIpv4());
    Ptr<MLPhaseComm> commPhase4 = Create<MLPhaseComm>(comm_size, list4, list4);
    schedule4->AddPhase(compPhase);
    if (sync) {
        schedule4->AddPhase(syncPhase);
    }
    schedule4->AddPhase(commPhase4);

    Ptr<MLSchedule> schedule5 = Create<MLSchedule>(addresses2, ip5, leader2, iters);
    std::list<Ipv4Address> list5;
    list5.push_back(InetSocketAddress::ConvertFrom(address1).GetIpv4());
    Ptr<MLPhaseComm> commPhase5 = Create<MLPhaseComm>(comm_size, list5, list5);
    schedule5->AddPhase(compPhase);
    if (sync) {
        schedule5->AddPhase(syncPhase);
    }
    schedule5->AddPhase(commPhase5);
    
    DataParallelHelper dataParallelHelper4("ns3::TcpSocketFactory", schedule4, address4);
    dataParallelHelper4.SetAttribute("FlowIdTag", UintegerValue(3));
    ApplicationContainer destApp = dataParallelHelper4.Install(allNodes.Get(4));
    destApp.Start(Seconds(0.0));
    destApp.Stop(Seconds(end_time));

	DataParallelHelper dataParallelHelper("ns3::TcpSocketFactory", schedule0, address0);
    dataParallelHelper.SetAttribute("FlowIdTag", UintegerValue(0));
	ApplicationContainer srcApp = dataParallelHelper.Install(allNodes.Get(0));
	srcApp.Start(Seconds(0.0));
	srcApp.Stop(Seconds(end_time));
    
    
    DataParallelHelper dataParallelHelper1("ns3::TcpSocketFactory", schedule1, address1);
    dataParallelHelper1.SetAttribute("FlowIdTag", UintegerValue(1));
    ApplicationContainer srcApp1 = dataParallelHelper1.Install(allNodes.Get(1));
    srcApp1.Start(Seconds(0.0));
    srcApp1.Stop(Seconds(end_time));
    
    DataParallelHelper dataParallelHelper5("ns3::TcpSocketFactory", schedule5, address5);
    dataParallelHelper5.SetAttribute("FlowIdTag", UintegerValue(2));
    ApplicationContainer srcApp5 = dataParallelHelper5.Install(allNodes.Get(5));
    srcApp5.Start(Seconds(0.0));
    srcApp5.Stop(Seconds(end_time)); 
    
    w1.push_back(DynamicCast<DataParallel>(destApp.Get(0)));
    w1.push_back(DynamicCast<DataParallel>(srcApp.Get(0)));
    w2.push_back(DynamicCast<DataParallel>(srcApp1.Get(0)));
    w2.push_back(DynamicCast<DataParallel>(srcApp5.Get(0)));

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
	NS_LOG_INFO("Run Simulation");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Finished simulation");

	return 0;
}
