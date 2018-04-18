#include <fstream>
#include <string>
#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/point-to-point-module.h"
#include "ns3/point-to-point-layout-module.h"
#include "ns3/applications-module.h"
#include "ns3/internet-module.h"
#include "ns3/flow-monitor-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/gnuplot.h"

using namespace ns3;


NS_LOG_COMPONENT_DEFINE("Assignment_4_logs");

class MyApp : public Application
{
public:

	MyApp();
	virtual ~MyApp();

	void Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate);
	void ChangeRate(DataRate newrate);

private:
	virtual void StartApplication(void);
	virtual void StopApplication(void);

	void ScheduleTx(void);
	void SendPacket(void);

	Ptr<Socket>	 m_socket;
	Address	 m_peer;
	uint32_t	m_packetSize;
	uint32_t	m_nPackets;
	DataRate	m_dataRate;
	EventId	 m_sendEvent;
	bool	m_running;
	uint32_t	m_packetsSent;
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

void // setting up the parameters of the object
MyApp::Setup(Ptr<Socket> socket, Address address, uint32_t packetSize, uint32_t nPackets, DataRate dataRate)
{
	m_socket = socket;
	m_peer = address;
	m_packetSize = packetSize;
	m_nPackets = nPackets;
	m_dataRate = dataRate;
}

void //function used to start the application with the provided values
MyApp::StartApplication(void)
{
	m_running = true;
	m_packetsSent = 0;
	m_socket->Bind();
	m_socket->Connect(m_peer);
	SendPacket();
}

void
MyApp::StopApplication(void)
{
	m_running = false;

	if(m_sendEvent.IsRunning())
	{
		Simulator::Cancel(m_sendEvent);
	}

	if(m_socket)
	{
		m_socket->Close();
	}
}

void
MyApp::SendPacket(void)
{
	Ptr<Packet> packet = Create<Packet>(m_packetSize);
	m_socket->Send(packet);

	if(++m_packetsSent < m_nPackets)
	{
		ScheduleTx();
	}
}


void
MyApp::ScheduleTx(void)
{
	if(m_running)
	{
		Time tNext(Seconds(m_packetSize * 8 / static_cast<double>(m_dataRate.GetBitRate())));
		m_sendEvent = Simulator::Schedule(tNext, &MyApp::SendPacket, this);
	}
}

void
MyApp::ChangeRate(DataRate newrate)
{
	m_dataRate = newrate;
	return;
}

void
IncRate(Ptr<MyApp> app, DataRate rate)
{
	app->ChangeRate(rate);
	return;
}



//---------------------MAIN FUNCTION STARTS HERE-------------------------------------------------


