#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include <ns3/txop.h>
#include <fstream>
#include <vector>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MyWifiSimulation"); // Define a logging component

int main(int argc, char *argv[]) {
    std::string phyMode ("DsssRate11Mbps"); // Fixed capped rate, this is the maximum link rate, not to be confused with link rate.
  //  bool verbose = false;
    bool tracing = true;
    LogComponentEnable("MyWifiSimulation", LOG_LEVEL_DEBUG); // Enable logging with debug level

    CommandLine cmd;
    cmd.Parse(argc, argv);

    int MinCw = 63;    // minimum CW change this to switch between Case A and B. 
    int MaxCw = 127;   // maximum CW

   // Gnuplot plot;
   // plot.SetTitle("Data Rate (Mbps) vs. Normalized Throughput");
    //plot.SetLegend("Data Rate (Mbps)", "Normalized Throughput");
    //Gnuplot2dDataset dataset;
    int dataRateMbps=11;//[5]={1,3,5,7,11};  // the link rate variable.
  // for (int j = 0; j <5; j++) { // Vary the data rate from 1 to 11 Mbps
        int n = 20; // Fixed number of nodes
        NodeContainer nodes;
        nodes.Create(n);

        MobilityHelper mobility;
        Ptr<GridPositionAllocator> positionAlloc = CreateObject<GridPositionAllocator>();
        positionAlloc->SetAttribute("MinX", DoubleValue(0.0));
        positionAlloc->SetAttribute("MinY", DoubleValue(0.0));
        positionAlloc->SetAttribute("DeltaX", DoubleValue(1.0));
        positionAlloc->SetAttribute("DeltaY", DoubleValue(1.0));
        positionAlloc->SetAttribute("GridWidth", UintegerValue(100));
        positionAlloc->SetAttribute("LayoutType", StringValue("RowFirst"));
        mobility.SetPositionAllocator(positionAlloc);
        mobility.SetMobilityModel("ns3::ConstantPositionMobilityModel");
        mobility.Install(nodes);

        WifiHelper wifi;
        wifi.SetStandard(WIFI_STANDARD_80211b);
        YansWifiPhyHelper phy;
        phy.Set("RxGain", DoubleValue(0));
        phy.SetPcapDataLinkType(WifiPhyHelper::DLT_IEEE802_11_RADIO);
        YansWifiChannelHelper channel = YansWifiChannelHelper::Default();
        phy.SetChannel(channel.Create());
        WifiMacHelper mac;
        wifi.SetRemoteStationManager("ns3::ConstantRateWifiManager",
                                    "DataMode", StringValue(phyMode),
                                    "ControlMode", StringValue(phyMode));
        NetDeviceContainer devices = wifi.Install(phy, mac, nodes);

        for (uint32_t i = 0; i < nodes.GetN(); ++i) {
            Ptr<Node> node = nodes.Get(i);
            Ptr<NetDevice> dev = devices.Get(i);
            Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
            Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();

            if (wifi_mac) {
                PointerValue ptr;
                wifi_mac->GetAttribute("Txop", ptr);
                Ptr<Txop> txop = ptr.Get<Txop>();
                txop->SetMaxCw(MaxCw); // Maximum CW
                txop->SetMinCw(MinCw);    // Minimum CW
            } else {
                NS_LOG_ERROR("Node " << i << " does not have a Txop object!");
            }
        }

        InternetStackHelper internet;
        internet.Install(nodes);

        Ipv4AddressHelper address;
        address.SetBase("10.1.1.0", "255.255.255.0");
        Ipv4InterfaceContainer interfaces = address.Assign(devices);

        uint16_t port = 9;
        ApplicationContainer onOffApps;
        ApplicationContainer sinkApps;
        Ptr<PacketSink> sink; 
        PacketSinkHelper sinkHelper("ns3::UdpSocketFactory",
                                    InetSocketAddress(interfaces.GetAddress(0), port));
        sinkApps = sinkHelper.Install(nodes.Get(0));
        sink = StaticCast<PacketSink> (sinkApps.Get(0));
        sinkApps.Start(Seconds(1.0));
        sinkApps.Stop(Seconds(150.0));

        OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces.GetAddress(0), port)));
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        //onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("11Mbps")));
        onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate(std::to_string(dataRateMbps)+"Mbps"))); // Set  the data rate/ link rate here!
        onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
        for (int i = 1; i < n; i++) {
            if (i != 0) {
                onOffApps.Add(onOffHelper.Install(nodes.Get(i)));
            }
        }
        onOffApps.Start(Seconds(2.0));
        onOffApps.Stop(Seconds(149.0));
        
        if (tracing)
    {
    std::string pcapFileName = "node0_pcap_" + std::to_string(dataRateMbps);  // create a file with a name: node0_pcap_+datarate+0-0.pcap
phy.EnablePcap(pcapFileName, devices.Get(0));

    }

        Simulator::Stop(Seconds(150));
        Simulator::Run();

       // double NormAverageThroughput = ((sink->GetTotalRx() * 8) / (dataRateMbps * 1e6 * (147)));   // % of maximum link rate rather than just data rate, remove dataRateMbps from the denominator if you don't want that but just average throughput (i.e not norm)
        //NS_LOG_INFO(dataRateMbps << " Mbps, " << NormAverageThroughput);

        //dataset.Add(dataRateMbps, NormAverageThroughput);
        Simulator::Destroy();
   // }

   // plot.AddDataset(dataset);
    //std::ofstream plotFile("R_v_throughput_plot.plt");
   // plot.GenerateOutput(plotFile);
   // plotFile.close();

    NS_LOG_INFO("Done.");
    return 0;
}

