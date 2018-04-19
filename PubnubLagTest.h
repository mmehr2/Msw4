#pragma once

#include <string>
#include <ctime>

class ServerLagTimeTestClient {
   static time_t TokenAsHiresTime( const std::string& token );
   static std::string HiresTimeAsToken( time_t );
   static double CalcTimeStats( const std::string& svr1, const std::string& svr2, const std::string& loc1, const std::string& loc2 );
public:
   // individual data points (17-digit time token strings 
   std::string serverTokenK1;
   std::string serverTokenK2;
   std::string serverTokenK3;
   std::string primaryLocalTokenPL1;
   std::string primaryLocalTokenPL2;
   std::string secondaryLocalTokenSL1;
   std::string secondaryLocalTokenSL2;

   // calculate the stats from these data points
   double PrimaryLagTimeMsec() const;
   double SecondaryLagTimeMsec() const;
   bool IsReportable() const;
   void StartTest();
};
