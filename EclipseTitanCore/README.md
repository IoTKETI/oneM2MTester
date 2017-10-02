## Dependency on Eclipse Titan Core version

Before running oneM2MTester, user has to build the oneM2MTester with Eclipse Titan TTCN-3 compiler. 
According to our knowledge and experience during the development of oneM2MTester, we found that the 
Titan core version (i.e. Titan TTCN-3 compiler version) impacts the compiling result of oneM2MTester.
Because oneM2MTester project incorporates oneM2M abstract test suite which defines a bunch of oneM2M
specific data types, whose complexity impacts the TTCN-3 compiler's performance.

Due to this reason, we provide a specific version of Eclipse Titan core in the repository, which has been verified 
to work well with oneM2MTester. Before running oneM2MTester, please make sure you are using the latest oneM2MTester
code in the GitHub, and also the Titan Core version you are using matches with the one offered to you in the GitHub.

oneM2MTester Configuration:
- Operating System: Ubuntu 14.04 LTS
- Eclipse Version: Mars.2 Release (4.5.2) 
- Titan Version: 113 200/6 R2A (Build date: May 26 2017)

Reference URL:
https://github.com/eclipse/titan.core/blob/master/README.linux
https://projects.eclipse.org/projects/tools.titan/downloads

Reference Document:
oneM2MTester_User_Guide_ver1_0_0.pdf in oneM2MTester_User_Manual folder
Installation_Guide_for_the_TITAN_TTCN-3_Test_Executor_2017.pdf in Eclipse_TITAN_Documentation folder




