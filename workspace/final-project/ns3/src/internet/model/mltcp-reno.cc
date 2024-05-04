/*
 * Copyright (c) 2019 NITK Surathkal
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Apoorva Bhargava <apoorvabhargava13@gmail.com>
 *         Mohit P. Tahiliani <tahiliani@nitk.edu.in>
 edited by Anton Zabreyko
 *
 */

#include "mltcp-reno.h"

#include "ns3/log.h"
#include "ns3/simulator.h"

namespace ns3
{

NS_LOG_COMPONENT_DEFINE("MLTcpReno");
NS_OBJECT_ENSURE_REGISTERED(MLTcpReno);

//const static uint32_t COMM_DATA = 250000000;
//const static Time COMP_TIME = Time("1s");
float lower = 1;
float upper = 4;

float lower_mul = 0.2;
float upper_mul = 0.8;

TypeId
MLTcpReno::GetTypeId()
{
    static TypeId tid = TypeId("ns3::MLTcpReno")
                            .SetParent<TcpCongestionOps>()
                            .SetGroupName("Internet")
                            .AddConstructor<MLTcpReno>();
    return tid;
}

MLTcpReno::MLTcpReno()
    : TcpCongestionOps()
{
    NS_LOG_FUNCTION(this);
    m_dataSent = 0;
    m_lastTimeSent = Time("0s");
    m_beta = 0.0;
    m_a = 4.0;
    m_b = 0.25;
}

MLTcpReno::MLTcpReno(const MLTcpReno& sock)
    : TcpCongestionOps(sock)
{
    NS_LOG_FUNCTION(this);
}

MLTcpReno::~MLTcpReno()
{
}

uint32_t
MLTcpReno::SlowStart(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    if (segmentsAcked >= 1)
    {
        uint32_t sndCwnd = tcb->m_cWnd;
        tcb->m_cWnd =
            std::min((sndCwnd + (segmentsAcked * tcb->m_segmentSize)), (uint32_t)tcb->m_ssThresh);
        NS_LOG_INFO("In SlowStart, updated to cwnd " << tcb->m_cWnd << " ssthresh "
                                                     << tcb->m_ssThresh);
        return segmentsAcked - ((tcb->m_cWnd - sndCwnd) / tcb->m_segmentSize);
    }

    return 0;
}

double MLTcpReno::f(double mu) {
    return m_a * mu + m_b;
}

void
MLTcpReno::CongestionAvoidance(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);

    uint32_t w = tcb->m_cWnd / tcb->m_segmentSize;
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " congestion " << m_dataSent);
    // Floor w to 1 if w == 0
    if (w == 0)
    {
        w = 1;
    }
    double alpha = f(m_beta);
    NS_LOG_DEBUG("w in segments " << w << " m_cWndCnt " << m_cWndCnt << " segments acked " << segmentsAcked);
    float change = ((float) segmentsAcked / w) * tcb->m_segmentSize * alpha;
    tcb->m_cWnd += change;
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " Alpha " << m_source << " " << m_dest << " " << alpha << " " << m_dataSent << " " << tcb->m_cWnd << " " << segmentsAcked << " " << w << " " << tcb->m_segmentSize << " " << change);
}


void
MLTcpReno::IncreaseWindow(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked)
{
    NS_LOG_FUNCTION(this << tcb << segmentsAcked);
    //m_dataSent += segmentsAcked * tcb->m_segmentSize; 
    NS_LOG_INFO(Simulator::Now().GetSeconds() << " DataSent " << m_source << " " << m_dest << " " << m_dataSent << " " << segmentsAcked << " " << tcb->m_segmentSize);
    //NS_LOG_INFO("doggo " << tcb->m_cWnd << " " << tcb->m_ssThresh);
    // Linux tcp_in_slow_start() condition
    if (tcb->m_cWnd < tcb->m_ssThresh)
    {
        NS_LOG_DEBUG("In slow start, m_cWnd " << tcb->m_cWnd << " m_ssThresh " << tcb->m_ssThresh);
        segmentsAcked = SlowStart(tcb, segmentsAcked);
    }
    else
    {
        NS_LOG_DEBUG("In cong. avoidance, m_cWnd " << tcb->m_cWnd << " m_ssThresh "
                                                   << tcb->m_ssThresh);
        CongestionAvoidance(tcb, segmentsAcked);
    }
}

void MLTcpReno::PktsAcked(Ptr<TcpSocketState> tcb, uint32_t segmentsAcked, const Time& rtt) {
    // loading configuration
    m_compTime = tcb->m_compTime;
    m_totalData = tcb->m_commSize;
    m_source = tcb->m_source;
    m_dest = tcb->m_dest;

 
    NS_LOG_INFO("mltcp params " << m_compTime << " " << m_totalData << " " << segmentsAcked << " " << rtt);

    m_dataSent += segmentsAcked * tcb->m_segmentSize;
    Time diffTime = Simulator::Now() - m_lastTimeSent;
    if (diffTime > 0.9 * m_compTime) {
        m_beta = 0;
        tcb->m_cWnd = 10 * tcb->m_segmentSize;
        //NS_LOG_INFO("timediff " << diffTime << " " << m_compTime << " " << 0.9 * m_compTime << " " << Simulator::Now() << " " << m_lastTimeSent);
        m_dataSent = 0;
    } else {
        m_beta = std::min(1.0, (double) m_dataSent/ (double) m_totalData);
    }
    m_lastTimeSent = Simulator::Now();
    NS_LOG_INFO("finishing pkts acked");
}

std::string
MLTcpReno::GetName() const
{
    return "MLTcpReno";
}

uint32_t
MLTcpReno::GetSsThresh(Ptr<const TcpSocketState> state, uint32_t bytesInFlight)
{
    NS_LOG_FUNCTION(this << state << bytesInFlight);

    // In Linux, it is written as:  return max(tp->snd_cwnd >> 1U, 2U);
    //float alpha = lower_mul + (float) m_dataSent/ (float) COMM_DATA * (upper_mul - lower_mul);
    //alpha = 0.5;
    //NS_LOG_INFO(Simulator::Now().GetSeconds() << " alpha: " << alpha);
    //NS_LOG_INFO(Simulator::Now().GetSeconds() << " TCPLoss " << this);
    //float newCwnd = state->m_cWnd * alpha;
    NS_LOG_INFO("getting sshthres");
    float newCwnd = state->m_cWnd * 0.5;
    return std::max<uint32_t>(2 * state->m_segmentSize, newCwnd);
}

Ptr<TcpCongestionOps>
MLTcpReno::Fork()
{
    return CopyObject<MLTcpReno>(this);
}

} // namespace ns3
