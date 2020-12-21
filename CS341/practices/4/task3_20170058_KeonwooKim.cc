/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 IITP RAS
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
 *
 * Authors: Pavel Boyko <boyko@iitp.ru>
 */

#include <iostream>
#include <cmath>
#include "ns3/aodv-module.h"
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/v4ping-helper.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/v4traceroute-helper.h"

using namespace ns3;

class Task3 
{
public:
  Task3 ();
  /// Run simulation
  void Run ();

private:

  // parameters
  unsigned int size;
  /// Simulation time, seconds
  double totalTime;
  /// Print routes if true
  bool printRoutes;

  // network
  /// nodes used in the example
  NodeContainer nodes;
  /// devices used in the example
  NetDeviceContainer devices;
  /// interfaces used in the example
  Ipv4InterfaceContainer interfaces;

private:
  /// Create the nodes
  void CreateNodes ();
  /// Create the devices
  void CreateDevices ();
  /// Create the network
  void InstallInternetStack ();
  /// Create the simulation applications
  void InstallApplications ();
};

int main (int argc, char **argv)
{
  Task3 test;

  test.Run ();
  return 0;
}

//-----------------------------------------------------------------------------
Task3::Task3 () :
  // size (6), // 3.1
  size (5),
  totalTime (100),
  printRoutes (true)
{
}


void
Task3::Run ()
{
//  Config::SetDefault ("ns3::WifiRemoteStationManager::RtsCtsThreshold", UintegerValue (1)); // enable rts cts all the time.
  CreateNodes ();
  CreateDevices ();
  InstallInternetStack ();
  InstallApplications ();

  std::cout << "Starting simulation for " << totalTime << " s ...\n";

  Simulator::Stop (Seconds (totalTime));
  Simulator::Run ();
  Simulator::Destroy ();
}


void
Task3::CreateNodes ()
{
  std::cout << "Creating " << size << " nodes.\n";
  nodes.Create (size);
  // Name nodes
  for (uint32_t i = 0; i < size; ++i)
    {
      std::ostringstream os;
      os << "node-" << i;
      Names::Add (os.str (), nodes.Get (i));
    }
  // Create static grid
  MobilityHelper mobility;
  ListPositionAllocator posAllocator;
  // * You might need to adjust the number of nodes in the simulation
  // posAllocator.Add (Vector (0, 0, 0));
  // posAllocator.Add (Vector (25, 25, 0));
  // posAllocator.Add (Vector (40, 10, 0));
  // posAllocator.Add (Vector (50, 0, 0));
  // posAllocator.Add (Vector (50, 50, 0));
  // posAllocator.Add (Vector (100, 50, 0));

  posAllocator.Add (Vector (-15, 0, 0));
  posAllocator.Add (Vector (15, 0, 0));
  posAllocator.Add (Vector (39.21, 17.72, 0));
  posAllocator.Add (Vector (0, 48.74, 0));
  posAllocator.Add (Vector (-39.21, 17.72, 0));


  mobility.SetMobilityModel ("ns3::ConstantPositionMobilityModel");
  mobility.SetPositionAllocator (&posAllocator);
  mobility.Install (nodes);
}

void
Task3::CreateDevices ()
{
  WifiMacHelper wifiMac;
  wifiMac.SetType ("ns3::AdhocWifiMac");
  YansWifiPhyHelper wifiPhy = YansWifiPhyHelper::Default ();
  YansWifiChannelHelper wifiChannel = YansWifiChannelHelper::Default ();
  wifiPhy.SetChannel (wifiChannel.Create ());
  WifiHelper wifi;
  wifi.SetRemoteStationManager ("ns3::ConstantRateWifiManager", "DataMode", StringValue ("OfdmRate6Mbps"), "RtsCtsThreshold", UintegerValue (0));
  devices = wifi.Install (wifiPhy, wifiMac, nodes); 

}

void
Task3::InstallInternetStack ()
{
  AodvHelper aodv;
  // you can configure AODV attributes here using aodv.Set(name, value)
  InternetStackHelper stack;
  stack.SetRoutingHelper (aodv); // has effect on the next Install ()
  stack.Install (nodes);
  Ipv4AddressHelper address;
  address.SetBase ("10.0.0.0", "255.0.0.0");
  interfaces = address.Assign (devices);

  if (printRoutes)
    {
      Ptr<OutputStreamWrapper> routingStream = Create<OutputStreamWrapper> ("scratch/task3.routes", std::ios::out);
      aodv.PrintRoutingTableAllAt (Seconds (totalTime - 1), routingStream);
    }
}

void
Task3::InstallApplications ()
{
  // V4TraceRouteHelper traceroute (Ipv4Address ("10.0.0.6"));
  // traceroute.SetAttribute ("Verbose", BooleanValue (true));
  // ApplicationContainer p = traceroute.Install (nodes.Get (0));

  V4TraceRouteHelper traceroute (Ipv4Address ("10.0.0.5"));
  traceroute.SetAttribute ("Verbose", BooleanValue (true));
  ApplicationContainer p = traceroute.Install (nodes.Get (2));

  p.Start (Seconds (0));
  p.Stop (Seconds (totalTime) - Seconds (0.001));
}
