#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/mobility-module.h"
#include "ns3/wifi-module.h"
#include "ns3/internet-module.h"
#include "ns3/olsr-helper.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/wifi-mac-helper.h"
#include "ns3/applications-module.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <string>

using namespace ns3;

// generate OnOffHelper and install it
void setOnOffHelper(ApplicationContainer *cbrApps, NodeContainer *nodes, int flowId, int srcNode, int destNode, double startingTime, uint32_t packetSize)
{
  uint16_t cbrPort = 12345;

  // set the ipv4 addr of dest node
  char destIpv4Address[10];
  snprintf (destIpv4Address, 10, "10.0.0.%d", destNode + 1);

  // set DataRate regarding \bugid{388} and \bugid{912}
  char dataRate[11];
  snprintf (dataRate, 11, "300%d00bps", (flowId - 1) * 11);

  OnOffHelper onOffHelper("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address(destIpv4Address), cbrPort));
  onOffHelper.SetAttribute("PacketSize", UintegerValue(packetSize));
  onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
  onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
  onOffHelper.SetAttribute("DataRate", StringValue(dataRate));
  onOffHelper.SetAttribute("StartTime", TimeValue(Seconds(startingTime)));
  cbrApps->Add(onOffHelper.Install(nodes->Get(srcNode)));
}

int main(int argc, char *argv[])
{
  // Network Configurations
  std::string phyMode("DsssRate1Mbps");
  double distance = 100;      // m
  uint32_t numNodes = 64;     // 8x8
  double interval = 0.1;      // seconds
  uint32_t packetSize = 1500; // bytes

  // Convert to time object
  Time interPacketInterval = Seconds(interval);

  // Trigger RTS/CTS here
  bool enableCtsRts = true;
  UintegerValue ctsThr = (enableCtsRts ? UintegerValue(100) : UintegerValue(2200));
  Config::SetDefault("ns3::WifiRemoteStationManager::RtsCtsThreshold", ctsThr);

  // Configure network - you can use examples/wireless/wifi-simple-adhoc-grid.cc as a guideline
  // Fix non-unicast data rate to be the same as that of unicast
  Config::SetDefault("ns3::WifiRemoteStationManager::NonUnicastMode",
                     StringValue(phyMode));

  NodeContainer nodes;
  nodes.Create(numNodes);

  // The below set of helpers will help us to put together the wifi NICs we want
  WifiHelper wifi;

  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default();

  YansWifiChannelHelper wifiChannel;
  wifiChannel.SetPropagationDelay("ns3::ConstantSpeedPropagationDelayModel");
  wifiChannel.AddPropagationLoss("ns3::FriisPropagationLossModel");
  wifiPhy.SetChannel(wifiChannel.Create());

  // Add an upper mac and disable rate control
  WifiMacHelper wifiMac;
  wifi.SetStandard(WIFI_PHY_STANDARD_80211b);
  wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                               "DataMode", StringValue(phyMode),
                               "ControlMode", StringValue(phyMode));
  // Set it to adhoc mode
  wifiMac.SetType("ns3::AdhocWifiMac");
  NetDeviceContainer devices = wifi.Install(wifiPhy, wifiMac, nodes);

  MobilityHelper mobility;
  mobility.SetPositionAllocator("ns3::GridPositionAllocator",
                                "MinX", DoubleValue(0.0),
                                "MinY", DoubleValue(0.0),
                                "DeltaX", DoubleValue(distance),
                                "DeltaY", DoubleValue(distance),
                                "GridWidth", UintegerValue(8),
                                "LayoutType", StringValue("RowFirst"));
  mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
  mobility.Install(nodes);

  // Enable OLSR
  OlsrHelper olsr;
  Ipv4StaticRoutingHelper staticRouting;

  Ipv4ListRoutingHelper list;
  list.Add(staticRouting, 0);
  list.Add(olsr, 10);

  InternetStackHelper internet;
  internet.SetRoutingHelper(list); // has effect on the next Install ()
  internet.Install(nodes);
  Ipv4AddressHelper ipv4;
  ipv4.SetBase("10.0.0.0", "255.0.0.0");
  ipv4.Assign(devices);

  // 7. Install applications: two CBR streams each saturating the channel
  ApplicationContainer cbrApps;

  // set flows
  setOnOffHelper(&cbrApps, &nodes, 1, 0, 63, 10.0, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 2, 56, 7, 10.5, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 3, 16, 23, 11.0, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 4, 40, 47, 11.5, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 5, 58, 2, 12.0, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 6, 5, 61, 12.5, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 7, 26, 53, 13.0, packetSize);
  setOnOffHelper(&cbrApps, &nodes, 8, 13, 51, 13.5, packetSize);

  // Install FlowMonitor on all nodes
  FlowMonitorHelper flowmon;
  Ptr<FlowMonitor> monitor = flowmon.Install(nodes);

  Simulator::Stop(Seconds(90.0));
  Simulator::Run();

  // Print per flow statistics
  monitor->CheckForLostPackets();
  Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
  std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();

  for (std::map<FlowId, FlowMonitor::FlowStats>::const_iterator iter = stats.begin(); iter != stats.end(); ++iter)
  {
    Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow(iter->first);

    std::cout << "Flow ID: " << iter->first << " Src Addr " << t.sourceAddress << " Dst Addr " << t.destinationAddress << "\n";
    std::cout << "Tx Packets = " << iter->second.txPackets << "\n";
    std::cout << "Rx Packets = " << iter->second.rxPackets << "\n";
    std::cout << "Throughput: " << iter->second.rxBytes * 8.0 / (iter->second.timeLastRxPacket.GetSeconds() - iter->second.timeFirstTxPacket.GetSeconds()) / 1024 << " Kbps"
              << "\n";
  }
  monitor->SerializeToXmlFile("scratch/practice_5.flowmon", false, true);
  Simulator::Destroy();

  return 0;
}
