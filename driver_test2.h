/**
 * driver_test2.h
 * Purpose: handle four TNC driver behaviors
 * @author Sijie Chen
 *
 * @version 3.0 12/19/2017
 */

#ifndef DRIVER_H
#define DRIVER_H

#include <iostream>            // cout
#include <math.h>              // pow
#include <limits.h>            // INT_MIN
#include <utility>             // pair
#include <string>              // string
#include <map>                 // map
#include <unordered_map>       // unordered_map
#include <vector>              // vector

#define isPunishRejectTimes 0
#define punishRejectTimes 2
#define useSurgePrice 0
#define doRelocateChoice 0
#define doStopChoice 0
#define doPlatformChoice 1

// Constant for relocation choice
#define alphaStay 0.0015
#define alphaAirport -1
#define alphaHome 1.5
#define alphaJoint -1

#define betaTravelTime -2
#define betaCongestionL1 -3
#define betaCongestionL2 -2
#define betaCongestionL3 -1

#define smallNumber 0.000000001

// Constant for stopping chocie
#define earningPerMile 0.5
#define workingTimePara 0.0079
#define earningPara 0.261

// Constant for platform choice
// Delayed Q-algorithm assumed value
#define S 240 // 5 zones * 2 platforms * 24 hours
#define A 5
#define gamma 0.5
#define epsilon 0.5
#define zeta 0.5

using namespace std;

/* Present driver current states
 * Rewrite operator== and hash function to use State as key in hash table
 */
struct State {
  int zone;
  int time;
  string platform;
  string action;
  
  bool operator==(const State &other) const {
    return (zone == other.zone
            && time == other.time
            && platform == other.platform
            && action == other.action);
  }
};
namespace std {
  template <>
  struct hash<State> {
    size_t operator()(const State& k) const {
      size_t res = 17;
      res = res * 31 + hash<int>()( k.zone );
      res = res * 31 + hash<int>()( k.time );
      res = res * 31 + hash<string>() ( k.platform );
      res = res * 31 + hash<string>() ( k.action );
      
      return res;
    }
  };
}

// Driver input data
struct Person {
  int driverId, startTime;
  string startZone, startPlatform;
};
// Request input data
struct Param {
  string origin, destination;
  double rating, requestTime, surgePrice, accessTime, travelTime;
  string platform;
  bool isPool;
  
  // Required data by driver to make a decision after finishing a trip
  double travel_time_downtown;
  double travel_time_airport;
  double travel_time_home;
};

class Driver {
public:
  /**
   * Constrcutor
   * @param struct Person including driver information
   * @return private data memebers would be initilized
   */
  Driver(Person people){
    this->driverId = people.driverId;
    this->startZone = people.startZone;
    this->currentZone = people.startZone;
    this->startTime = people.startTime;
    this->nextAvailableTime = people.startTime;
    if (doPlatformChoice) this->currentPlatform = people.startPlatform;
    
    init_beta_relocation_choice();
    init_actions();
    init_states_and_rewards();
  }
  
  /**
   * Response to a request, need to update driver info if accept the request
   * @param struct Param including request information
   */
  bool isAccept ( Param params ) {
    double ans = -1 - 0.5 * (params.accessTime / 10) - 0.4 * params.isPool +
    2 * params.surgePrice * useSurgePrice + 0.5 * params.rating - 2 * rejInRow * isPunishRejectTimes;
    if ( ans > 0 || (isPunishRejectTimes && (rejInRow >= punishRejectTimes))) {
      rejInRow = 0; acSum++; assignSum++;
      
      this->currentZone = params.destination;
      if (params.isPool == 0) this->rideType = 1;
      else if (params.isPool == 1) this->rideType = 2;
      this->nextAvailableTime = params.requestTime + params.accessTime + params.travelTime;
      
      // Different price structure
      //this->earnings = params.travelTime * (earningPerMile - 0.2);
      this->earnings = params.travelTime * earningPerMile * 0.2 + 4;

    } else  {
      rejInRow++; rejSum++; assignSum++;
      this->rideType = 0;
      return false;
    }
    
    return true;
  }
  
  /**
   * After make a response to a request, the driver should do other choices
   */
  bool otherInfoUpdate(Param params) {
    // Do stop choice first
    if (doStopChoice) {
      if (stopChoice()) return true;
      
    }
    if (doRelocateChoice) {
      relocateChoice(params);
    }
    if (doPlatformChoice) {
      platformChoice(params);
      
    }
    return true;
  }
  
  /**
   * Getter
   */
  int getDriverId() { return this->driverId; }
  string getStartZone() { return this->startZone; }
  string getCurrentZone() { return this->currentZone; }
  int getNextAvaliableTime() { return this->nextAvailableTime; }
  int getAcSum() { return this->acSum; }
  int getRejSum() { return this->rejSum; }
  int getAssignSum() { return this->assignSum; }
  string getCurrentPlatform() { return this->currentPlatform; }
  int getRideType() { return this->rideType; }
  bool getStatus() { return this->status; }
  int getRelocateCount() { return this->relocateCount; }
  int getStopCount() { return this->stopCount; }
  
