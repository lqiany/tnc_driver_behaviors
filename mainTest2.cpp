/**
 * mainTest2.cpp
 * Purpose: test file
 *
 * @author Sijie Chen
 * @version 1.0 11/20/2017
 */

#include "center.h"
#include <fstream>
#include <cfloat>

#define requestNumber 1000

int main() {
  for ( int driverNumber = 1; driverNumber < 851; driverNumber++ ) {
    // Initilize Center object
    ifstream TT("Traveltime2.txt");
    ifstream driver("drivers.txt");
    //Center center(TT, driver);
    
    // Get next request
    ifstream infile("requests.txt");
    string origin, destination;
    int platform;
    double rating, reqTime, sp;
    bool isPool;

  
    //cout << "j: " << j << endl;
    Center center(TT, driver, driverNumber);
    for ( int i = 0; i < requestNumber; i++ ) {
      
      infile >> origin >> destination >> rating >> reqTime >> platform >> isPool >> sp;
      
      Param params;
      params.origin = origin;
      params.destination = destination;
      params.rating = rating;
      params.requestTime = reqTime;
      
      if (platform == 1 || platform == 2) params.platform = "uber";
      else params.platform = "lyft";
      
      params.isPool = isPool;
      params.surgePrice = sp;
      
      // Assign request
      center.assignRequest(params, driverNumber);
      
    }
    
    // Get final report
    center.print();
  }

  return 0;
}
