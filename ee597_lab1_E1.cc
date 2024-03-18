


#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/internet-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/command-line.h"
#include "ns3/config.h"
#include "ns3/double.h"
#include "ns3/string.h"
#include "ns3/log.h"
#include "ns3/yans-wifi-helper.h"
#include "ns3/mobility-helper.h"
#include "ns3/ipv4-address-helper.h"
#include "ns3/yans-wifi-channel.h"
#include "ns3/mobility-model.h"
#include "ns3/internet-stack-helper.h"
#include <ns3/txop.h>
#include <fstream>
#include <vector>
#include "ns3/csma-module.h"
#include<cmath>
#include <numeric>


using namespace ns3;

NS_LOG_COMPONENT_DEFINE("MyWifiSimulation"); // Define a logging component

int main(int argc, char *argv[]) {
    std::string phyMode ("DsssRate11Mbps"); // this is the maximum capped link rate that the PHY layer can handle
   // bool verbose = false;
    bool tracing = true;
    LogComponentEnable("MyWifiSimulation", LOG_LEVEL_DEBUG); // Enable logging with debug level

    CommandLine cmd;
    cmd.Parse(argc, argv);
int n=10; // no of nodes, you can also switch to a loop, but a loop was terribly slow in my computer. 
    int MinCw = 63;    // minimum CW , change these values to switch between Case A and B. 
    int MaxCw = 127;   // maximum CW
 std::vector<double> backoffTimes;
 

    //for (int n = 3; n <= 10; ++n) { // Vary the number of nodes from 3 to 10
        NodeContainer nodes;
        nodes.Create(n);
// Grid Based node positioning model.
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

// Install the MAC and Internet devices for the nodes 
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

        for (uint32_t i = 0; i < nodes.GetN(); ++i) {   // snippet for modifying the window sizes.
            Ptr<Node> node = nodes.Get(i);
            Ptr<NetDevice> dev = devices.Get(i);
            Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
            Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();

            if (wifi_mac) {
                PointerValue ptr;
                wifi_mac->GetAttribute("Txop", ptr);
                Ptr<Txop> txop = ptr.Get<Txop>();
                txop->SetMaxCw(MaxCw);
                txop->SetMinCw(MinCw);
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
                                    InetSocketAddress(interfaces.GetAddress(0), port));  // node 0 as a packet sink or receiver.
        sinkApps = sinkHelper.Install(nodes.Get(0));
        sink = StaticCast<PacketSink> (sinkApps.Get(0));
        sinkApps.Start(Seconds(1.0));
        sinkApps.Stop(Seconds(150.0));

        OnOffHelper onOffHelper("ns3::UdpSocketFactory", Address(InetSocketAddress(interfaces.GetAddress(0), port))); // all other nodes except node 0 as transmitters and send CBR to node 0
        onOffHelper.SetAttribute("OnTime", StringValue("ns3::ConstantRandomVariable[Constant=1]"));
        onOffHelper.SetAttribute("OffTime", StringValue("ns3::ConstantRandomVariable[Constant=0]"));
        onOffHelper.SetAttribute("DataRate", DataRateValue(DataRate("11Mbps")));   // link data rate
        onOffHelper.SetAttribute("PacketSize", UintegerValue(512));
        for (int i = 1; i < n; i++) {
            if (i != 0) {
                onOffApps.Add(onOffHelper.Install(nodes.Get(i)));
            }
        }
        onOffApps.Start(Seconds(2.0));
        onOffApps.Stop(Seconds(149.0));
        /*
        for (uint32_t i = 0; i < nodes.GetN(); ++i) {
        Ptr<NetDevice> dev = devices.Get(i);
            Ptr<WifiNetDevice> wifi_dev = DynamicCast<WifiNetDevice>(dev);
            Ptr<WifiMac> wifi_mac = wifi_dev->GetMac();
        if(wifi_mac){
            PointerValue ptr;
                wifi_mac->GetAttribute("Txop", ptr);
                Ptr<Txop> txop = ptr.Get<Txop>();
            uint32_t backoffSlots = ns3::Backoff::GetBackoffTime(void)	

            double backoffTime = backoffSlots * 20.0; // Each slot is 20 microseconds in 802.11b
            backoffTimes.push_back(backoffTime);}
            }*/
        
        
        if (tracing)
    {
    std::string pcapFileName = "node0_pcap_" + std::to_string(n);  // trace the simulation to a pcap file
phy.EnablePcap(pcapFileName, devices.Get(0));

    }
    
/*double mean = std::accumulate(backoffTimes.begin(), backoffTimes.end(), 0.0) / backoffTimes.size();

    // Calculate variance
    double sumOfSquares = std::accumulate(backoffTimes.begin(), backoffTimes.end(), 0.0, 
        [mean](double accumulator, double val) { return accumulator + (val - mean) * (val - mean); }
    );
    double variance = sumOfSquares / backoffTimes.size();

    // Print mean and variance
    std::cout << "Mean backoff time: " << mean << std::endl;
    std::cout << "Variance of backoff time: " << variance << std::endl;*/

        Simulator::Stop(Seconds(150));  // Simulation being done for 149-2=147 s. 
        Simulator::Run();

       // double NormAverageThroughput = ((sink->GetTotalRx() * 8) / (11 * 1e6 * (147)));   // % of maximum link rate
       // NS_LOG_INFO(n << ", " << NormAverageThroughput);

       // dataset.Add(n, NormAverageThroughput);
        Simulator::Destroy();
    

   // plot.AddDataset(dataset);
   // std::ofstream plotFile("throughput_plot.plt");
   // plot.GenerateOutput(plotFile);
   // plotFile.close();

    NS_LOG_INFO("Done.");
    return 0;
}

