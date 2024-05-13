/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
//
// Copyright (c) 2006 Georgia Tech Research Corporation
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License version 2 as
// published by the Free Software Foundation;
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
//
// Author: George F. Riley<riley@ece.gatech.edu>
// Edited by Anton Zabreyko based on on-off and packet sink applications

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#include "ns3/log.h"
#include "ns3/address.h"
#include "ns3/inet-socket-address.h"
#include "ns3/inet6-socket-address.h"
#include "ns3/node.h"
#include "ns3/nstime.h"
#include "ns3/data-rate.h"
#include "ns3/random-variable-stream.h"
#include "ns3/socket.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/simulator.h"
#include "ns3/socket-factory.h"
#include "ns3/packet.h"
#include "ns3/uinteger.h"
#include "ns3/trace-source-accessor.h"
#include "data-parallel.h"
#include "ns3/udp-socket-factory.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/flow-id-tag.h"

NS_LOG_COMPONENT_DEFINE("DataParallel");

using namespace std;

namespace ns3 {

NS_OBJECT_ENSURE_REGISTERED(DataParallel);

TypeId DataParallel::GetTypeId(void) {
  static TypeId tid = TypeId("ns3::DataParallel")
    .SetParent<Application>()
    .AddConstructor<DataParallel>()
    .AddAttribute("Schedule", "The schedule for the application.",
                  PointerValue(),
                  MakePointerAccessor(&DataParallel::m_schedule),
                  MakePointerChecker<MLSchedule>())
    .AddAttribute ("PacketSize", "The size of packets sent in on state",
                   UintegerValue(9000),
                   MakeUintegerAccessor(&DataParallel::m_pktSize),
                   MakeUintegerChecker<uint32_t>(1))
    .AddAttribute("Local", "The address of this node",
                   AddressValue(),
                   MakeAddressAccessor(&DataParallel::m_local),
                   MakeAddressChecker())
    .AddAttribute("FlowIdTag", "A number to tag packets with.",
                  UintegerValue(102),
                  MakeUintegerAccessor(&DataParallel::m_flowIdTag),
                  MakeUintegerChecker<uint32_t>())
    .AddAttribute ("Protocol", "The type of protocol to use.",
                   TypeIdValue(UdpSocketFactory::GetTypeId ()),
                   MakeTypeIdAccessor(&DataParallel::m_tid),
                   MakeTypeIdChecker())
  ;
  return tid;
}


DataParallel::DataParallel ()
{
  NS_LOG_FUNCTION_NOARGS ();
  //m_socket = 0;
  m_listenerSocket = 0;
  m_connected = false;
  m_residualBits = 0;
  m_lastStartTime = Seconds(0);
  m_totBytes = 0;
  m_packetsSent = 0;
  m_schedule = nullptr;
  m_currentState = 0;
  m_commData = 0;
  m_notifyCompletion = MakeCallback(&DataParallel::DefaultNotify, this);
}

DataParallel::~DataParallel()
{
    NS_LOG_FUNCTION_NOARGS();
}

Ipv4Address DataParallel::GetAddress() {
    return InetSocketAddress::ConvertFrom(m_local).GetIpv4();
}

void
DataParallel::DoDispose (void)
{
  NS_LOG_FUNCTION_NOARGS ();

  m_listenerSocket = 0;
  m_socketList.clear();

  // chain up
  Application::DoDispose ();
}

// Application Methods
void DataParallel::StartApplication () // Called at time specified by Start
{
  NS_LOG_FUNCTION_NOARGS ();

  // Close any exiting sockets
  CloseSockets();

  // Create the listener socket if not already
  
  if (!m_listenerSocket) {
    m_listenerSocket = Socket::CreateSocket(GetNode(), m_tid);
    m_listenerSocket->Bind(m_local);
    m_listenerSocket->Listen();
    m_listenerSocket->ShutdownSend();
    m_listenerSocket->SetAcceptCallback(
        MakeNullCallback<bool, Ptr<Socket>, const Address &>(),
        MakeCallback(&DataParallel::HandleAccept, this)
    );
    m_listenerSocket->SetCloseCallbacks(
        MakeCallback(&DataParallel::HandleSenderClose, this),
        MakeCallback(&DataParallel::HandleSenderError, this)
    );
  }
  if (m_schedule) {
    StartJob();  
  }
}

void DataParallel::StartJob() {
    CloseSockets(); // TODO: remove after fixing synchronization
  InetSocketAddress senderAddr(InetSocketAddress::ConvertFrom(m_local).GetIpv4());
  std::set<Ipv4Address> peers = m_schedule->GetDestinations();
  uint16_t port = InetSocketAddress::ConvertFrom(m_local).GetPort(); // TODO: better specification of port
  for (Ipv4Address addr : peers) {
      Ptr<Socket> socket = Socket::CreateSocket(GetNode(), m_tid);
      socket->Bind(senderAddr);
      socket->Connect(InetSocketAddress(addr, port));
      socket->SetAllowBroadcast(true);
      socket->SetDataSentCallback(MakeCallback(&DataParallel::CountSend, this));
      socket->SetSendCallback(MakeCallback(&DataParallel::SendData, this));
      m_socketMap[addr] = socket;
      Ptr<TcpSocketBase> tcpSocket = DynamicCast<TcpSocketBase>(socket);
      if (tcpSocket) {
          tcpSocket->ConfigureMLParameters(m_schedule->CalculateCompTime(), m_schedule->CalculateCommSize());
          tcpSocket->ConfigureLogging(GetAddress(), addr);
          //tcpSocket->TraceConnectWithoutContext("CongestionWindowAddress", MakeCallback(&DataParallel::TraceCwnd, this));
          //tcpSocket->TraceConnectWithoutContext("RTTAddress", MakeCallback(&DataParallel::TraceRTT, this));
      }
    }
    m_currentState = 0;
    m_currIter = 0;
    m_currVotes = 0;
    StartNextPhase();
}

void DataParallel::AddSchedule(Ptr<MLSchedule> schedule) {
    if (!Busy()) {
        m_schedule = schedule;
        //m_schedule->SetLocalAddress(GetAddress());
        StartJob();
    }
}

bool DataParallel::Busy() {
    return m_schedule != nullptr && (m_schedule->Iters() == 0 || m_currIter < m_schedule->Iters());
}

void DataParallel::TraceCwnd(uint32_t oldCwnd, uint32_t newCwnd, uint32_t local, uint16_t localPort, uint32_t peer, uint16_t peerPort) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPCwnd " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << InetSocketAddress::ConvertFrom(m_local).GetPort() << " " << Ipv4Address(peer) << " " << peerPort << " " << newCwnd);
}

