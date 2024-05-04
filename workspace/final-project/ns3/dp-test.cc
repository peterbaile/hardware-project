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

void TraceTxDrop(uint32_t s1, uint32_t s2, Ptr<const Packet> packet) {
    FlowIdTag tag;
    if (packet->PeekPacketTag(tag) == true) {
        uint32_t tagId = (uint32_t) tag.GetFlowId();
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " TxDropData " << s1 << " " << s2 << " " << tagId << " " << packet->GetSize());
    } else {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " TxDropAck " << s1 << " " << s2);
    }
}

void TraceRxDrop(uint32_t s1, uint32_t s2, Ptr<const Packet> packet) {
    FlowIdTag tag;
    if (packet->PeekPacketTag(tag) == true) {
        uint32_t tagId = (uint32_t) tag.GetFlowId();
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " RxDropData " << s1 << " " << s2 << " " << tagId << " " << packet->GetSize());
    } else {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " RxDropAck " << s1 << " " << s2);
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
    Config::SetDefault("ns3::TcpSocket::DelAckCount", UintegerValue(0));
    Config::SetDefault("ns3::TcpSocket::DelAckTimeout", TimeValue(Seconds(0.0)));
	LogComponentEnable("DataParallel", LOG_LEVEL_INFO);
    LogComponentEnable("TcpSocketBase", LOG_LEVEL_INFO);
    LogComponentEnable("MLTcpReno", LOG_LEVEL_INFO);
    LogComponentEnable("custom", LOG_LEVEL_INFO);
	
    NS_LOG_INFO("Building device topology");
	NodeContainer allNodes;
	allNodes.Create(6);

    NS_LOG_INFO("Adding link between switches");
    std::string dataRate = "50Gbps";
    std::string latency = "5us";
    std::string bufferSize = "2MB";

    NodeContainer s1s2;
    s1s2.Add(allNodes.Get(2));
    s1s2.Add(allNodes.Get(3));
    NetDeviceContainer s1s2Device;
	PointToPointHelper pointToPoint;
	pointToPoint.SetDeviceAttribute("DataRate", StringValue(dataRate));
	pointToPoint.SetChannelAttribute("Delay", StringValue(latency));
    //pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue("1p"));
    pointToPoint.SetQueue("ns3::DropTailQueue", "MaxSize", StringValue(bufferSize));
    pointToPoint.DisableFlowControl();
    s1s2Device = pointToPoint.Install(s1s2);
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 2, 3));
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 3, 2));
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(0))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 2, 3));
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 3, 2));
    //Ptr<DropTailQueue<Packet>> queue = DynamicCast<DropTailQueue<Packet>>(DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->GetQueue());
    //queue->TraceConnectWithoutContext("BytesInQueue", MakeCallback(TraceQueue));
    //queue->TraceConnectWithoutContext("PacketsInQueue", MakeCallback(TraceQueuePackets));
    //qptr = queue;
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(1))->SetPromiscReceiveCallback(MakeCallback(&TraceBottleneck));
    //DynamicCast<PointToPointNetDevice>(s1s2Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeCallback(&DropTrace));
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
    //DynamicCast<PointToPointNetDevice>(n0s1Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 0, 2));
    //DynamicCast<PointToPointNetDevice>(n0s1Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 2, 0));
    //DynamicCast<PointToPointNetDevice>(n0s1Device.Get(0))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 0, 2));
    //DynamicCast<PointToPointNetDevice>(n0s1Device.Get(1))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 2, 0));
    

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
    //DynamicCast<PointToPointNetDevice>(n1s1Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 1, 2));
    //DynamicCast<PointToPointNetDevice>(n1s1Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 2, 1));
    //DynamicCast<PointToPointNetDevice>(n1s1Device.Get(0))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 1, 2));
    //DynamicCast<PointToPointNetDevice>(n1s1Device.Get(1))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 2, 1));

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
    //DynamicCast<PointToPointNetDevice>(n4s2Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 4, 3));
    //DynamicCast<PointToPointNetDevice>(n4s2Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 3, 4));
    //DynamicCast<PointToPointNetDevice>(n4s2Device.Get(0))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 4, 3));
    //DynamicCast<PointToPointNetDevice>(n4s2Device.Get(1))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 3, 4));

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
    //DynamicCast<PointToPointNetDevice>(n5s2Device.Get(0))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 5, 3));
    //DynamicCast<PointToPointNetDevice>(n5s2Device.Get(1))->TraceConnectWithoutContext("MacTxDrop", MakeBoundCallback(TraceTxDrop, 3, 5));
    //DynamicCast<PointToPointNetDevice>(n5s2Device.Get(0))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 5, 3));
    //DynamicCast<PointToPointNetDevice>(n5s2Device.Get(1))->TraceConnectWithoutContext("MacRxDrop", MakeBoundCallback(TraceRxDrop, 3, 5));
	
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
    Time comp_time = Time("340ms");
    uint32_t comm_size = 2125000000;
    uint32_t end_time = 1000;
    uint32_t iters = 30;

    // Creating the addresses for the applications
    Ipv4Address ip0 = n0s1Interface.GetAddress(0);
    Ipv4Address ip1 = n1s1Interface.GetAddress(0);
    Ipv4Address ip4 = n4s2Interface.GetAddress(0);
    Ipv4Address ip5 = n5s2Interface.GetAddress(0);

    Address address0(InetSocketAddress(ip0, port));
    Address address1(InetSocketAddress(ip1, port));
    Address address4(InetSocketAddress(ip4, port));
    Address address5(InetSocketAddress(ip5, port));

    std::set<Ipv4Address> addresses = {ip0, ip1, ip4, ip5};
    Ipv4Address leader = ip0;

    // Creating ML schedules
    Ptr<MLSchedule> schedule0 = Create<MLSchedule>(addresses, ip0, leader, iters);
    Ptr<MLPhaseComp> compPhase = Create<MLPhaseComp>(comp_time);
    Ptr<MLPhaseSync> syncPhase = Create<MLPhaseSync>();
    std::list<Ipv4Address> list0;
    list0.push_back(InetSocketAddress::ConvertFrom(address4).GetIpv4());
    Ptr<MLPhaseComm> commPhase0 = Create<MLPhaseComm>(comm_size, list0, list0);
    schedule0->AddPhase(compPhase);
    //schedule0->AddPhase(syncPhase);
    schedule0->AddPhase(commPhase0);

    Ptr<MLSchedule> schedule1 = Create<MLSchedule>(addresses, ip1, leader, iters);
    std::list<Ipv4Address> list1;
    list1.push_back(InetSocketAddress::ConvertFrom(address5).GetIpv4());
    Ptr<MLPhaseComm> commPhase1 = Create<MLPhaseComm>(comm_size, list1, list1);
    schedule1->AddPhase(compPhase);
    //schedule1->AddPhase(syncPhase);
    schedule1->AddPhase(commPhase1);

    Ptr<MLSchedule> schedule4 = Create<MLSchedule>(addresses, ip4, leader, iters);
    std::list<Ipv4Address> list4;
    list4.push_back(InetSocketAddress::ConvertFrom(address0).GetIpv4());
    Ptr<MLPhaseComm> commPhase4 = Create<MLPhaseComm>(comm_size, list4, list4);
    schedule4->AddPhase(compPhase);
    //schedule4->AddPhase(syncPhase);
    schedule4->AddPhase(commPhase4);

    Ptr<MLSchedule> schedule5 = Create<MLSchedule>(addresses, ip5, leader, iters);
    std::list<Ipv4Address> list5;
    list5.push_back(InetSocketAddress::ConvertFrom(address1).GetIpv4());
    Ptr<MLPhaseComm> commPhase5 = Create<MLPhaseComm>(comm_size, list5, list5);
    schedule5->AddPhase(compPhase);
    //schedule5->AddPhase(syncPhase);
    schedule5->AddPhase(commPhase5);

    
    OnOffHelper onOffHelper4("ns3::TcpSocketFactory", address0);
    onOffHelper4.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper4.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper4.SetAttribute("Remote", AddressValue(address4));
    onOffHelper4.SetAttribute("DataRate", DataRateValue(DataRate("50Gb/s")));
    ApplicationContainer destApp = onOffHelper4.Install(allNodes.Get(0));
    destApp.Start(Seconds(0.0));
    destApp.Stop(Seconds(end_time));
    
    /*
    DataParallelHelper dataParallelHelper4("ns3::TcpSocketFactory", schedule4, address4);
    dataParallelHelper4.SetAttribute("FlowIdTag", UintegerValue(3));
    ApplicationContainer destApp = dataParallelHelper4.Install(allNodes.Get(4));
    destApp.Start(Seconds(0.0));
    destApp.Stop(Seconds(end_time));
    */
    
    PacketSinkHelper pktHelper("ns3::TcpSocketFactory", address4);
    ApplicationContainer srcApp = pktHelper.Install(allNodes.Get(4));
    srcApp.Start(Seconds(0.0));
    srcApp.Stop(Seconds(end_time));
    
    /*
	DataParallelHelper dataParallelHelper("ns3::TcpSocketFactory", schedule0, address0);
    dataParallelHelper.SetAttribute("FlowIdTag", UintegerValue(0));
	ApplicationContainer srcApp = dataParallelHelper.Install(allNodes.Get(0));
	srcApp.Start(Seconds(0.0));
	srcApp.Stop(Seconds(end_time));
    */
    OnOffHelper onOffHelper1("ns3::TcpSocketFactory", address1);
    onOffHelper1.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
    onOffHelper1.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
    onOffHelper1.SetAttribute("Remote", AddressValue(address5));
    onOffHelper1.SetAttribute("DataRate", DataRateValue(DataRate("50Gb/s")));
    ApplicationContainer destApp1 = onOffHelper1.Install(allNodes.Get(1));
    destApp1.Start(Seconds(0.0));
    destApp1.Stop(Seconds(end_time));
    
    PacketSinkHelper pktHelper5("ns3::TcpSocketFactory", address5);
    ApplicationContainer srcApp5 = pktHelper5.Install(allNodes.Get(5));
    srcApp5.Start(Seconds(0.0));
    srcApp5.Stop(Seconds(end_time));
    
    /* 
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
    */

    Ipv4GlobalRoutingHelper::PopulateRoutingTables();
    
	NS_LOG_INFO("Run Simulation");
	Simulator::Run();
	Simulator::Destroy();
	NS_LOG_INFO("Finished simulation");

	return 0;
}