  /**
   * Printer, to print useful final report
   * assignSum, rejSum, acSum
   * relocate times, switch platform times, and stop time if applicable
   */
  void print() {
    cout << "*** Driver ID ***" << endl;
    cout << getDriverId() << endl;
    cout << "Total assignment: " << getAssignSum() << endl;
    cout << "Total accept: " << getAcSum() << endl;
    cout << "Total reject: " << getRejSum() << endl;
    
    if (doRelocateChoice) {
      cout << "Total relocation counts: " << getRelocateCount() << endl;
    }
    if (doPlatformChoice) {}
    if (doStopChoice) {}
  }

private:
  // Fix
  int driverId;
  string startZone;   // home
  int startTime;
  
  // Need to update
  string currentZone;
  int nextAvailableTime;
  
  int rejInRow = 0;
  int acSum = 0;
  int rejSum = 0;
  int assignSum = 0;
  
  string currentPlatform = "both"; // both; uber; lyft
  int rideType = 0; //  0, no ride; 1, shared; 2, solo
  
  bool status = true; // in or out of system
  
  // Related to relocation choice
  int relocateCount = 0;
  
  // Related to stop choice
  int stopCount = 0;
  
  // utility model assumed value, beta
  unordered_map<string, double> beta_direction_choice;
  void init_beta_relocation_choice() {
    
    beta_direction_choice["air0"] = 0.1;
    beta_direction_choice["air1"] = 0.1;
    beta_direction_choice["air2"] = 0.1;
    beta_direction_choice["air3"] = 0.1;
    beta_direction_choice["air4"] = 0.1;
    beta_direction_choice["air5"] = 0.1;
    beta_direction_choice["air6"] = 1;
    beta_direction_choice["air7"] = 1;
    beta_direction_choice["air8"] = 1;
    beta_direction_choice["air9"] = 1;
    beta_direction_choice["air10"] = 1;
    beta_direction_choice["air11"] = 1;
    beta_direction_choice["air12"] = 2;
    beta_direction_choice["air13"] = 2;
    beta_direction_choice["air14"] = 2;
    beta_direction_choice["air15"] = 2;
    beta_direction_choice["air16"] = 2;
    beta_direction_choice["air17"] = 2;
    beta_direction_choice["air18"] = 1.5;
    beta_direction_choice["air19"] = 1.5;
    beta_direction_choice["air20"] = 1.5;
    beta_direction_choice["air21"] = 1.5;
    beta_direction_choice["air22"] = 1.5;
    beta_direction_choice["air23"] = 1.5;
    
    beta_direction_choice["home0"] = 0.1;
    beta_direction_choice["home1"] = 0.1;
    beta_direction_choice["home2"] = 0.1;
    beta_direction_choice["home3"] = 0.1;
    beta_direction_choice["home4"] = 0.1;
    beta_direction_choice["home5"] = 0.1;
    beta_direction_choice["home6"] = 0.1;
    beta_direction_choice["home7"] = 0.1;
    beta_direction_choice["home8"] = -2;
    beta_direction_choice["home9"] = -2;
    beta_direction_choice["home10"] = -2;
    beta_direction_choice["home11"] = -2;
    beta_direction_choice["home12"] = -2;
    beta_direction_choice["home13"] = -2;
    beta_direction_choice["home14"] = -2;
    beta_direction_choice["home15"] = -2;
    beta_direction_choice["home16"] = -2;
    beta_direction_choice["home17"] = 2;
    beta_direction_choice["home18"] = 2;
    beta_direction_choice["home19"] = 2;
    beta_direction_choice["home20"] = 2;
    beta_direction_choice["home21"] = 2;
    beta_direction_choice["home22"] = 2;
    beta_direction_choice["home23"] = 2;
    
  }
  
  /**
   * Stopping chocie
   * @return boolean, true if the driver wants to stop; otherwise, false
   */
  double earnings = 0; // accumulated earnings
  bool stopChoice() {
    double ans;
    int working_time = nextAvailableTime - startTime;
    ans = 0.0079 * working_time - 0.261 * earnings - 1;
    //cout << "ans: " << ans << endl;
    if ( ans > 0 ) {
      // Set nextAvailableTime as 60*24+1
      //this->nextAvailableTime = driverLogOut;
      this->status = false;
      stopCount++;
      return true;
    }
    
    return false;
  }
  