void DataParallel::TraceRTT(Time oldCwnd, Time newCwnd, uint32_t local, uint16_t localPort, uint32_t peer, uint16_t peerPort) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPRTT " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << InetSocketAddress::ConvertFrom(m_local).GetPort() << " " << Ipv4Address(peer) << " " << peerPort << " " << newCwnd.GetMicroSeconds());
}

void DataParallel::CloseSockets() {
    while (!m_socketList.empty()) {
        Ptr<Socket> socket = m_socketList.front();
        m_socketList.pop_front();
        if (socket) {
            socket->Close();
        }
    }
    
    for (auto const& addrSockPair : m_socketMap) {
        Ptr<Socket> socket = addrSockPair.second;
        if (socket) {
            socket->Close();
        }
    }
    m_socketMap.clear();
}

void DataParallel::StopApplication () // Called at time specified by Stop
{
    NS_LOG_FUNCTION_NOARGS();
    CancelEvents();

    CloseSockets();
    if (m_listenerSocket) {
        m_listenerSocket->Close();
        m_listenerSocket->SetRecvCallback(MakeNullCallback<void, Ptr<Socket>>());
    
    }
}

void DataParallel::CancelEvents()
{
  NS_LOG_FUNCTION_NOARGS();

  if (m_sendEvent.IsRunning ())
    { // Cancel the pending send packet event
      // Calculate residual bits since last packet sent
      Time delta (Simulator::Now () - m_lastStartTime);
      int64x64_t bits = delta.To (Time::S) * m_cbrRate.GetBitRate ();
      m_residualBits += bits.GetHigh ();
    }
  Simulator::Cancel (m_sendEvent);
  Simulator::Cancel (m_startStopEvent);
}