int main( int argc, char* argv[]){

	Time::SetResolution(Time::NS);
	Config::SetDefault("ns3::TcpL4Protocol::SocketType", TypeIdValue(TcpNewReno::GetTypeId()));
	
	//details of the file used to store the output and graph
	std :: string fileNameWithNoExtension = "Fairness_Index";
	std :: string graphicsFileName	= fileNameWithNoExtension + ".png";
	std :: string plotFileName	= fileNameWithNoExtension + ".plt";
	std :: string plotTitle	 = "Effect of Buffer Size on Fairness Index";
	std :: string dataTitle	 = "Fairness Index";


	// Instantiate the plot and set its title.
	Gnuplot plot(graphicsFileName);
	plot.SetTitle(plotTitle);

	// Make the graphics file, which the plot file will create when it
	// is used with Gnuplot, be a PNG file.
	plot.SetTerminal("png");
	// Set the labels for each axis.
	plot.SetLegend("Buffer Size (in packets)", "Fairness Index");
	// Set the range for the x axis.
	plot.AppendExtra("set xrange [0:800]");
	// Instantiate the dataset, set its title, and make the points be
	// plotted along with connecting lines.
	Gnuplot2dDataset dataset;
	dataset.SetTitle(dataTitle);
	dataset.SetStyle(Gnuplot2dDataset::LINES_POINTS);

	for(int buffer_size=10*1500;buffer_size<=800*1500;buffer_size+=200*1500){

		NS_LOG_INFO("Creating Nodes.");
		NodeContainer nodes;
		nodes.Create(8); //8 nodes created 6 for hosts and 2 for routers

		NodeContainer R1R2 = NodeContainer(nodes.Get(0), nodes.Get(1)); //R1-R2 router link
		NodeContainer R1H1 = NodeContainer(nodes.Get(0), nodes.Get(2)); //R1-H1 router 1 to host links
		NodeContainer R1H2 = NodeContainer(nodes.Get(0), nodes.Get(3)); //R1-H2
		NodeContainer R1H3 = NodeContainer(nodes.Get(0), nodes.Get(4)); //R1-H3
		NodeContainer R2H4 = NodeContainer(nodes.Get(1), nodes.Get(5)); //R2-H4 router 2 to host links
		NodeContainer R2H5 = NodeContainer(nodes.Get(1), nodes.Get(6)); //R2-H5
		NodeContainer R2H6 = NodeContainer(nodes.Get(1), nodes.Get(7)); //R2-H6

		InternetStackHelper mynetwork;
		mynetwork.Install(nodes); 

		//creating channels from nodes to routers of 100Mbps datarate and 10ms delay
		NS_LOG_INFO("Creating channels from nodes to routers");
		PointToPointHelper node2router;
		node2router.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
		node2router.SetChannelAttribute("Delay", StringValue("10ms"));
		NetDeviceContainer r1h1 = node2router.Install(R1H1);
		NetDeviceContainer r1h2 = node2router.Install(R1H2);
		NetDeviceContainer r1h3 = node2router.Install(R1H3);
		NetDeviceContainer r2h4 = node2router.Install(R2H4);
		NetDeviceContainer r2h5 = node2router.Install(R2H5);
		NetDeviceContainer r2h6 = node2router.Install(R2H6);
		
		//creating channel from router1 to router2 of 10Mbps datarate and 100ms delay 
		NS_LOG_INFO("Creating channels from routers to routers");
		PointToPointHelper router2router;
		router2router.SetQueue("ns3::DropTailQueue");//setting the routers to use drop tail queue 
		router2router.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
		router2router.SetChannelAttribute("Delay", StringValue("100ms"));
		NetDeviceContainer r1r2 = router2router.Install(R1R2);

		NS_LOG_INFO("Assigning IPv4 IP addresses to the nodes.");
		Ipv4AddressHelper IP;
		IP.SetBase("10.1.7.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router_link = IP.Assign(r1r2);

		IP.SetBase("10.1.1.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node1 =IP.Assign(r1h1);

		IP.SetBase("10.1.2.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node2 = IP.Assign(r1h2);

		IP.SetBase("10.1.3.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node3 = IP.Assign(r1h3);

		IP.SetBase("10.1.4.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node4 = IP.Assign(r2h4);

		IP.SetBase("10.1.5.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node5 = IP.Assign(r2h5);

		IP.SetBase("10.1.6.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node6 = IP.Assign(r2h6);

		//enabling static global routing
		Ipv4GlobalRoutingHelper::PopulateRoutingTables();


		//creating 4 TCP connections---------------------------------------------------------------------

		//CONNECTING H1 to H4 Across the routers
		uint32_t recv_port1	= 8000;//port of the connection
		Address recv_addr1(InetSocketAddress(router2_node4.GetAddress(1), recv_port1));
		PacketSinkHelper myhelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port1));
		ApplicationContainer simulation1 = myhelper1.Install(nodes.Get(5));
		simulation1.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation1.Stop(Seconds(10.0));//end time of the simulation

		Ptr<Socket> tcp_socket1 = Socket::CreateSocket(nodes.Get(2), TcpSocketFactory::GetTypeId());
		tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket1->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_1 = CreateObject<MyApp>();
		App_1->Setup(tcp_socket1, recv_addr1, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(2)->AddApplication(App_1);
		App_1->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_1->SetStopTime(Seconds(10.0));


		//CONNECTING H6 to H3 Across the routers
		uint32_t recv_port2	= 8001;//port of the connection
		Address recv_addr2(InetSocketAddress(router1_node3.GetAddress(1), recv_port2));
		PacketSinkHelper myhelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port2));
		ApplicationContainer simulation2 = myhelper2.Install(nodes.Get(4));
		simulation2.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation2.Stop(Seconds(10.0));

		Ptr<Socket> tcp_socket2 = Socket::CreateSocket(nodes.Get(7), TcpSocketFactory::GetTypeId());
		tcp_socket2->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket2->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_2 = CreateObject<MyApp>();
		App_2->Setup(tcp_socket2, recv_addr2, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(7)->AddApplication(App_2);
		App_2->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_2->SetStopTime(Seconds(10.0));
	
		//CONNECTING H2 to H1
		uint32_t recv_port3	= 8002;//port of the connection
		Address recv_addr3(InetSocketAddress(router1_node1.GetAddress(1), recv_port3));
		PacketSinkHelper myhelper3("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port3));
		ApplicationContainer simulation3 = myhelper3.Install(nodes.Get(2));
		simulation3.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation3.Stop(Seconds(10.0));

		Ptr<Socket> tcp_socket3 = Socket::CreateSocket(nodes.Get(3), TcpSocketFactory::GetTypeId());
		tcp_socket3->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket3->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_3 = CreateObject<MyApp>();
		App_3->Setup(tcp_socket3, recv_addr3, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(3)->AddApplication(App_3);
		App_3->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_3->SetStopTime(Seconds(10.0));

		
		// //CONNECTING H5 to H4 (UDP)
		uint32_t recv_port4	= 8003;//port of the connection
		Address recv_addr4(InetSocketAddress(router2_node4.GetAddress(1), recv_port4));
		PacketSinkHelper myhelper4("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port4));
		ApplicationContainer simulation4 = myhelper4.Install(nodes.Get(5));
		simulation4.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation4.Stop(Seconds(10.0));

		Ptr<Socket> tcp_socket4 = Socket::CreateSocket(nodes.Get(6), UdpSocketFactory::GetTypeId ());
		//tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket4->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_4 = CreateObject<MyApp>();
		App_4->Setup(tcp_socket4, recv_addr4, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(6)->AddApplication(App_4);
		App_4->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_4->SetStopTime(Seconds(10.0));


		//CONNECTING H2 to H5 (UDP)
		uint32_t recv_port5	= 8004;//port of the connection
		Address recv_addr5(InetSocketAddress(router2_node5.GetAddress(1), recv_port5));
		PacketSinkHelper myhelper5("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port5));
		ApplicationContainer simulation5 = myhelper5.Install(nodes.Get(6));
		simulation5.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation5.Stop(Seconds(10.0));

		Ptr<Socket> tcp_socket5 = Socket::CreateSocket(nodes.Get(3), UdpSocketFactory::GetTypeId ());
		//tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket5->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_5 = CreateObject<MyApp>();
		App_5->Setup(tcp_socket5, recv_addr5, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(6)->AddApplication(App_5);
		App_5->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_5->SetStopTime(Seconds(10.0));	

		//CONNECTING H6 to H4(tcp) Across the routers
		uint32_t recv_port6	= 8005;//port of the connection
		Address recv_addr6(InetSocketAddress(router2_node4.GetAddress(1), recv_port6));
		PacketSinkHelper myhelper6("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port6));
		ApplicationContainer simulation6 = myhelper6.Install(nodes.Get(5));
		simulation6.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation6.Stop(Seconds(10.0));

		Ptr<Socket> tcp_socket6 = Socket::CreateSocket(nodes.Get(7), TcpSocketFactory::GetTypeId());
		tcp_socket6->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket6->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_6 = CreateObject<MyApp>();
		App_6->Setup(tcp_socket6, recv_addr6, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(7)->AddApplication(App_6);
		App_6->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_6->SetStopTime(Seconds(10.0));//stopping the transmission at 
	
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();


		NS_LOG_INFO("Starting Simulation.");
		Simulator::Stop(Seconds(15.0));
		Simulator::Run();

		monitor->CheckForLostPackets();

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
		double Sumx = 0, SumSqx = 0;
		int n = 0;

		std::cout << "\nBuffer Size:   " << buffer_size/1500 << "\n";//printing the current buffer size in no of packets
		for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
		{
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
			std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") - ";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
			  
			double TPut = i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
			Sumx += TPut;
			SumSqx += TPut * TPut ;
			n++;
		}

		double FairnessIndex =(Sumx * Sumx)/(6 * SumSqx) ;
		dataset.Add(buffer_size/1500, FairnessIndex);
		std :: cout << " FairnessIndex:	" << FairnessIndex << std :: endl;

		monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

		Simulator::Destroy();
	
	}

	plot.AddDataset(dataset);
	// Open the plot file.
	std :: ofstream plotFile(plotFileName.c_str());
	// Write the plot file.
	plot.GenerateOutput(plotFile);
	// Close the plot file.
	plotFile.close();
//----------------------------------------------------------------------------------------------------------------------

	std :: string fileNameWithNoExtension2 = "TCP_throughput1";
	std :: string graphicsFileName2	= fileNameWithNoExtension2 + ".png";
	std :: string plotFileName2	= fileNameWithNoExtension2 + ".plt";
	std :: string plotTitle2	 = "Effect of UDP datarate on TCP Throughput from H1 to H4";
	std :: string dataTitle2	 = "Throughput";

	Gnuplot plot2(graphicsFileName2);
	plot2.SetTitle(plotTitle2);
	plot2.SetTerminal("png");
	plot2.SetLegend("UDP DataRate", "TCP Throughput");
	plot2.AppendExtra("set xrange [0:100]");
	Gnuplot2dDataset dataset2;
	dataset2.SetTitle(dataTitle2);
	dataset2.SetStyle(Gnuplot2dDataset::LINES_POINTS);


	std :: string fileNameWithNoExtension3 = "UDP_throughput";
	std :: string graphicsFileName3	= fileNameWithNoExtension3 + ".png";
	std :: string plotFileName3	= fileNameWithNoExtension3 + ".plt";
	std :: string plotTitle3	 = "Effect of UDP datarate on UDP Throughput from H5 to H4";
	std :: string dataTitle3	 = "Throughput";


	// Instantiate the plot and set its title.
	Gnuplot plot3(graphicsFileName3);
	plot3.SetTitle(plotTitle3);

	// Make the graphics file, which the plot file will create when it
	// is used with Gnuplot, be a PNG file.
	plot3.SetTerminal("png");
	// Set the labels for each axis.
	plot3.SetLegend("UDP DataRate", "TCP Throughput");
	// Set the range for the x axis.
	plot3.AppendExtra("set xrange [0:100]");
	// Instantiate the dataset, set its title, and make the points be
	// plotted along with connecting lines.
	Gnuplot2dDataset dataset3;
	dataset3.SetTitle(dataTitle3);
	dataset3.SetStyle(Gnuplot2dDataset::LINES_POINTS);


	std :: string fileNameWithNoExtension4 = "TCP_throughput2";
	std :: string graphicsFileName4	= fileNameWithNoExtension4 + ".png";
	std :: string plotFileName4	= fileNameWithNoExtension4 + ".plt";
	std :: string plotTitle4	 = "Effect of UDP datarate on TCP Throughput from H2 to H1";
	std :: string dataTitle4	 = "Throughput";


	// Instantiate the plot and set its title.
	Gnuplot plot4(graphicsFileName4);
	plot4.SetTitle(plotTitle4);

	// Make the graphics file, which the plot file will create when it
	// is used with Gnuplot, be a PNG file.
	plot4.SetTerminal("png");
	// Set the labels for each axis.
	plot4.SetLegend("UDP DataRate", "TCP Throughput");
	// Set the range for the x axis.
	plot4.AppendExtra("set xrange [0:100]");
	// Instantiate the dataset, set its title, and make the points be
	// plotted along with connecting lines.
	Gnuplot2dDataset dataset4;
	dataset4.SetTitle(dataTitle4);
	dataset4.SetStyle(Gnuplot2dDataset::LINES_POINTS);


	std :: string fileNameWithNoExtension5 = "TCP_throughput3";
	std :: string graphicsFileName5	= fileNameWithNoExtension5 + ".png";
	std :: string plotFileName5	= fileNameWithNoExtension5 + ".plt";
	std :: string plotTitle5	 = "Effect of UDP datarate on TCP Throughput from H6 to H3";
	std :: string dataTitle5	 = "Throughput";

	Gnuplot plot5(graphicsFileName5);
	plot5.SetTitle(plotTitle5);
	plot5.SetTerminal("png");
	plot5.SetLegend("UDP DataRate", "TCP Throughput");
	plot5.AppendExtra("set xrange [0:100]");
	Gnuplot2dDataset dataset5;
	dataset5.SetTitle(dataTitle5);
	dataset5.SetStyle(Gnuplot2dDataset::LINES_POINTS);



	std :: string fileNameWithNoExtension6 = "TCP_throughput4";
	std :: string graphicsFileName6	= fileNameWithNoExtension6 + ".png";
	std :: string plotFileName6	= fileNameWithNoExtension6 + ".plt";
	std :: string plotTitle6	 = "Effect of UDP datarate on TCP Throughput from H6 to H4";
	std :: string dataTitle6	 = "Throughput";

	Gnuplot plot6(graphicsFileName6);
	plot6.SetTitle(plotTitle6);
	plot6.SetTerminal("png");
	plot6.SetLegend("UDP DataRate", "TCP Throughput");
	plot6.AppendExtra("set xrange [0:100]");
	Gnuplot2dDataset dataset6;
	dataset6.SetTitle(dataTitle6);
	dataset6.SetStyle(Gnuplot2dDataset::LINES_POINTS);



	for(int data_rate = 20; data_rate<=100; data_rate += 20){

		int buffer_size = 100 * 1500;

		std :: string new_data_rate = std :: to_string(data_rate) + "Mbps";
		NS_LOG_INFO("Creating Nodes.");
		NodeContainer nodes;
		nodes.Create(8); //8 nodes created 6 for hosts and 2 for routers

		NodeContainer R1R2 = NodeContainer(nodes.Get(0), nodes.Get(1)); //R1-R2 router link
		NodeContainer R1H1 = NodeContainer(nodes.Get(0), nodes.Get(2)); //R1-H1 router 1 to host links
		NodeContainer R1H2 = NodeContainer(nodes.Get(0), nodes.Get(3)); //R1-H2
		NodeContainer R1H3 = NodeContainer(nodes.Get(0), nodes.Get(4)); //R1-H3
		NodeContainer R2H4 = NodeContainer(nodes.Get(1), nodes.Get(5)); //R2-H4 router 2 to host links
		NodeContainer R2H5 = NodeContainer(nodes.Get(1), nodes.Get(6)); //R2-H5
		NodeContainer R2H6 = NodeContainer(nodes.Get(1), nodes.Get(7)); //R2-H6

		InternetStackHelper mynetwork;
		mynetwork.Install(nodes); 

		//creating channels from nodes to routers of 100Mbps datarate and 10ms delay
		NS_LOG_INFO("Creating channels from nodes to routers");
		PointToPointHelper node2router;
		node2router.SetDeviceAttribute("DataRate", StringValue("100Mbps"));
		node2router.SetChannelAttribute("Delay", StringValue("10ms"));
		NetDeviceContainer r1h1 = node2router.Install(R1H1);
		NetDeviceContainer r1h2 = node2router.Install(R1H2);
		NetDeviceContainer r1h3 = node2router.Install(R1H3);
		NetDeviceContainer r2h4 = node2router.Install(R2H4);
		NetDeviceContainer r2h5 = node2router.Install(R2H5);
		NetDeviceContainer r2h6 = node2router.Install(R2H6);
		
		//creating channel from router1 to router2 of 10Mbps datarate and 100ms delay 
		NS_LOG_INFO("Creating channels from routers to routers");
		PointToPointHelper router2router;
		router2router.SetQueue("ns3::DropTailQueue");//setting the routers to use drop tail queue 
		router2router.SetDeviceAttribute("DataRate", StringValue("10Mbps"));
		router2router.SetChannelAttribute("Delay", StringValue("100ms"));
		NetDeviceContainer r1r2 = router2router.Install(R1R2);

		NS_LOG_INFO("Assigning IPv4 IP addresses to the nodes.");
		Ipv4AddressHelper IP;
		IP.SetBase("10.1.7.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router_link = IP.Assign(r1r2);

		IP.SetBase("10.1.1.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node1 =IP.Assign(r1h1);

		IP.SetBase("10.1.2.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node2 = IP.Assign(r1h2);

		IP.SetBase("10.1.3.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router1_node3 = IP.Assign(r1h3);

		IP.SetBase("10.1.4.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node4 = IP.Assign(r2h4);

		IP.SetBase("10.1.5.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node5 = IP.Assign(r2h5);

		IP.SetBase("10.1.6.0", "255.255.255.0");//setting base ip address and netmask
		Ipv4InterfaceContainer router2_node6 = IP.Assign(r2h6);

		//enabling static global routing
		Ipv4GlobalRoutingHelper::PopulateRoutingTables();


		//creating 4 TCP connections---------------------------------------------------------------------

		//CONNECTING H1 to H4 Across the routers
		uint32_t recv_port1	= 8000;//port of the connection
		Address recv_addr1(InetSocketAddress(router2_node4.GetAddress(1), recv_port1));
		PacketSinkHelper myhelper1("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port1));
		ApplicationContainer simulation1 = myhelper1.Install(nodes.Get(5));
		simulation1.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation1.Stop(Seconds(100.0));//end time of the simulation

		Ptr<Socket> tcp_socket1 = Socket::CreateSocket(nodes.Get(2), TcpSocketFactory::GetTypeId());
		tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket1->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_1 = CreateObject<MyApp>();
		App_1->Setup(tcp_socket1, recv_addr1, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(2)->AddApplication(App_1);
		App_1->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_1->SetStopTime(Seconds(100.0));


		//CONNECTING H6 to H3 Across the routers
		uint32_t recv_port2	= 8001;//port of the connection
		Address recv_addr2(InetSocketAddress(router1_node3.GetAddress(1), recv_port2));
		PacketSinkHelper myhelper2("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port2));
		ApplicationContainer simulation2 = myhelper2.Install(nodes.Get(4));
		simulation2.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation2.Stop(Seconds(100.0));

		Ptr<Socket> tcp_socket2 = Socket::CreateSocket(nodes.Get(7), TcpSocketFactory::GetTypeId());
		tcp_socket2->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket2->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_2 = CreateObject<MyApp>();
		App_2->Setup(tcp_socket2, recv_addr2, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(7)->AddApplication(App_2);
		App_2->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_2->SetStopTime(Seconds(100.0));
	
		//CONNECTING H2 to H1
		uint32_t recv_port3	= 8002;//port of the connection
		Address recv_addr3(InetSocketAddress(router1_node1.GetAddress(1), recv_port3));
		PacketSinkHelper myhelper3("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port3));
		ApplicationContainer simulation3 = myhelper3.Install(nodes.Get(2));
		simulation3.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation3.Stop(Seconds(100.0));

		Ptr<Socket> tcp_socket3 = Socket::CreateSocket(nodes.Get(3), TcpSocketFactory::GetTypeId());
		tcp_socket3->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket3->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_3 = CreateObject<MyApp>();
		App_3->Setup(tcp_socket3, recv_addr3, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(3)->AddApplication(App_3);
		App_3->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_3->SetStopTime(Seconds(100.0));

		
		// //CONNECTING H5 to H4 (UDP)
		uint32_t recv_port4	= 8003;//port of the connection
		Address recv_addr4(InetSocketAddress(router2_node4.GetAddress(1), recv_port4));
		PacketSinkHelper myhelper4("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port4));
		ApplicationContainer simulation4 = myhelper4.Install(nodes.Get(5));
		simulation4.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation4.Stop(Seconds(100.0));

		Ptr<Socket> tcp_socket4 = Socket::CreateSocket(nodes.Get(6), UdpSocketFactory::GetTypeId ());
		//tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket4->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_4 = CreateObject<MyApp>();
		App_4->Setup(tcp_socket4, recv_addr4, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(6)->AddApplication(App_4);
		App_4->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_4->SetStopTime(Seconds(100.0));


		//CONNECTING H2 to H5 (UDP)
		uint32_t recv_port5	= 8004;//port of the connection
		Address recv_addr5(InetSocketAddress(router2_node5.GetAddress(1), recv_port5));
		PacketSinkHelper myhelper5("ns3::UdpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port5));
		ApplicationContainer simulation5 = myhelper5.Install(nodes.Get(6));
		simulation5.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation5.Stop(Seconds(100.0));

		Ptr<Socket> tcp_socket5 = Socket::CreateSocket(nodes.Get(3), UdpSocketFactory::GetTypeId ());
		//tcp_socket1->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket5->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_5 = CreateObject<MyApp>();
		App_5->Setup(tcp_socket5, recv_addr5, 1040, 1000000, DataRate(new_data_rate));
		nodes.Get(6)->AddApplication(App_5);
		App_5->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_5->SetStopTime(Seconds(100.0));	

		//CONNECTING H6 to H4(tcp) Across the routers
		uint32_t recv_port6	= 8005;//port of the connection
		Address recv_addr6(InetSocketAddress(router2_node4.GetAddress(1), recv_port6));
		PacketSinkHelper myhelper6("ns3::TcpSocketFactory", InetSocketAddress(Ipv4Address::GetAny(), recv_port6));
		ApplicationContainer simulation6 = myhelper6.Install(nodes.Get(5));
		simulation6.Start(Seconds(0.0));//starting simulation at 0 seconds
		simulation6.Stop(Seconds(100.0));

		Ptr<Socket> tcp_socket6 = Socket::CreateSocket(nodes.Get(7), TcpSocketFactory::GetTypeId());
		tcp_socket6->SetAttribute("SndBufSize",ns3::UintegerValue(buffer_size));
		tcp_socket6->SetAttribute("RcvBufSize",ns3::UintegerValue(buffer_size));
	
		Ptr<MyApp> App_6 = CreateObject<MyApp>();
		App_6->Setup(tcp_socket6, recv_addr6, 1040, 1000000, DataRate("20Mbps"));
		nodes.Get(7)->AddApplication(App_6);
		App_6->SetStartTime(Seconds(1.0));//starting simulation at 1 seconds
		App_6->SetStopTime(Seconds(100.0));//stopping the transmission at 
	
		FlowMonitorHelper flowmon;
		Ptr<FlowMonitor> monitor = flowmon.InstallAll();


		NS_LOG_INFO("Starting Simulation.");
		Simulator::Stop(Seconds(15.0));
		Simulator::Run();

		monitor->CheckForLostPackets();

		Ptr<Ipv4FlowClassifier> classifier = DynamicCast<Ipv4FlowClassifier>(flowmon.GetClassifier());
		std::map<FlowId, FlowMonitor::FlowStats> stats = monitor->GetFlowStats();
		double Sumx = 0, SumSqx = 0;
		int n = 0;

		std::cout << "\nBuffer Size:   " << buffer_size/1500 << "\n";//printing the current buffer size in no of packets
		double throughput;
		double throughput2;
		double throughput3;
		double throughput4;
		double throughput5;
		int j = 0;
		for(std::map<FlowId, FlowMonitor::FlowStats>::const_iterator i = stats.begin(); i != stats.end(); ++i)
		{	
			Ipv4FlowClassifier::FiveTuple t = classifier->FindFlow (i->first);
      
			std::cout << "Flow " << i->first  << " (" << t.sourceAddress << " -> " << t.destinationAddress << ") - ";
			std::cout << "  Tx Bytes:   " << i->second.txBytes << "\n";
			std::cout << "  Rx Bytes:   " << i->second.rxBytes << "\n";
			std::cout << "  Throughput: " << i->second.rxBytes * 8.0 / (i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024  << " Mbps\n";
			  
			double TPut = i->second.rxBytes * 8.0 /(i->second.timeLastRxPacket.GetSeconds() - i->second.timeFirstTxPacket.GetSeconds())/1024/1024;
			Sumx += TPut;
			SumSqx += TPut * TPut ;
			n++;

			if(j==0)
				throughput = TPut;
			if(j==1)
				throughput3 = TPut;
			if(j==2)
				throughput2 = TPut;
			if(j==4)
				throughput4 = TPut;
			if(j==5)
				throughput5 = TPut;
			j++;
		}

		
		dataset2.Add(data_rate, throughput);
		dataset3.Add(data_rate, throughput2);
		dataset4.Add(data_rate, throughput3);
		dataset5.Add(data_rate, throughput4);
		dataset6.Add(data_rate, throughput5);

		monitor->SerializeToXmlFile("lab-1.flowmon", true, true);

		Simulator::Destroy();
		

	}

	plot2.AddDataset(dataset2);
	std :: ofstream plotFile2(plotFileName2.c_str());
	plot2.GenerateOutput(plotFile2);
	plotFile2.close();

	plot3.AddDataset(dataset3);
	std :: ofstream plotFile3(plotFileName3.c_str());
	plot3.GenerateOutput(plotFile3);
	plotFile3.close();

	plot4.AddDataset(dataset4);
	std :: ofstream plotFile4(plotFileName4.c_str());
	plot4.GenerateOutput(plotFile4);
	plotFile4.close();

	plot5.AddDataset(dataset5);
	std :: ofstream plotFile5(plotFileName5.c_str());
	plot5.GenerateOutput(plotFile5);
	plotFile5.close();

	plot6.AddDataset(dataset6);
	std :: ofstream plotFile6(plotFileName6.c_str());
	plot6.GenerateOutput(plotFile6);
	plotFile6.close();

	NS_LOG_INFO("Done.");
}
