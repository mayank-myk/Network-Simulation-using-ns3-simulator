# Network-Simulation-using-ns3-simulator
effect of data rate and buffer size on udp and tcp throughput

       H1 ---+      +--- H6
             |      |
       H2 ---R1 -- R2--- H5
             |      |
       H3 ---+      +--- H4

Comparing the effect of buffer size on TCP and UDP flows. Select a Dumbbell topology with two routers
R1 and R2 connected by a (10 Mbps,100 ms) link. Each of the routers is connected to 3 hosts i.e., H1, H2 and H3
are connected to R1 and, H4, H5 and H6 are connected to R2. All the hosts are attached to the routers with (100
Mbps, 10ms) links. Both the routers (i.e., R1 and R2) use drop-tail queues with equal queue size set according to
bandwidth-delay product. Choose a packet size of 1.5KB. Start 4 TCP New Reno flows and after a while start 2
CBR over UDP flows each with 20 Mbps. These flows are randomly distributed across H1, H2 and H3 Increase
the rate of one UDP flow up to 100 Mbps and observe its impact on the throughput of the TCP flows and the other
UDP flow. Vary the buffer size in the range of 10 packets to 800 packets and repeat the above experiments to find
out the impact of buffer size on the fair share of bandwidth and plot the necessary graphs.