void DataParallel::DefaultNotify(Ptr<DataParallel> ptr) {
    return;
}

void DataParallel::SetNotifyCallback(Callback<void, Ptr<DataParallel>> callback) {
    m_notifyCompletion = callback;
}

void DataParallel::StartNextPhase() {
    InitPhase(m_currentState);
    RunNextPhase();
}

void DataParallel::EndPhase(bool init) {
    m_currentState = (m_currentState + 1) % m_schedule->Size();
    if (m_currentState == 0) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPFinish " << GetAddress());
        m_currIter += 1;
    }
    if (!Busy()) {
        m_notifyCompletion(this);
        return;
    }
    if (init) {
        StartNextPhase();
    } else {
        RunNextPhase();
    }
}

void DataParallel::InitPhase(uint32_t phase) {
    Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(m_schedule->GetPhase(phase));
    if (commPhase) {
        InitCommPhase(phase);
    }
}

void DataParallel::RunNextPhase() {
    Ptr<MLPhase> currentPhase = m_schedule->GetPhase(m_currentState);
    if (DynamicCast<MLPhaseComp>(currentPhase)) { //TODO: change this to wokr based on inheritance instead?
        StartCompPhase();
    } else if (DynamicCast<MLPhaseComm>(currentPhase)) {
        StartCommPhase();
    }  else if (DynamicCast<MLPhaseSync>(currentPhase)) {
        StartSyncPhase();
    } else if (DynamicCast<MLPhaseSyncFunc>(currentPhase)) {
        StartSyncFuncPhase();
    } else {
        NS_LOG_ERROR("Unknown phase in data parallel application! Application halting.");
    }

}

void DataParallel::StartCompPhase() {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPCompStart " << GetAddress());
    Ptr<MLPhaseComp> compPhase = DynamicCast<MLPhaseComp>(m_schedule->GetPhase(m_currentState));
    m_startStopEvent = Simulator::Schedule(compPhase->GetCompTime(), &DataParallel::EndCompPhase, this);
    //m_currentState = (m_currentState + 1) % m_schedule->Size();
}

void DataParallel::EndCompPhase() {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPCompEnd " << GetAddress());
    EndPhase();
}

void DataParallel::InitCommPhase(uint32_t phase) {
    Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(m_schedule->GetPhase(phase));
    m_commData = 0; //commPhase->GetCommData();
    std::list<Ipv4Address> rcvList = commPhase->GetRcvList();
    std::list<Ipv4Address> sendList = commPhase->GetSendList();
    for (Ipv4Address addr : rcvList) {
        m_dataReceivedMap[addr] = 0;
    }
    for (Ipv4Address addr : sendList) {
        Ptr<Socket> socket = m_socketMap[addr];
        m_dataLoadedMap[addr] = 0;
        m_dataSentMap[addr] = commPhase->GetCommData(); //m_commData;
    }
    
}