  /**
   * Relocation choice
   */
  bool relocateChoice(Param params) {
    double Vs = alphaStay;
    double Vdt = betaTravelTime * params.travel_time_downtown/10.0;
    double Vair = alphaAirport
    + betaTravelTime * params.travel_time_airport/10.0
    + beta_direction_choice["air"+to_string(nextAvailableTime/60)];
    double Vh = alphaHome
    + betaTravelTime * params.travel_time_home/10.0
    + beta_direction_choice["home" + to_string(nextAvailableTime/60)];
    double Vrs = alphaJoint;
    
    map<string, double> prob;
    double exp_sum = exp(Vdt) + exp(Vair) + exp(Vh);
    
    prob["stay"] = exp(Vs + Vrs) * exp(Vs) / 100.0;
    prob["downtown"] = exp(Vdt + Vrs) * exp(Vdt) / exp_sum;
    prob["airport"] = exp(Vair + Vrs) * exp(Vair) / exp_sum;
    prob["home"] = exp(Vh + Vrs) * exp(Vh) / exp_sum;
    
    double max = smallNumber;
    string ret;
    for ( auto it = prob.begin(); it != prob.end(); ++it ) {
      if ( max < it->second ) {
        max = it->second;
        ret = it->first;
      }
    }
    
    if ( ret == "stay" ) ret = currentZone;
    else if ( ret == "downtown" ) ret = "10";
    else if ( ret == "airport" ) ret = "3";
    else if ( ret == "home" ) ret = startZone;
    
    if (currentZone == ret) return false;
    else  {
      currentZone = ret;
      this->relocateCount++;
      return true;
    }
  }
 
  /**
   * Paltform chocie
   */
  bool platformChoice(Param params) {
    State s;
    s.zone = 1;
    s.platform = params.platform;
    //s.time = currentTime / 60;
    s.time = 0;
    // Other reuiqred value
    double kappa = 1.0 / ((1.0-gamma)* epsilon);
    double m = log(3*S*A* (1+S*A*kappa) / zeta) / (2 * pow(epsilon, 2.0)* pow(1-gamma, 2.0));
    
    double max = -0.5;
    string act;
    for ( int i = 0; i < 5; i++ ) {
      s.action = actions[i];
      if ( max < states[s].Q ) {
        max = states[s].Q;
        act = actions[i];
      }
    }
    
    State s_new;
    if ( act == "loggin_both" ) {
      s_new.platform = "both";
      currentPlatform = "both";
    } else if ( act == "change_to_lyft" ) {
      s_new.platform = "lyft";
      currentPlatform = "lyft";
    } else {
      s_new.platform = "uber";
      currentPlatform = "uber";
    }
    s_new.zone = s.zone;
    s_new.time = s.time;
    
    double r = rewards[s_new];
    if ( states[s].LEARN ) {
      states[s].U += (r + gamma);
      states[s].l++;
      if (states[s].l == m) {
        if ((states[s].Q - states[s].U / m )
            >= 2 * epsilon ) {
          states[s].Q =
          states[s].U / m + epsilon;
          t_top = this->nextAvailableTime;
        } else if (states[s].t >= t_top) {
          states[s].LEARN = false;
        }
        //states[make_pair(s,act)].t = this->nextAvailableTime;
        states[s].t = this->nextAvailableTime;
        states[s].U = 0;
        states[s].l = 0;
      } else if (states[s].t < t_top)
        states[s].LEARN = true;
      cout << "YES" << endl;
    }
    //cout << "NO" << endl;
    return true;
    
  }
  
  // action choice set
  vector<string> actions;
  void init_actions() {
    actions.push_back("keep_in_uber");
    actions.push_back("keep_in_lyft");
    actions.push_back("change_to_uber");
    actions.push_back("change_to_lyft");
    actions.push_back("loggin_both");
  }
  int t_top;
  
  struct Value {
    float Q;
    float U;
    int l;
    int t;
    bool LEARN;
  };
  // Connect state and value
  unordered_map<State, Value> states;
  unordered_map<State, float> rewards;
  void init_states_and_rewards() {
    Value value;
    value.Q = 1 / (1-gamma);
    value.U = 0;
    value.l = 0;
    value.t = 0;
    value.LEARN = true;
    
    State state0;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "uber";
    state0.action = actions[0];
    states[state0] = value;
    rewards[state0] = 0.01;
    
    State state1;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "uber";
    state0.action = actions[1];
    states[state1] = value;
    rewards[state1] = 0.01;
    
    State state2;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "uber";
    state0.action = actions[2];
    states[state2] = value;
    rewards[state2] = 0.01;
    
    State state3;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "uber";
    state0.action = actions[3];
    states[state3] = value;
    rewards[state3] = 0.01;
    
    State state4;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "uber";
    state0.action = actions[4];
    states[state4] = value;
    rewards[state4] = 0.01;
    
    State state5;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "lyft";
    state0.action = actions[0];
    states[state5] = value;
    rewards[state5] = 0.01;
    
    State state6;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "lyft";
    state0.action = actions[1];
    states[state6] = value;
    rewards[state6] = 0.01;
    
    State state7;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "lyft";
    state0.action = actions[2];
    states[state7] = value;
    rewards[state7] = 0.01;
    
    State state8;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "lyft";
    state0.action = actions[3];
    states[state8] = value;
    rewards[state8] = 0.01;
    
    State state9;
    state0.zone = 1;
    state0.time = 0; // 00:00 - 01:00 am
    state0.platform = "lyft";
    state0.action = actions[4];
    states[state9] = value;
    rewards[state9] = 0.01;

  }
};
#endif
