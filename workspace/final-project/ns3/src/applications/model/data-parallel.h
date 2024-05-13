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
// Edited by Anton Zabreyko

// ns3 - On/Off Data Source Application class
// George F. Riley, Georgia Tech, Spring 2007
// Adapted from ApplicationOnOff in GTNetS.

#ifndef DATA_PARALLEL_H
#define DATA_PARALLEL_H

#include "ns3/address.h"
#include "ns3/application.h"
#include "ns3/event-id.h"
#include "ns3/ptr.h"
#include "ns3/data-rate.h"
#include "ns3/traced-callback.h"
#include "ns3/ml-schedule.h"
#include <map>

namespace ns3 {

class Address;
class RandomVariableStream;
class Socket;
class MLSchedule;

/**
 * \ingroup applications 
 * \defgroup onoff DataParallel
 *
 * This traffic generator follows an On/Off pattern: after 
 * Application::StartApplication
 * is called, "On" and "Off" states alternate. The duration of each of
 * these states is determined with the onTime and the offTime random
 * variables. During the "Off" state, no traffic is generated.
 * During the "On" state, cbr traffic is generated. This cbr traffic is
 * characterized by the specified "data rate" and "packet size".
 */
/**
* \ingroup onoff
*
* \brief Generate traffic to a single destination according to an
*        OnOff pattern.
*
* This traffic generator follows an On/Off pattern: after
* Application::StartApplication
* is called, "On" and "Off" states alternate. The duration of each of
* these states is determined with the onTime and the offTime random
* variables. During the "Off" state, no traffic is generated.
* During the "On" state, cbr traffic is generated. This cbr traffic is
* characterized by the specified "data rate" and "packet size".
*
* Note:  When an application is started, the first packet transmission
* occurs _after_ a delay equal to (packet size/bit rate).  Note also,
* when an application transitions into an off state in between packet
* transmissions, the remaining time until when the next transmission
* would have occurred is cached and is used when the application starts
* up again.  Example:  packet size = 1000 bits, bit rate = 500 bits/sec.
* If the application is started at time 3 seconds, the first packet
* transmission will be scheduled for time 5 seconds (3 + 1000/500)
* and subsequent transmissions at 2 second intervals.  If the above
* application were instead stopped at time 4 seconds, and restarted at
* time 5.5 seconds, then the first packet would be sent at time 6.5 seconds,
* because when it was stopped at 4 seconds, there was only 1 second remaining
* until the originally scheduled transmission, and this time remaining
* information is cached and used to schedule the next transmission
* upon restarting.
*
* If the underlying socket type supports broadcast, this application
* will automatically enable the SetAllowBroadcast(true) socket option.
*/
class DataParallel : public Application 
{
public:
  static TypeId GetTypeId (void);

  DataParallel ();

  virtual ~DataParallel();

  /**
   * \return pointer to associated socket
   */
  Ptr<Socket> GetSocket (void) const;

  void AddSchedule(Ptr<MLSchedule>);
  bool Busy();
  Ipv4Address GetAddress();
  void SetNotifyCallback(Callback<void, Ptr<DataParallel>>);

  void StartSyncFuncPhase();
  void EndSyncFuncPhase(); // temporary, should delete later

protected:
  virtual void DoDispose (void);
private:
  // inherited from Application base class.
  virtual void StartApplication (void);    // Called at time specified by Start
  virtual void StopApplication (void);     // Called at time specified by Stop

  //helpers
  void CancelEvents ();

  /*
  void Construct (Ptr<Node> n,
                  const Address &remote,
                  std::string tid,
                  const RandomVariable& ontime,
                  const RandomVariable& offtime,
                  uint32_t size);
 */

  Ptr<MLSchedule> m_schedule;
  std::map<Ipv4Address, Ptr<Socket>> m_socketMap;
  uint32_t        m_currentState;
  std::map<Ipv4Address, uint32_t> m_dataLoadedMap;
  std::map<Ipv4Address, uint32_t> m_dataSentMap;
  std::map<Ipv4Address, uint32_t> m_dataReceivedMap;
  std::list<Ptr<Socket>> m_socketList; // List of sockets
  Ptr<Socket>     m_listenerSocket; // Listener socket
  Address         m_local;
  Callback<void, Ptr<DataParallel>>  m_notifyCompletion;
  bool            m_connected;    // True if connected
  Time            m_compTime;     // Time spent on computation phase
  uint32_t        m_commData;   // Data to send per communication phase
  DataRate        m_cbrRate;      // Rate that data is generated
  uint32_t        m_packetsSent;   // Counter for number of packets to be sent in communication phase
  uint32_t        m_dataSent;
  //uint32_t        m_dataReceived; 
  uint32_t        m_pktSize;      // Size of packets
  uint32_t        m_residualBits; // Number of generated, but not sent, bits
  Time            m_lastStartTime; // Time last packet sent
  uint32_t        m_totBytes;     // Total bytes sent so far
  EventId         m_startStopEvent;     // Event id for next start or stop event
  EventId         m_sendEvent;    // Eventid of pending "send packet" event
  bool            m_sending;      // True if currently in sending state
  TypeId          m_tid;
  uint32_t        m_numPackets;
  uint32_t        m_remainder;
  uint32_t        m_flowIdTag;
  uint32_t        m_currIter;
  uint32_t        m_currVotes;

private:
  void StartJob();
  void StartNextPhase();
  void InitPhase(uint32_t);
  void RunNextPhase();
  void StartCompPhase();
  void EndCompPhase();
  void InitCommPhase(uint32_t);
  void StartCommPhase();
  void StartSyncPhase();
  void CheckSyncPhase();
  void EndSyncPhase();
  void CheckEndComm();
  void EndPhase(bool init = true);
  void CloseSockets();
  void ConnectionSucceeded (Ptr<Socket>);
  void ConnectionFailed (Ptr<Socket>);
  void Ignore (Ptr<Socket>);
  void SendData(Ptr<Socket>, uint32_t);
  void CountSend(Ptr<Socket>, uint32_t);
  void ReadData(Ptr<Socket>);
  void HandleAccept(Ptr<Socket>, const Address &);
  void HandleSenderClose(Ptr<Socket>);
  void HandleSenderError(Ptr<Socket>);
  void TraceCwnd(uint32_t, uint32_t, uint32_t, uint16_t, uint32_t, uint16_t);
  void TraceRTT(Time, Time, uint32_t, uint16_t, uint32_t, uint16_t);
  void DefaultNotify(Ptr<DataParallel>);
};

class PacketIdTag : public Tag {

public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(TagBuffer buf) const;
    virtual void Deserialize(TagBuffer buf);
    virtual void Print(std::ostream &os) const;
    PacketIdTag();
    
    PacketIdTag(uint32_t packetId);
    void SetPacketId(uint32_t packetId);
    uint32_t GetPacketId(void) const;
    static uint32_t AllocatePacketId(void);

private:
    uint32_t m_packetId;

};

class SyncTag : public Tag {

public:
    static TypeId GetTypeId(void);
    virtual TypeId GetInstanceTypeId(void) const;
    virtual uint32_t GetSerializedSize(void) const;
    virtual void Serialize(TagBuffer buf) const;
    virtual void Deserialize(TagBuffer buf);
    virtual void Print(std::ostream &os) const;
    SyncTag();
    
    SyncTag(uint8_t vote);
    void SetVote(uint8_t vote);
    uint8_t GetVote() const;

private:
    uint8_t m_vote;

};

} // namespace ns3

#endif /* DATA_PARALLEL_H */
