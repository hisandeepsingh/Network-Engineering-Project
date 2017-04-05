#ifndef TCP_TEST_H
#define TCP_TEST_H

#include <vector>
#include "ns3/tcp-congestion-ops.h"
#include "ns3/event-id.h"

namespace ns3 {

/**
 *\brief Buffer structure to store delays
 */
struct TestOwdCircBuf
{
  std::vector<uint32_t> buffer; //!< Vector to store the delay
  uint32_t min;  //!< The index of minimum value
  uint32_t max;
};

/**
 * \ingroup congestionOps
 *
 * \brief An implementation of LEDBAT
 */

class TcpTest : public TcpNewReno
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);

  /**
   * Create an unbound tcp socket.
   */
  TcpTest (void);

  /**
   * \brief Copy constructor
   * \param sock the object to copy
   */
  TcpTest (const TcpTest& sock);

  /**
   * \brief Destructor
   */
  virtual ~TcpTest (void);

  /**
   * \brief Get the name of the TCP flavour
   *
   * \return The name of the TCP
   */
  virtual std::string GetName () const;

  /**
   * \brief Get information from the acked packet
   *
   * \param tcb internal congestion state
   * \param segmentsAcked count of segments ACKed
   * \param rtt The estimated rtt
   */
  virtual void PktsAcked (Ptr<TcpSocketState> tcb, uint32_t segmentsAcked,
                          const Time& rtt);

  /**
   * \brief Get the slow start threshold
   *
   * \param tcb internal congestion state
   * \param bytesInFlight bytes in flight
   *
   * \return The slow start threshold
   */
  virtual uint32_t GetSsThresh (Ptr<const TcpSocketState> tcb,
                                uint32_t bytesInFlight);
  virtual Ptr<TcpCongestionOps> Fork ();

private:
  /**
   * \brief Initialise a new buffer
   *
   * \param buffer The buffer to be initialised
   */
  void InitCircbuf (struct TestOwdCircBuf &buffer);

  typedef uint32_t (*FilterFunction)(struct TestOwdCircBuf &);

  /**
   * \brief Return the minimum delay of the buffer
   *
   * \param b The buffer
   * \return The minimum delay
   */
  static uint32_t MinCircBuff (struct TestOwdCircBuf &b);

  static uint32_t MaxCircBuff (struct TestOwdCircBuf &b);

  /**
   * \brief Return the value of current delay
   *
   * \param filter The filter function
   * \return The current delay
   */
  uint32_t CurrentDelay (FilterFunction filter);

  /**
   * \brief Return the value of base delay
   *
   * \return The base delay
   */
  uint32_t BaseDelay ();

  /**
   * \brief Add new delay to the buffers
   *
   * \param cb The buffer
   * \param owd The new delay
   * \param maxlen The maximum permitted length
   */
  void AddDelay (struct TestOwdCircBuf &cb, uint32_t owd, uint32_t maxlen);

  /**
   * \brief Update the current delay buffer
   *
   * \param owd The delay
   */
  void UpdateCurrentDelay (uint32_t owd);

  /**
   * \brief Update the base delay buffer
   *
   * \param owd The delay
   */
  void UpdateBaseDelay (uint32_t owd);

  uint32_t m_baseHistoLen;           //!< Length of base delay history buffer
  uint32_t m_noiseFilterLen;         //!< Length of current delay buffer
  uint64_t m_lastRollover;           //!< Timestamp of last added delay
  struct TestOwdCircBuf m_baseHistory;   //!< Buffer to store the base delay
  struct TestOwdCircBuf m_noiseFilter;   //!< Buffer to store the current delay
  bool m_validFlag;                  //!< LEDBAT Flag
  uint32_t m_maxObserved;
};

} // namespace ns3

#endif /* TCP_TEST_H */