void DataParallel::StartCommPhase() {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPCommStart " << GetAddress());
    Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(m_schedule->GetPhase(m_currentState));
    std::list<Ipv4Address> sendList = commPhase->GetSendList();
    m_commData = commPhase->GetCommData();
    for (Ipv4Address addr : sendList) {
        Ptr<Socket> socket = m_socketMap[addr];
        Ptr<Packet> packet = Create<Packet>(std::min(m_commData, m_pktSize));
        FlowIdTag tag;
        tag.SetFlowId(m_flowIdTag);
        packet->AddPacketTag(tag);
        NS_LOG_INFO("doing socket stuff");
        socket->Send(packet);
        NS_LOG_INFO("was it the socket");
        m_dataLoadedMap[addr] += packet->GetSize();
        NS_LOG_INFO(addr);
        //NS_LOG_INFO("sending the data " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << addr);
    }
    CheckEndComm();
}

// core assumption here is that a sync phase starts when i am no longer expecting any communication with me directly
void DataParallel::StartSyncPhase() {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPSyncStart " << GetAddress());
    InitPhase((m_currentState + 1) % m_schedule->Size());
    if (m_schedule->AmLeader()) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPLeader " << GetAddress());
        CheckSyncPhase();
    } else {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << GetAddress() << " sending sync packet");
        Ptr<Packet> packet = Create<Packet>(1);
        SyncTag tag(1);
        Ptr<Socket> socket = m_socketMap[m_schedule->Leader()];
        packet->AddByteTag(tag);
        socket->Send(packet);
    }
}

void DataParallel::CheckSyncPhase() {
    Ptr<MLPhaseSync> syncPhase = DynamicCast<MLPhaseSync>(m_schedule->GetPhase(m_currentState));
    if (!syncPhase) {
        return;
    }
    if (m_currVotes == m_schedule->NumWorkers() - 1) {
        EndSyncPhase();
    }
}

void DataParallel::EndSyncPhase() {
    // transmit a packet to all peers so that we can
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPSyncEnd " << GetAddress());
    Ptr<MLPhaseSync> syncPhase = DynamicCast<MLPhaseSync>(m_schedule->GetPhase(m_currentState));
    if (!syncPhase) {
        NS_LOG_INFO("Warning: Data Parallel job was told to end syncing while not in synchronization stage!");
        return;
    }
    if (m_schedule->AmLeader()) {
        m_currVotes = 0; 
        for (auto const& addrSockPair : m_socketMap) {
            Ptr<Socket> socket = addrSockPair.second;
            Ptr<Packet> packet = Create<Packet>(1);
            SyncTag tag(1);
            packet->AddByteTag(tag);
            socket->Send(packet);
            m_dataSentMap[addrSockPair.first] += 1; // hacky way of not having the sync packet count against the counter - this should probably be different? actually, it's not necessarily bad but it does mean that this isn't checked for until the next communication
        }
    }
    //m_currentState += 1;
    EndPhase(false);
    //StartNextPhase();
}

void DataParallel::StartSyncFuncPhase() {
    //NS_LOG_INFO("starting syncfunc phase " << GetAddress());
    //NS_LOG_INFO("ending sync func phase");
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPSyncStart " << GetAddress());
    InitPhase((m_currentState + 1) % m_schedule->Size());
    Ptr<MLPhaseSyncFunc> phase = DynamicCast<MLPhaseSyncFunc>(m_schedule->GetPhase(m_currentState));
    phase->GetNotifyFunc()(this);
}

void DataParallel::EndSyncFuncPhase() {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPSyncEnd " << GetAddress());
    EndPhase(false);
}

void DataParallel::SendData(Ptr<Socket> socket, uint32_t bufferSpace) {
    Address peer;
    socket->GetPeerName(peer);
    Ipv4Address peerAddress = InetSocketAddress::ConvertFrom(peer).GetIpv4();
    while (bufferSpace > 0 && m_dataLoadedMap[peerAddress] < m_commData) {
        uint32_t packetSize = std::min(m_commData - m_dataLoadedMap[peerAddress], std::min(bufferSpace, m_pktSize));
        Ptr<Packet> packet = Create<Packet>(packetSize);
        FlowIdTag tag;
        tag.SetFlowId(m_flowIdTag);
        packet->AddPacketTag(tag);
        socket->Send(packet);
        m_dataLoadedMap[peerAddress] += packetSize;
        bufferSpace -= packetSize;
    } 
}

