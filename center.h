/**
 * center.h
 * Purpose: assign a request to a driver. This class store network information,
 *    like travel time. And all drivers' information. And print out final
 *    simulation report.
 *
 * @author Sijie Chen
 * @version 2.0 12/19/2017
 */

#ifndef center_h
#define center_h

#include "driver_test2.h"
#include <fstream>
#define largeNumber 10000

class Center {
public:
  /**
   * Store travel time and drivers input
   */
  Center (ifstream & TT, ifstream & driver, int driverNumber) {

    string key;
    double value;
    int n;
    TT >> n;
    for ( int i = 0; i < n; i++ ) {
      TT >> key >> value;
      TravelTime[key] = value;
    }
    
    int driverId;
    string startZone;
    int startTime, startPlatform;
    for ( int i = 0; i < driverNumber; i++ ) {
      driver >> driverId >> startZone >> startTime >> startPlatform;
      
      Person person;
      person.driverId = driverId;
      person.startZone = startZone;
      person.startTime = startTime;
      if (startPlatform == 0)
        person.startPlatform = "both";
      else if (startPlatform == 1)
        person.startPlatform = "uber";
      else
        person.startPlatform = "lyft";
      
      Driver driverAgent(person);
      drivers.push_back(driverAgent);
    }
  }
  
  /**
   * API to assign a request.
   * Should check if the driver accept the reqeust or not.
   * Should update the driver information
   * @param origin, origin of request to calculate access time
   * @param reqTime, to check driver avaliable
   * @param Param, the request information
   * @return boolean, true if a request can be servered
   */
  bool assignRequest ( Param params, int driverNumber ) {
    pair<int, double> nextDriver = this->findDriver( params, 0 );
    if ( nextDriver.first == 0 ) {
      // no driver found
      this->failureCount++;
      return false;
    }
    
    // check driver's response to the request
    params.accessTime = nextDriver.second;
    while ( !drivers[nextDriver.first - 1].isAccept( params ) &&
           nextDriver.first <= driverNumber ) {
      nextDriver = this->findDriver( params, nextDriver.first );
      
      if ( nextDriver.first == 0 ) {
        // no driver found
        this->failureCount++;
        //cout << "*** Rejected Request ***" << endl;
        //cout << "Orign: " << params.origin << endl;
        //cout << "Destination: " << params.destination << endl;
        //cout << params.rating << endl;
        return false;
      }
    }
    if ( nextDriver.first > drivers.size() ) {
      this->failureCount++;
      //cout << "*** Rejected Request ***" << endl;
      //cout << "Orign: " << params.origin << endl;
      //cout << "Destination: " << params.destination << endl;
      //cout << params.rating << endl;
      return false;
    }
    // Find other travel time for relocation
    params.travel_time_downtown = TravelTime[params.destination + "-" +"10"];
    params.travel_time_airport = TravelTime[params.destination + "-" + "3"];
    params.travel_time_home = TravelTime[params.destination + "-"
    + drivers[nextDriver.first - 1].getStartZone()];
    
    drivers[nextDriver.first - 1].otherInfoUpdate(params);
    
    return true;
  }
  
  /**
   * Print useful result
   * Summary value
   * Each driver information
   */
  void print() {
    //cout << "********** Simulation Final Report **********" << endl;

    /*cout << "***** Each Driver Report *****" << endl;*/
    //int switchSum = 0;
    //int stopSum = 0;
    /*for ( int i = 0; i < drivers.size(); i++ ) {
      drivers[i].print();
      //switchSum += drivers[i].getRelocateCount();
      //stopSum += drivers[i].getStopCount();
    }*/
    //cout << switchSum << endl;
    //cout << stopSum << endl;
    //cout << "***** Total Unsuccessful Request *****" << endl;
    cout << this->getFailureCount() << endl;
  }
  
  /**
   * Getter
   */
  int getFailureCount() { return this->failureCount; }
  int getAssignmentCount() { return this->assignmentCount; }
private:
  unordered_map<string, double> TravelTime; // key #node-#node
  vector<Driver> drivers; // all in system drivers
  int failureCount = 0;
  int assignmentCount = 0;
  /**
   * Find driver to assign a request
   * @param origin, origin of request to calculate access time
   * @param reqTime, to check driver avaliable
   * @return pair <driverId, accessTime>
   */
  pair<int, double> findDriver ( Param params, int lastId ) {
    // Loop through drivers vector and keep the minimal access time one
    double minAccessTime = largeNumber, curTime = largeNumber;
    int retId = 0;
    for ( int i = lastId; i < drivers.size(); i++ ) {
      // Check driver avaliability
      if ( !drivers[i].getStatus() ) continue;
      //cout <<drivers[i].getCurrentPlatform()<< endl;
      if ( drivers[i].getCurrentPlatform() != "both" &&
          drivers[i].getCurrentPlatform() != params.platform ) continue;
      if ( drivers[i].getNextAvaliableTime() > params.requestTime &&
          drivers[i].getRideType() == 2 )
        continue;
      //if ( drivers[i].getNextAvaliableTime() == driverLogOut ) continue;
      // make key for lookup
      string key = params.origin + "-" + drivers[i].getCurrentZone();
      curTime = TravelTime[key];
      if ( curTime < minAccessTime ) {
        minAccessTime = curTime;
        retId = i + 1; // driverId starts from 1
      }
    }
    pair <int, double> ret(retId, minAccessTime);
    return ret;
  }
};

#endif /* center_h */
