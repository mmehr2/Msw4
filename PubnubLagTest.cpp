#include "stdafx.h"
#include "PubnubLagTest.h"

/*static*/ time_t ServerLagTimeTestClient::TokenAsHiresTime( const std::string& token )
{
   return _atoi64(token.c_str());
}

/*static*/ std::string ServerLagTimeTestClient::HiresTimeAsToken( time_t tstamp )
{
   static char buffer[32];
   ctime_s(buffer, 32, &tstamp);
   return buffer;
}

/*static*/ double ServerLagTimeTestClient::CalcTimeStats( 
   const std::string& svr1, 
   const std::string& svr2, 
   const std::string& loc1, 
   const std::string& loc2 )
{
   double result = 0.0;
   time_t svrt1 = TokenAsHiresTime(svr1);
   time_t svrt2 = TokenAsHiresTime(svr2);
   time_t loct1 = TokenAsHiresTime(loc1);
   time_t loct2 = TokenAsHiresTime(loc2);
   double dsvr = difftime(svrt2, svrt1); // should be smaller
   double dloc = difftime(loct2, loct1); // should be larger
   result = abs(dloc - dsvr);
   result /= 1e4; // convert from hires 100ns counts to msec
   return result;
}

double ServerLagTimeTestClient::PrimaryLagTimeMsec() const
{
   // implements equation involving K1, K2, PL1, PL2
   double result = CalcTimeStats( this->serverTokenK1, this->serverTokenK2,
      this->primaryLocalTokenPL1, this->primaryLocalTokenPL2);
   return result;
}

double ServerLagTimeTestClient::SecondaryLagTimeMsec() const
{
   // implements equation involving K2, K3, SL1, SL2
   double result = CalcTimeStats( this->serverTokenK2, this->serverTokenK3,
      this->secondaryLocalTokenSL1, this->secondaryLocalTokenSL2);
   return result;
}

bool ServerLagTimeTestClient::IsReportable() const
{
   if (this->primaryLocalTokenPL1.empty())   return false;
   if (this->primaryLocalTokenPL2.empty())   return false;
   if (this->secondaryLocalTokenSL1.empty())   return false;
   if (this->secondaryLocalTokenSL2.empty())   return false;
   if (this->serverTokenK1.empty())   return false;
   if (this->serverTokenK2.empty())   return false;
   if (this->serverTokenK3.empty())   return false;
   return true;
}

void ServerLagTimeTestClient::StartTest()
{
   this->primaryLocalTokenPL1.clear();
   this->primaryLocalTokenPL2.clear();
   this->secondaryLocalTokenSL1.clear();
   this->secondaryLocalTokenSL2.clear();
   this->serverTokenK1.clear();
   this->serverTokenK2.clear();
   this->serverTokenK3.clear();
}