void DataParallel::CountSend(Ptr<Socket> socket, uint32_t num) {
    Address peer;
    socket->GetPeerName(peer);
    Ipv4Address peerAddress = InetSocketAddress::ConvertFrom(peer).GetIpv4();
    if (DynamicCast<MLPhaseSync>(m_schedule->GetPhase(m_currentState))) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " sending synchronization packet " << GetAddress() << " " << " " << peerAddress << " " << num); 
        return;
    }
    m_dataSentMap[peerAddress] -= num;
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPTx " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << InetSocketAddress::ConvertFrom(m_local).GetPort() << " " << InetSocketAddress::ConvertFrom(peer).GetIpv4() << " " << InetSocketAddress::ConvertFrom(peer).GetPort() << " " << num << " " << m_dataSentMap[peerAddress]);
    CheckEndComm();
}

void DataParallel::ReadData(Ptr<Socket> socket) {
    Ptr<Packet> pkt = socket->Recv();
    if (!pkt) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << "Warning: received null packet!");
        return;
    }
    Address peer;
    socket->GetPeerName(peer);
    uint32_t totalData = pkt->GetSize();

    SyncTag tag;
    if (pkt->FindFirstMatchingByteTag(tag)) {
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPSync " << InetSocketAddress::ConvertFrom(peer).GetIpv4() << " " << InetSocketAddress::ConvertFrom(peer).GetPort() << " " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << InetSocketAddress::ConvertFrom(m_local).GetPort());
        if (m_schedule->AmLeader()) {
            m_currVotes += 1;
            CheckSyncPhase();
        } else {
            EndSyncPhase();
        }
        totalData -= 1;
        //return;
    }
    if (totalData > 0) {
        Ipv4Address peerAddress = InetSocketAddress::ConvertFrom(peer).GetIpv4();
        m_dataReceivedMap[peerAddress] += totalData;
        NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPRcv " << InetSocketAddress::ConvertFrom(peer).GetIpv4() << " " << InetSocketAddress::ConvertFrom(peer).GetPort() << " " << InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " " << InetSocketAddress::ConvertFrom(m_local).GetPort() << " " << totalData << " " << m_dataReceivedMap[peerAddress] << " " << m_commData);
        CheckEndComm();
    }
}

void DataParallel::CheckEndComm() {
    Ptr<MLPhaseComm> commPhase = DynamicCast<MLPhaseComm>(m_schedule->GetPhase(m_currentState));
    if (!commPhase) {
        NS_LOG_INFO("Warning: checking EndComm while not communicating!");
        return;
    }
    //NS_LOG_INFO(m_currentState << " " << commPhase);
    std::list<Ipv4Address> rcvList = commPhase->GetRcvList();
    std::list<Ipv4Address> sendList = commPhase->GetSendList();
    for (Ipv4Address addr : rcvList) {
        if (m_dataReceivedMap[addr] != m_commData) {
            NS_LOG_INFO("gorilla " << addr << " " << m_dataReceivedMap[addr] << " " << m_commData);
            return;
        }
    }
    for (Ipv4Address addr : sendList) {
        if (m_dataSentMap[addr] != 0) {
            NS_LOG_INFO("orangutan " << addr << " " << m_dataSentMap[addr]);
            return;
        }
    }
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DPCommEnd " << GetAddress());
    EndPhase();
    //m_currentState = (m_currentState + 1) % m_schedule->Size();
    //StartNextPhase();
}

void DataParallel::HandleAccept(Ptr<Socket> socket, const Address& addr) {
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " " << GetAddress() << " Connected to " << InetSocketAddress::ConvertFrom(addr).GetIpv4() << " " << InetSocketAddress::ConvertFrom(addr).GetPort());
    m_socketList.push_back(socket);
    socket->SetRecvCallback(MakeCallback(&DataParallel::ReadData, this));
}

