#include "ns3/applications-module.h"
#include "ns3/core-module.h"
#include "ns3/internet-module.h"
#include "ns3/netanim-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/traffic-control-module.h"

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

int main (int argc, char* argv[]) 
{
    Time::SetResolution(Time::NS);
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpCubic"));
    //Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::TcpLinuxReno")); 
    Config::SetDefault("ns3::TcpL4Protocol::SocketType", StringValue("ns3::MLTcpReno"));
    Config::SetDefault("ns3::TcpSocket::RcvBufSize", UintegerValue(1 << 30));
    Config::SetDefault("ns3::TcpSocket::SndBufSize", UintegerValue(1 << 30));
    LogComponentEnable("PacketSink", LOG_LEVEL_INFO);
	LogComponentEnable("DataParallel", LOG_LEVEL_INFO);
    //LogComponentEnable("MLTcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("custom", LOG_LEVEL_INFO);
    LogComponentEnable("DefaultSimulatorImpl", LOG_LEVEL_INFO);
    LogComponentEnable("MapScheduler", LOG_LEVEL_INFO);
    LogComponentEnable("EventImpl", LOG_LEVEL_INFO);
	
    NS_LOG_INFO("Building device topology");
	NodeContainer allNodes;
	allNodes.Create(4);


    NS_LOG_INFO("Adding link between switches");
    std::string dataRate = "50Mbps";
    std::string latency = "10ms";
    std::string bufferSize = "375KB";
    
    NodeContainer n0n1;
    n0n1.Add(allNodes.Get(0));
    n0n1.Add(allNodes.Get(1));
    NetDeviceContainer n0n1Device;
	PointToPointHelper pointToPoint01;
	pointToPoint01.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint01.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint01.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint01.DisableFlowControl();
    n0n1Device = pointToPoint01.Install(n0n1);

    NodeContainer n1n2;
    n1n2.Add(allNodes.Get(1));
    n1n2.Add(allNodes.Get(2));
    NetDeviceContainer n1n2Device;
	PointToPointHelper pointToPoint12;
	pointToPoint12.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint12.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint12.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint12.DisableFlowControl();
    n1n2Device = pointToPoint12.Install(n1n2);

    NodeContainer n2n3;
    n2n3.Add(allNodes.Get(2));
    n2n3.Add(allNodes.Get(3));
    NetDeviceContainer n2n3Device;
	PointToPointHelper pointToPoint23;
	pointToPoint23.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint23.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint23.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint23.DisableFlowControl();
    n2n3Device = pointToPoint23.Install(n2n3);
    
    NodeContainer n3n0;
    n3n0.Add(allNodes.Get(3));
    n3n0.Add(allNodes.Get(0));
    NetDeviceContainer n3n0Device;
	PointToPointHelper pointToPoint30;
	pointToPoint30.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint30.SetChannelAttribute("Delay", StringValue(latency));
    pointToPoint30.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint30.DisableFlowControl();
    n3n0Device = pointToPoint30.Install(n3n0);
    
	InternetStackHelper internet;
	internet.Install(allNodes);

	NS_LOG_INFO("Assigning IP addresses");
	Ipv4AddressHelper address;
	address.SetBase("10.1.1.0", "255.255.255.0");
    Ipv4InterfaceContainer n0n1Interface = address.Assign(n0n1Device);
    address.SetBase("10.1.2.0", "255.255.255.0");
    Ipv4InterfaceContainer n1n2Interface = address.Assign(n1n2Device);
    address.SetBase("10.1.3.0", "255.255.255.0");
    Ipv4InterfaceContainer n2n3Interface = address.Assign(n2n3Device);
    address.SetBase("10.1.4.0", "255.255.255.0");
    Ipv4InterfaceContainer n3n0Interface = address.Assign(n3n0Device);

	uint16_t port = 9;

    NS_LOG_INFO("Starting data parallel applications");
    Time compTime = Time("5us");
    uint32_t comm_size = 12500;
    uint32_t end_time = 1.0;

    // Creating the addresses for the applications
    Address address0(InetSocketAddress(n0n1Interface.GetAddress(0), port));
    Address address1(InetSocketAddress(n1n2Interface.GetAddress(0), port));
    Address address2(InetSocketAddress(n2n3Interface.GetAddress(0), port));
    Address address3(InetSocketAddress(n3n0Interface.GetAddress(0), port));
    
    std::list<Ipv4Address> address_list0;
    address_list0.push_back(InetSocketAddress::ConvertFrom(address1).GetIpv4());
    
    std::list<Ipv4Address> address_list1;
    address_list1.push_back(InetSocketAddress::ConvertFrom(address2).GetIpv4());
    
    std::list<Ipv4Address> address_list2;
    address_list2.push_back(InetSocketAddress::ConvertFrom(address3).GetIpv4());
    
    std::list<Ipv4Address> address_list3;
    address_list3.push_back(InetSocketAddress::ConvertFrom(address0).GetIpv4());

    // Creating schedules for each node
    
    Ptr<MLSchedule> schedule0 = Create<MLSchedule>();
    Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(compTime);
    schedule0->AddPhase(compPhase);
    Ptr<MLPhaseComm> commPhase = Create<MLPhaseComm>(comm_size, address_list0, address_list2);
    schedule0->AddPhase(commPhase);
    schedule0->AddPhase(commPhase);
    schedule0->AddPhase(commPhase); 

    Ptr<MLSchedule> schedule1 = Create<MLSchedule>();
    compPhase = Create<MLPhaseComp>(compTime);
    schedule1->AddPhase(compPhase);
    commPhase = Create<MLPhaseComm>(comm_size, address_list1, address_list3);
    schedule1->AddPhase(commPhase);
    schedule1->AddPhase(commPhase);
    schedule1->AddPhase(commPhase);

    Ptr<MLSchedule> schedule2 = Create<MLSchedule>();
    compPhase = Create<MLPhaseComp>(compTime);
    schedule2->AddPhase(compPhase);
    commPhase = Create<MLPhaseComm>(comm_size, address_list2, address_list0);
    schedule2->AddPhase(commPhase);
    schedule2->AddPhase(commPhase);
    schedule2->AddPhase(commPhase);
    
    Ptr<MLSchedule> schedule3 = Create<MLSchedule>();
    compPhase = Create<MLPhaseComp>(compTime);
    schedule3->AddPhase(compPhase);
    commPhase = Create<MLPhaseComm>(comm_size, address_list3, address_list1);
    schedule3->AddPhase(commPhase);
    schedule3->AddPhase(commPhase);
    schedule3->AddPhase(commPhase);

    
    DataParallelHelper dataParallelHelper0("ns3::TcpSocketFactory", schedule0, address0);
    dataParallelHelper0.SetAttribute("FlowIdTag", UintegerValue(3));
    ApplicationContainer app0 = dataParallelHelper0.Install(allNodes.Get(0));
    app0.Start(Seconds(0.0));
    app0.Stop(Seconds(end_time));

	DataParallelHelper dataParallelHelper1("ns3::TcpSocketFactory", schedule1, address1);
    dataParallelHelper1.SetAttribute("FlowIdTag", UintegerValue(0));
	ApplicationContainer app1 = dataParallelHelper1.Install(allNodes.Get(1));
	app1.Start(Seconds(0.0));
	app1.Stop(Seconds(end_time));

	DataParallelHelper dataParallelHelper2("ns3::TcpSocketFactory", schedule2, address2);
    dataParallelHelper2.SetAttribute("FlowIdTag", UintegerValue(0));
	ApplicationContainer app2 = dataParallelHelper2.Install(allNodes.Get(2));
	app2.Start(Seconds(0.0));
	app2.Stop(Seconds(end_time));
    
	DataParallelHelper dataParallelHelper3("ns3::TcpSocketFactory", schedule3, address3);
    dataParallelHelper3.SetAttribute("FlowIdTag", UintegerValue(0));
	ApplicationContainer app3 = dataParallelHelper3.Install(allNodes.Get(3));
	app3.Start(Seconds(0.0));
	app3.Stop(Seconds(end_time));
    
    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
	
	
	NS_LOG_INFO("Run Simulation");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Finished simulation");

	return 0;
}
