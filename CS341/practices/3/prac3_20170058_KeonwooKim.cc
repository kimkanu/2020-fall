/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
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
 */

#include <fstream>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/applications-module.h"
#include "ns3/tcp-socket-state.h"
#include "ns3/stats-module.h"

using namespace ns3;
using namespace std;

NS_LOG_COMPONENT_DEFINE("Prac3");

class MyApp : public Application
{
public:
  MyApp();
  virtual ~MyApp();

  /**
   * Register this type.
   * \return The TypeId.
   */
  static TypeId GetTypeId(void);
  void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);

private:
  virtual void StartApplication(void);
  virtual void StopApplication(void);

  void ScheduleTx(void);
  void SendPacket(void);

  Ptr<Socket> m_socket;
  Address m_peer;
  uint32_t m_packetSize;
  uint32_t m_nPackets;
  DataRate m_dataRate;
  EventId m_sendEvent;
  bool m_running;
  uint32_t m_packetsSent;
};

MyApp::MyApp()
    : m_socket(0),
      m_peer(),
      m_packetSize(0),
      m_nPackets(0),
      m_dataRate(0),
      m_sendEvent(),
      m_running(false),
      m_packetsSent(0)
{
}

MyApp::~MyApp()
{
  m_socket = 0;
}

/* static */
TypeId MyApp::GetTypeId(void)
{
  static TypeId tid = TypeId("MyApp")
                          .SetParent<Application>()
                          .SetGroupName("Tutorial")
                          .AddConstructor<MyApp>();
  return tid;
}

void MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
  m_socket = socket;
  m_peer = address;
  m_packetSize = packetSize;
  m_nPackets = nPackets;
  m_dataRate = dataRate;
}

void MyApp::StartApplication(void)
{
  m_running = true;
  m_packetsSent = 0;
  m_socket->Bind();
  m_socket->Connect(m_peer);
  SendPacket();
}

void MyApp::StopApplication(void)
{
  m_running = false;

  if (m_sendEvent.IsRunning())
  {
    Simulator::Cancel(m_sendEvent);
  }

  if (m_socket)
  {
    m_socket->Close();
  }
}

void MyApp::SendPacket(void)
{
  Ptr<Packet> packet = Create<Packet>(m_packetSize);
  m_socket->Send(packet);

  if (++m_packetsSent < m_nPackets)
  {
    ScheduleTx();
  }
}

void MyApp::ScheduleTx(void)
{
  if (m_running)
  {
    Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
    m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
  }
}

static void
NotifyChange(Ptr<OutputStreamWrapper> stream, uint32_t oldval, uint32_t newval)
{
  *stream->GetStream() << Simulator::Now().GetSeconds() << "\t" << oldval << "\t" << newval << std::endl;
}

Ptr<PacketSink> sink1, sink2, sink3;                        /* Pointer to the packet sink application */
uint64_t lastTotalRx1(0), lastTotalRx2(0), lastTotalRx3(0); /* The value of the last total received bytes */

void ReceivedBytes1(Ptr<OutputStreamWrapper> stream)
{
  Time now = Simulator::Now(); /* Return the simulator's virtual time. */
  *stream->GetStream() << now.GetSeconds() << "\t"
                       << (sink1->GetTotalRx() - lastTotalRx1) * 8 / 0.1 * 1e-6 << "\t" // throughput in Mbps
                       << sink1->GetTotalRx() << "\t" << std::endl;                     // received bytes in sink
  lastTotalRx1 = sink1->GetTotalRx();
  Simulator::Schedule(MilliSeconds(100), &ReceivedBytes1, stream);
}

void ReceivedBytes2(Ptr<OutputStreamWrapper> stream)
{
  Time now = Simulator::Now(); /* Return the simulator's virtual time. */
  *stream->GetStream() << now.GetSeconds() << "\t"
                       << (sink2->GetTotalRx() - lastTotalRx2) * 8 / 0.1 * 1e-6 << "\t" // throughput in Mbps
                       << sink2->GetTotalRx() << "\t" << std::endl;                     // received bytes in sink
  lastTotalRx2 = sink2->GetTotalRx();
  Simulator::Schedule(MilliSeconds(100), &ReceivedBytes2, stream);
}

