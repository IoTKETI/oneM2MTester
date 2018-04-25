## Dependency on Eclipse Titan Core version

Before running oneM2MTester, user has to build the oneM2MTester with Eclipse Titan TTCN-3 compiler. 
According to our knowledge and experience during the development of oneM2MTester, we found that the 
Titan core version (i.e. Titan TTCN-3 compiler version) impacts the compiling result of oneM2MTester.
Because oneM2MTester project incorporates oneM2M abstract test suite which defines a bunch of oneM2M
specific data types, whose complexity impacts the TTCN-3 compiler's performance.

Due to this reason, we provide a specific version URL of Eclipse Titan Core in this repository, which has been verified 
to work well with oneM2MTester. Before running oneM2MTester, please make sure you are using the same Eclipse Titan Core used in oneM2MTester.

oneM2MTester Configuration:
- Operating System: Ubuntu 16.04 LTS
- Eclipse Version: Neon.3 Release (4.6.3) 
- Titan Core Version: https://github.com/eclipse/titan.core/tree/78a82971bdd606e92e522b9808e27991ddad7dc9