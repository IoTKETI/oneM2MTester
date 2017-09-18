## Dependency on Eclipse Titan Core version

Before running oneM2MTester(v2.0.0), user has to build the oneM2MTester with Eclipse Titan TTCN-3 compiler. 
According to our knowledge and experience during the development of oneM2MTester, we found that the 
Titan core version (i.e. Titan TTCN-3 compiler version) impacts the compiling result of oneM2MTester.
Because oneM2MTester project incorporates oneM2M abstract test suite which defines a bunch of oneM2M
specific data types, whose complexity impacts the TTCN-3 compiler's performance.

Due to this reason, we provide a specific version of Eclipse Titan core in the repository, which has been verified 
to work well with oneM2MTester (v2.0). Before running oneM2MTester, please make sure you are using the latest oneM2MTester
code in the GitHub, and also the Titan Core version you are using matches with the one offered to you in the GitHub.