void ReceivedBytes3(Ptr<OutputStreamWrapper> stream)
{
  Time now = Simulator::Now(); /* Return the simulator's virtual time. */
  *stream->GetStream() << now.GetSeconds() << "\t"
                       << (sink3->GetTotalRx() - lastTotalRx3) * 8 / 0.1 * 1e-6 << "\t" // throughput in Mbps
                       << sink3->GetTotalRx() << "\t" << std::endl;                     // received bytes in sink
  lastTotalRx3 = sink3->GetTotalRx();
  Simulator::Schedule(MilliSeconds(100), &ReceivedBytes3, stream);
}

int main(int argc, char *argv[])
{
  CommandLine cmd(__FILE__);
  cmd.Parse(argc, argv);

  NS_LOG_INFO("Create nodes.");
  NodeContainer c;
  c.Create(5);
  NodeContainer n1n0 = NodeContainer(c.Get(1), c.Get(0));
  NodeContainer n2n1 = NodeContainer(c.Get(2), c.Get(1));
  NodeContainer n3n1 = NodeContainer(c.Get(3), c.Get(1));
  NodeContainer n4n1 = NodeContainer(c.Get(4), c.Get(1));

  InternetStackHelper internet;
  internet.Install(c);

  // We create the channels first without any IP addressing information
  NS_LOG_INFO("Create channels.");
  PointToPointHelper p2p;
  p2p.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer d1d0 = p2p.Install(n1n0);
  Ptr<RateErrorModel> em = CreateObject<RateErrorModel>();
  em->SetAttribute("ErrorRate", DoubleValue(0.000001));
  d1d0.Get(1)->SetAttribute("ReceiveErrorModel", PointerValue(em));

  p2p.SetDeviceAttribute("DataRate", StringValue("2Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer d2d1 = p2p.Install(n2n1);

  p2p.SetDeviceAttribute("DataRate", StringValue("5Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer d3d1 = p2p.Install(n3n1);

  p2p.SetDeviceAttribute("DataRate", StringValue("1Mbps"));
  p2p.SetChannelAttribute("Delay", StringValue("10ms"));
  NetDeviceContainer d4d1 = p2p.Install(n4n1);

  // Later, we add IP addresses.
  NS_LOG_INFO("Assign IP Addresses.");
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.1.1.0", "255.255.255.0");
  Ipv4InterfaceContainer i1i0 = ipv4.Assign(d1d0);

  ipv4.SetBase("10.1.2.0", "255.255.255.0");
  Ipv4InterfaceContainer i2i1 = ipv4.Assign(d2d1);

  ipv4.SetBase("10.1.3.0", "255.255.255.0");
  Ipv4InterfaceContainer i3i1 = ipv4.Assign(d3d1);

  ipv4.SetBase("10.1.4.0", "255.255.255.0");
  Ipv4InterfaceContainer i4i1 = ipv4.Assign(d4d1);

  Ipv4GlobalRoutingHelper::PopulateRoutingTables();

  NS_LOG_INFO("Create Applications.");

  AsciiTraceHelper asciiTraceHelper;

  /**
   * TASK 1 - APP (a)
   */

  // sink app 1
  uint16_t sinkPort1 = 8080;
  Address sinkAddress1(InetSocketAddress(i1i0.GetAddress(1), sinkPort1));
  PacketSinkHelper packetSinkHelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort1));
  ApplicationContainer sinkApp1 = packetSinkHelper1.Install(c.Get(0));
  sink1 = StaticCast<PacketSink>(sinkApp1.Get(0));
  sinkApp1.Start(Seconds(0.)); //sink apps run from t=0s
  sinkApp1.Stop(Seconds(120.));

  // app 1
  Ptr<Socket> ns3TcpSocket1 = Socket::CreateSocket(c.Get(2), TcpSocketFactory::GetTypeId());
  Ptr<MyApp> app1 = CreateObject<MyApp>();
  app1->Setup(ns3TcpSocket1, sinkAddress1, 1000, 25000, DataRate("2Mbps"));
  c.Get(2)->AddApplication(app1);
  app1->SetStartTime(Seconds(1.)); //app starts at t=1s
  app1->SetStopTime(Seconds(120.));

  Simulator::Schedule(Seconds(1.1),
                      &ReceivedBytes1,
                      asciiTraceHelper.CreateFileStream("prac3_app1.rx"));
  ns3TcpSocket1->TraceConnectWithoutContext(
      "CongestionWindow",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app1.cwnd")));
  ns3TcpSocket1->TraceConnectWithoutContext(
      "SlowStartThreshold",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app1.ssthresh")));

  /**
   * TASK 1 - APP (b)
   */

  // sink app 2
  uint16_t sinkPort2 = 8081;
  Address sinkAddress2(InetSocketAddress(i1i0.GetAddress(1), sinkPort2));
  PacketSinkHelper packetSinkHelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort2));
  ApplicationContainer sinkApp2 = packetSinkHelper2.Install(c.Get(0));
  sink2 = StaticCast<PacketSink>(sinkApp2.Get(0));
  sinkApp2.Start(Seconds(0.)); //sink apps run from t=0s
  sinkApp2.Stop(Seconds(120.));

  // app 2
  Ptr<Socket> ns3TcpSocket2 = Socket::CreateSocket(c.Get(3), TcpSocketFactory::GetTypeId());
  Ptr<MyApp> app2 = CreateObject<MyApp>();
  app2->Setup(ns3TcpSocket2, sinkAddress2, 1000, 25000, DataRate("5Mbps"));
  c.Get(3)->AddApplication(app2);
  app2->SetStartTime(Seconds(15.)); //app starts at t=15s
  app2->SetStopTime(Seconds(120.));

  Simulator::Schedule(Seconds(15.1),
                      &ReceivedBytes2,
                      asciiTraceHelper.CreateFileStream("prac3_app2.rx"));
  ns3TcpSocket2->TraceConnectWithoutContext(
      "CongestionWindow",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app2.cwnd")));
  ns3TcpSocket2->TraceConnectWithoutContext(
      "SlowStartThreshold",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app2.ssthresh")));

  /**
   * TASK 1 - APP (c)
   */

  // sink app 3
  uint16_t sinkPort3 = 8082;
  Address sinkAddress3(InetSocketAddress(i1i0.GetAddress(1), sinkPort3));
  PacketSinkHelper packetSinkHelper3("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), sinkPort3));
  ApplicationContainer sinkApp3 = packetSinkHelper3.Install(c.Get(0));
  sink3 = StaticCast<PacketSink>(sinkApp3.Get(0));
  sinkApp3.Start(Seconds(0.)); //sink apps run from t=0s
  sinkApp3.Stop(Seconds(120.));

  // app 3
  Ptr<Socket> ns3TcpSocket3 = Socket::CreateSocket(c.Get(4), TcpSocketFactory::GetTypeId());
  Ptr<MyApp> app3 = CreateObject<MyApp>();
  app3->Setup(ns3TcpSocket3, sinkAddress3, 1000, 5000, DataRate("2Mbps"));
  c.Get(4)->AddApplication(app3);
  app3->SetStartTime(Seconds(30.)); //app starts at t=30s
  app3->SetStopTime(Seconds(120.));

  Simulator::Schedule(Seconds(30.1),
                      &ReceivedBytes3,
                      asciiTraceHelper.CreateFileStream("prac3_app3.rx"));
  ns3TcpSocket3->TraceConnectWithoutContext(
      "CongestionWindow",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app3.cwnd")));
  ns3TcpSocket3->TraceConnectWithoutContext(
      "SlowStartThreshold",
      MakeBoundCallback(
          &NotifyChange,
          asciiTraceHelper.CreateFileStream("prac3_app3.ssthresh")));

  Simulator::Stop(Seconds(120.));
  Simulator::Run();
  Simulator::Destroy();

  /////////
  /////////
  /////////
  /////////
  /////////

  // 1.1.1.
  Gnuplot prac3CwndPlot("prac3.cwnd.png");
  prac3CwndPlot.SetTitle("Congestion Window Calculation");
  prac3CwndPlot.SetTerminal("png");
  prac3CwndPlot.SetLegend("Time (in Seconds)", "Congestion Window (cwnd)");
  prac3CwndPlot.AppendExtra("set xrange [0:120]");
  double x, y, y0;

  Gnuplot2dDataset prac3App1CwndDataset;
  prac3App1CwndDataset.SetTitle("Congestion Window 1");
  prac3App1CwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App1CwndDataFile("prac3_app1.cwnd");
  while (prac3App1CwndDataFile >> x >> y >> y0)
    prac3App1CwndDataset.Add(x, y);
  prac3CwndPlot.AddDataset(prac3App1CwndDataset);

  Gnuplot2dDataset prac3App2CwndDataset;
  prac3App2CwndDataset.SetTitle("Congestion Window 2");
  prac3App2CwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App2CwndDataFile("prac3_app2.cwnd");
  while (prac3App2CwndDataFile >> x >> y >> y0)
    prac3App2CwndDataset.Add(x, y);
  prac3CwndPlot.AddDataset(prac3App2CwndDataset);

  Gnuplot2dDataset prac3App3CwndDataset;
  prac3App3CwndDataset.SetTitle("Congestion Window 3");
  prac3App3CwndDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App3CwndDataFile("prac3_app3.cwnd");
  while (prac3App3CwndDataFile >> x >> y >> y0)
    prac3App3CwndDataset.Add(x, y);
  prac3CwndPlot.AddDataset(prac3App3CwndDataset);

  string prac3CwndPlotFileName = "prac3.cwnd.plt";
  ofstream prac3CwndPlotFile(prac3CwndPlotFileName.c_str());
  prac3CwndPlot.GenerateOutput(prac3CwndPlotFile);
  prac3CwndPlotFile.close();

  // 1.1.2.
  Gnuplot prac3RxPlot("prac3.rx.png");
  prac3RxPlot.SetTitle("TCP Throughput (Mbps)");
  prac3RxPlot.SetTerminal("png");
  prac3RxPlot.SetLegend("Time (in Seconds)", "TCP Throughput (Mbps)");
  prac3RxPlot.AppendExtra("set xrange [0:120]");

  Gnuplot2dDataset prac3App1RxDataset;
  prac3App1RxDataset.SetTitle("TCP Throughput 1");
  prac3App1RxDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App1RxDataFile("prac3_app1.rx");
  while (prac3App1RxDataFile >> x >> y >> y0)
    prac3App1RxDataset.Add(x, y);
  prac3RxPlot.AddDataset(prac3App1RxDataset);

  Gnuplot2dDataset prac3App2RxDataset;
  prac3App2RxDataset.SetTitle("TCP Throughput 2");
  prac3App2RxDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App2RxDataFile("prac3_app2.rx");
  while (prac3App2RxDataFile >> x >> y >> y0)
    prac3App2RxDataset.Add(x, y);
  prac3RxPlot.AddDataset(prac3App2RxDataset);

  Gnuplot2dDataset prac3App3RxDataset;
  prac3App3RxDataset.SetTitle("TCP Throughput 3");
  prac3App3RxDataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);
  std::ifstream prac3App3RxDataFile("prac3_app3.rx");
  while (prac3App3RxDataFile >> x >> y >> y0)
    prac3App3RxDataset.Add(x, y);
  prac3RxPlot.AddDataset(prac3App3RxDataset);

  string prac3RxPlotFileName = "prac3.rx.plt";
  ofstream prac3RxPlotFile(prac3RxPlotFileName.c_str());
  prac3RxPlot.GenerateOutput(prac3RxPlotFile);
  prac3RxPlotFile.close();

  /////////
  /////////
  /////////
  /////////
  /////////

  return 0;
}
