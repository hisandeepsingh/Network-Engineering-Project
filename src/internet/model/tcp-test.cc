#include "tcp-test.h"
#include "ns3/tcp-socket-base.h"
#include "ns3/log.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("TcpTest");
NS_OBJECT_ENSURE_REGISTERED (TcpTest);

TypeId
TcpTest::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::TcpTest")
    .SetParent<TcpNewReno> ()
    .AddConstructor<TcpTest> ()
    .SetGroupName ("Internet")
  ;
  return tid;
}

TcpTest::TcpTest (void)
  : TcpNewReno ()
{
  NS_LOG_FUNCTION (this);
  m_baseHistoLen = 10;
  m_noiseFilterLen = 8;
  InitCircbuf (m_baseHistory);
  InitCircbuf (m_noiseFilter);
  m_lastRollover = 0;
  m_maxObserved = 0;
}

void TcpTest::InitCircbuf (struct TestOwdCircBuf &buffer)
{
  NS_LOG_FUNCTION (this);
  buffer.buffer.clear ();
  buffer.min = 0;
}

TcpTest::TcpTest (const TcpTest& sock)
  : TcpNewReno (sock)
{
  NS_LOG_FUNCTION (this);
  m_baseHistoLen = sock.m_baseHistoLen;
  m_noiseFilterLen = sock.m_noiseFilterLen;
  m_baseHistory = sock.m_baseHistory;
  m_noiseFilter = sock.m_noiseFilter;
  m_lastRollover = sock.m_lastRollover;
}

TcpTest::~TcpTest (void)
{
  NS_LOG_FUNCTION (this);
}

Ptr<TcpCongestionOps>
TcpTest::Fork (void)
{
  return CopyObject<TcpTest> (this);
}

std::string
TcpTest::GetName () const
{
  return "TcpTest";
}

uint32_t TcpTest::MinCircBuff (struct TestOwdCircBuf &b)
{
  if (b.buffer.size () == 0)
    {
      return ~0;
    }
  else
    {
      return b.buffer[b.min];
    }
}

uint32_t TcpTest::MaxCircBuff (struct TestOwdCircBuf &b)
{
  if (b.buffer.size () == 0)
    {
      return 0;
    }
  else
    {
      return b.buffer[b.max];
    }
}

uint32_t TcpTest::CurrentDelay (FilterFunction filter)
{
  return filter (m_noiseFilter);
}

uint32_t TcpTest::BaseDelay ()
{
  return MinCircBuff (m_baseHistory);
}

uint32_t TcpTest::GetSsThresh (Ptr<const TcpSocketState> tcb,
                                 uint32_t bytesInFlight)
{
  NS_LOG_FUNCTION (this << tcb << bytesInFlight);
  if(!m_validFlag || m_maxObserved == 0)
  {
    return std::max (2 * tcb->m_segmentSize, bytesInFlight / 2);
  }
  else if ((CurrentDelay (&TcpTest::MaxCircBuff) - BaseDelay ())<=0)
  {
    return bytesInFlight + tcb->m_segmentSize;
  }
  else
  {
    return std::max (2 * tcb->m_segmentSize, std::min((bytesInFlight * m_maxObserved) / (2 * (CurrentDelay (&TcpTest::MaxCircBuff) - BaseDelay ())), bytesInFlight + tcb->m_segmentSize));
  }
}


void TcpTest::AddDelay (struct TestOwdCircBuf &cb, uint32_t owd, uint32_t maxlen)
{
  NS_LOG_FUNCTION (this << owd << maxlen << cb.buffer.size ());
  if (cb.buffer.size () == 0)
    {
      NS_LOG_LOGIC ("First Value for queue");
      cb.buffer.push_back (owd);
      cb.min = 0;
      cb.max = 0;
      return;
    }
  cb.buffer.push_back (owd);
  if (cb.buffer[cb.min] > owd)
    {
      cb.min = cb.buffer.size () - 1;
    }
  if (cb.buffer[cb.max] < owd)
    {
      cb.max = cb.buffer.size () - 1;
    }
  if (cb.buffer.size () >= maxlen)
    {
      NS_LOG_LOGIC("Queue full" << maxlen);
      cb.buffer.erase (cb.buffer.begin ());
      cb.min = 0;
      cb.max = 0;
      NS_LOG_LOGIC("Current min element" << cb.buffer[cb.min]);
      for (uint32_t i = 1; i < maxlen - 1; i++)
        {
          if (cb.buffer[i] < cb.buffer[cb.min])
            {
              cb.min = i;
            }
          if (cb.buffer[i] > cb.buffer[cb.max])
            {
              cb.max = i;
            }
        }
    }
}

void TcpTest::UpdateCurrentDelay (uint32_t owd)
{
  AddDelay (m_noiseFilter, owd, m_noiseFilterLen);
}

void TcpTest::UpdateBaseDelay (uint32_t owd)
{
  if (m_baseHistory.buffer.size () == 0)
    {
      AddDelay (m_baseHistory, owd, m_baseHistoLen);
      return;
    }
  uint64_t timestamp = (uint64_t) Simulator::Now ().GetSeconds ();

  if (timestamp - m_lastRollover > 60)
    {
      m_lastRollover = timestamp;
      AddDelay (m_baseHistory, owd, m_baseHistoLen);
    }
  else
    {
      uint32_t last = m_baseHistory.buffer.size () - 1;
      if (owd < m_baseHistory.buffer[last])
        {
          m_baseHistory.buffer[last] = owd;
          if (owd < m_baseHistory.buffer[m_baseHistory.min])
            {
              m_baseHistory.min = last;
            }
        }
    }
}

void TcpTest::PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                           const Time& rtt)
{
  NS_LOG_FUNCTION (this << tcb << segmentsAcked << rtt);
  if (tcb->m_rcvTimestampValue == 0 || tcb->m_rcvTimestampEchoReply == 0)
    {
     m_validFlag = false;
    }
  else
    {
      m_validFlag = true;
    }
  if (m_validFlag & rtt.IsPositive ())
    {
      UpdateCurrentDelay (tcb->m_rcvTimestampValue - tcb->m_rcvTimestampEchoReply);
      UpdateBaseDelay (tcb->m_rcvTimestampValue - tcb->m_rcvTimestampEchoReply);
      uint32_t queue_delay = CurrentDelay (&TcpTest::MinCircBuff) - BaseDelay ();
      if (m_maxObserved < queue_delay)
        {
          m_maxObserved = queue_delay;
        }
    } 
}
}
