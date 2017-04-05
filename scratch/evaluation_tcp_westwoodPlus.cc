#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/wifi-module.h"
#include "ns3/applications-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/config-store-module.h"
#include "ns3/wireless-point-to-point-dumbbell.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE ("Exhaustive Evaluation");

uint32_t nleft=3;
uint32_t nright=3;
uint32_t maxBytes = 102400000;
uint64_t lastTotalRx = 0;
double tmp=(double) 8 /1e6;
int countDrop=0;

Ptr<PacketSink> sink0;
Ptr<PacketSink> sink1;
Ptr<PacketSink> sink2;


static void
RxDrop (Ptr<const Packet> p)
{  countDrop++;
  NS_LOG_UNCOND ("RxDrop at " << Simulator::Now ().GetSeconds ());

}

void
CalculateThroughput ()
{
  Time now = Simulator::Now ();
  double cur = (sink0->GetTotalRx () + sink1->GetTotalRx () + sink2->GetTotalRx () - lastTotalRx) * tmp;     
  std::cout << now.GetSeconds () << "s: \t" << cur << " Mbit/s" <<std::endl;
  lastTotalRx = sink0->GetTotalRx () + sink1->GetTotalRx () + sink2->GetTotalRx ();
  Simulator::Schedule (MilliSeconds (1000), &CalculateThroughput);
}

int   
main (int argc, char *argv[])
{ 
  double simulationTime = 20;
  CommandLine cmd;
  cmd.AddValue ("simulationTime", "Simulation time in seconds", simulationTime);
  cmd.Parse (argc, argv);

  PointToPointHelper right,central;
  
  central.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  central.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
  right.SetDeviceAttribute ("DataRate", StringValue ("10Mbps"));
  right.SetChannelAttribute ("Delay", StringValue ("2ms"));
  
  WirelessPointToPointDumbbellHelper wp2p(nleft,nright,right,central,"ns3::RandomDirection2dMobilityModel");

  /** TCP Deoghar Protocol  **/
  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpTest::GetTypeId ()));

 /** TCP WestwoodPlus Protocol **/

  //Config::SetDefault ("ns3::TcpL4Protocol::SocketType", TypeIdValue (TcpWestwood::GetTypeId ()));
  Config::SetDefault ("ns3::TcpWestwood::ProtocolType", EnumValue (TcpWestwood::WESTWOODPLUS));

  InternetStackHelper stack;
  //stack.SetTcp("ns3::TcpLedbat");
  wp2p.InstallStack(stack);

  Ptr<RateErrorModel> em = CreateObject<RateErrorModel> ();
  em->SetAttribute ("ErrorRate", DoubleValue(0.001));
  wp2p.GetLeft ()->GetDevice (0) -> SetAttribute ("ReceiveErrorModel", PointerValue (em));

  Ipv4AddressHelper routerIp;
  routerIp.SetBase ("10.1.1.0", "255.255.255.0");
  
  Ipv4AddressHelper rightIp;
  rightIp.SetBase("10.1.3.0","255.255.255.0");
  
  Ipv4AddressHelper leftIp;
  leftIp.SetBase("10.1.2.0","255.255.255.0");
  wp2p.AssignIpv4Addresses(leftIp,rightIp,routerIp);
  
  //central.EnablePcapAll("TcpDeoghar",false);
  central.EnablePcapAll("TcpWestwoodPlus",false);
  Ipv4GlobalRoutingHelper::PopulateRoutingTables ();

  BulkSendHelper source ("ns3::TcpSocketFactory", Address ());
  source.SetAttribute ("MaxBytes", UintegerValue (maxBytes));

  PacketSinkHelper sinkhelper ("ns3::TcpSocketFactory", InetSocketAddress (Ipv4Address::GetAny (), 8080));
  ApplicationContainer sinkApps;
  
  
  for (uint32_t i = 0; i < nright; ++i)
    {
      sinkApps.Add (sinkhelper.Install (wp2p.GetRight(i)));
    }
  sink0 = StaticCast<PacketSink> (sinkApps.Get (0));
  sink1 = StaticCast<PacketSink> (sinkApps.Get (1));
  sink2 = StaticCast<PacketSink> (sinkApps.Get (2));
  sinkApps.Start (Seconds (0.0));
  sinkApps.Stop (Seconds (20.0));

  ApplicationContainer sourceApps;

  for (uint32_t i = 0; i < nleft; ++i)
    {
      AddressValue sourceaddress (InetSocketAddress (wp2p.GetRightIpv4Address(i), 8080));
      source.SetAttribute ("Remote", sourceaddress);
      sourceApps.Add (source.Install (wp2p.GetLeft(i)));
    }
  sourceApps.Start (Seconds (0.5));
  sourceApps.Stop (Seconds (20.0));

 wp2p.GetLeft ()->GetDevice (0)->TraceConnectWithoutContext ("PhyRxDrop", MakeCallback (&RxDrop));
  
  Simulator::Schedule (Seconds (1), &CalculateThroughput);
  NS_LOG_INFO ("Run Simulation.");
  Simulator::Stop(Seconds(simulationTime));
  Simulator::Run ();
  
  double cur = (sink0->GetTotalRx () + sink1->GetTotalRx () + sink2->GetTotalRx () - lastTotalRx) * tmp;
  NS_LOG_UNCOND ("------------------------------\n" << "\nGoodput Mbits/sec:\t"<< cur/Simulator::Now ().GetSeconds ());
  NS_LOG_UNCOND ("----------------------------");
 std::cout <<  "Totaldrop: \t" <<countDrop<<std::endl;
  Simulator::Destroy ();

  return 0;
}