void DataParallel::HandleSenderClose(Ptr<Socket> peer) {
    NS_LOG_WARN(Simulator::Now().GetSeconds() << " " <<  InetSocketAddress::ConvertFrom(m_local).GetIpv4() << " Node at socket " << peer << " has disconnected!");
    m_socketList.remove(peer);
}

void DataParallel::HandleSenderError(Ptr<Socket> peer) {
    NS_LOG_WARN(Simulator::Now().GetSeconds() << " Error: node at socket " << peer << " has crashed!");
    m_socketList.remove(peer);
}

void DataParallel::ConnectionSucceeded (Ptr<Socket> peer)
{
  NS_LOG_FUNCTION_NOARGS ();
  m_connected = true;
}

void DataParallel::ConnectionFailed (Ptr<Socket> peer) {
    NS_LOG_FUNCTION_NOARGS();
    NS_LOG_WARN(Simulator::Now().GetSeconds() << " DataParallel, Connection Failed");
}

//NS_LOG_COMPONENT_DEFINE("PacketIdTag");
NS_OBJECT_ENSURE_REGISTERED(PacketIdTag);

TypeId PacketIdTag::GetTypeId(void) { //TODO: consider removinv this, also make the flow id tag a configurable parameter so it can be turned omn/off
    static TypeId tid = TypeId("ns3::PacketIdTag")
        .SetParent<Tag>()
        .SetGroupName("Network")
        .AddConstructor<PacketIdTag>()
    ;
    return tid;
}

TypeId PacketIdTag::GetInstanceTypeId(void) const {
    return GetTypeId();
}

uint32_t PacketIdTag::GetSerializedSize(void) const {
    return 4;
}

void PacketIdTag::Serialize(TagBuffer buf) const {
    buf.WriteU32(m_packetId);
}

void PacketIdTag::Deserialize(TagBuffer buf) {
    m_packetId = buf.ReadU32();
}

void PacketIdTag::Print(std::ostream &os) const {
    os << "PacketId=" << m_packetId;
}

PacketIdTag::PacketIdTag() : Tag() {}

PacketIdTag::PacketIdTag(uint32_t packetId) : Tag(), m_packetId(packetId) {}

void PacketIdTag::SetPacketId(uint32_t packetId) {
    m_packetId = packetId;
}

uint32_t PacketIdTag::GetPacketId(void) const {
    return m_packetId;
}

uint32_t PacketIdTag::AllocatePacketId(void) {
    static uint32_t nextPacketId = 1;
    uint32_t packetId = nextPacketId;
    nextPacketId += 1;
    return packetId;
}

//NS_LOG_COMPONENT_DEFINE("SyncTag");
NS_OBJECT_ENSURE_REGISTERED(SyncTag);

TypeId SyncTag::GetTypeId(void) {
    static TypeId tid = TypeId("ns3::SyncTag")
        .SetParent<Tag>()
        .SetGroupName("Network")
        .AddConstructor<SyncTag>()
    ;
    return tid;
}

TypeId SyncTag::GetInstanceTypeId(void) const {
    return GetTypeId();
}

uint32_t SyncTag::GetSerializedSize(void) const {
    return 1;
}

void SyncTag::Serialize(TagBuffer buf) const {
    buf.WriteU8(m_vote);
}

void SyncTag::Deserialize(TagBuffer buf) {
    m_vote = buf.ReadU8();
}

void SyncTag::Print(std::ostream &os) const {
    os << "Vote=" << m_vote;
}

SyncTag::SyncTag() : Tag() {}

SyncTag::SyncTag(uint8_t vote) : Tag(), m_vote(vote) {}

void SyncTag::SetVote(uint8_t vote) {
    m_vote = vote;
}

uint8_t SyncTag::GetVote(void) const {
    return m_vote;
}

} // Namespace ns3
