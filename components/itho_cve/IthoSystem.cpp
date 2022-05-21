#include "SystemConfig.h"
#include "IthoSystem.h"
#include "Hardware.h"
#include "i2c_esp32.h"

namespace esphome {
namespace itho_cve {

uint8_t ithoDeviceID = 0;
uint8_t itho_fwversion = 0;
uint8_t id0 = 0;
uint8_t id1 = 0;
uint8_t id2 = 0;
const struct ithoDeviceType* ithoDeviceptr = nullptr;
int16_t ithoSettingsLength = 0;
int16_t ithoStatusLabelLength = 0;
std::vector<ithoDeviceStatus> ithoStatus;
std::vector<ithoDeviceMeasurements> ithoMeasurements;
std::vector<ithoDeviceMeasurements> ithoInternalMeasurements;
struct lastCommand lastCmd;
ithoSettings * ithoSettingsArray = nullptr;

bool get2410 = false;
bool set2410 = false;
uint8_t index2410 = 0;
int32_t value2410 = 0;
int32_t * resultPtr2410 = nullptr;
bool i2c_result_updateweb = false;

bool itho_internal_hum_temp = false;
double ithoHum = 0;
double ithoTemp = 0;

Ticker getSettingsHack;
SemaphoreHandle_t mutexI2Ctask;

struct ithoLabels {
  const char* labelFull;
  const char* labelNormalized;
};

static const std::map<uint8_t, const char*> fanInfo = {
  {0x00, "off"},
  {0X01, "low"},
  {0X02, "medium"},
  {0X03, "high"},
  {0X04, "speed 4"},
  {0X05, "speed 5"},
  {0X06, "speed 6"},
  {0X07, "speed 7"},
  {0X08, "speed 8"},
  {0X09, "speed 9"},
  {0X0A, "speed 10"},
  {0X0B, "timer 1"},
  {0X0C, "timer 2"},
  {0X0D, "timer 3"},
  {0X0E, "timer 4"},
  {0X0F, "timer 5"},
  {0X10, "timer 6"},
  {0X11, "timer 7"},
  {0X12, "timer 8"},
  {0X13, "timer 9"},
  {0X14, "timer 10"},
  {0X15, "away"},
  {0X16, "absolute minimum"},
  {0X17, "absolute maximum"},
  {0x18, "auto"},
  {0xFF, "unknown"}
};

const std::map<uint8_t, const char*> fanSensorErrors = {
  {0xEF, "not available"},
  {0XF0, "shorted sensor"},
  {0XF1, "open sensor"},
  {0XF2, "not available error"},
  {0XF3, "out of range high"},
  {0XF4, "out of range low"},
  {0XF5, "not reliable"},
  {0XF6, "reserved error"},
  {0XF7, "reserved error"},
  {0XF8, "reserved error"},
  {0XF9, "reserved error"},
  {0XFA, "reserved error"},
  {0XFB, "reserved error"},
  {0XFC, "reserved error"},
  {0XFD, "reserved error"},
  {0XFE, "reserved error"},
  {0XFF, "unknown error"}
};

const std::map<uint8_t, const char*> fanSensorErrors2 = {
  {0x7F, "not available"},
  {0xEF, "not available"},
  {0X80, "shorted sensor"},
  {0X81, "open sensor"},
  {0X82, "not available error"},
  {0X83, "out of range high"},
  {0X84, "out of range low"},
  {0X85, "not reliable"},
  {0XFF, "unknown error"}
};

const std::map<uint8_t, const char*> fanHeatErrors = {
  {0xEF, "not available"},
  {0XF0, "open actuato"},
  {0XF1, "shorted actuato"},
  {0XF2, "not available error"},
  {0XFF, "unknown error"}
};

struct ithoLabels ithoLabelErrors[] {
	{   "Nullptr error" ,				  "nullptr-error"},
	{   "No label for device error" ,	  "no-label-for-device-error"},
	{   "No label for version error" ,	  "no-label-for-version-error"},
	{   "Label out of bound error" ,	  "label-out-of-bound-error"}
};

struct ithoLabels itho31D9Labels[] {
	{   "Speed status" ,	  "speed-status"},
	{   "Internal fault" ,	  "internal-fault"},
	{   "Frost cycle" ,		  "frost-cycle"},
	{   "Filter dirty" ,	  "filter-dirty"}
};

struct ithoLabels itho31DALabels[] {
	{   "AirQuality (%)" ,		  "airquality_perc"},
	{   "AirQbased on" ,		  "airq-based-on"},
	{   "CO2level (ppm)" ,		  "co2level_ppm"},
	{   "Indoorhumidity (%)" ,	  "indoor-humidity_perc"},
	{   "Outdoorhumidity (%)" ,	  "outdoor-humidity_perc"},
	{   "Exhausttemp (°C)" ,	  "exhausttemp_c"},
	{   "SupplyTemp (°C)" ,		  "supply-temp_c"},
	{   "IndoorTemp (°C)" ,		  "indoor-temp_c"},
	{   "OutdoorTemp (°C)" ,	  "outdoor-temp_c"},
	{   "SpeedCap" ,			  "speed-cap"},
	{   "BypassPos (%)" ,		  "bypass-pos_perc"},
	{   "FanInfo" ,				  "fan-info"},
	{   "ExhFanSpeed (%)" ,		  "exh-fan-speed_perc"},
	{   "SpeedInFan (%)" ,		  "in-fan-speed_perc"},
	{   "RemainingTime (min)" ,	  "remaining-time_min"},
	{   "PostHeat (%)" ,		  "post-heat_perc"},
	{   "PreHeat (%)" ,			  "pre-heat_perc"},
	{   "InFlow (l sec)" ,		  "in-flow-l_sec"},
	{   "ExhFlow (l sec)" ,		  "exh-flow-l_sec"}
};


const uint8_t itho_CVE14status1_4[] { 0,1,2,3,4,5,6,255};
const uint8_t itho_CVE14status5_6[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,255};

const uint16_t itho_CVE14setting1_4[]	{ 0,1,2,3,4,5,6,7,8,9,999};
const uint16_t itho_CVE14setting5[] 	{ 0,1,2,3,4,5,6,7,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,8,9,999};
const uint16_t itho_CVE14setting6[] 	{ 0,1,2,3,4,5,6,7,19,20,32,22,23,24,25,26,27,28,29,30,31,8,9,999};

const uint16_t *itho14SettingsMap[] =	{ nullptr, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting1_4, itho_CVE14setting5,  itho_CVE14setting6  };
const uint8_t  *itho14StatusMap[]   =	{ nullptr, itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status1_4,  itho_CVE14status5_6, itho_CVE14status5_6 };

const char* ithoCVE14SettingsLabels[] =  {
     "OEM number",
     "Print version",
     "Min setting potentiometer low (rpm)",
     "Max setting potentiometer low (rpm)",
     "Min setting potentiometer high (rpm)",
     "Max setting potentiometer high (rpm)",
     "RF enable",
     "I2C mode",
     "Manual operation",
     "Speed at manual operation (rpm)",
     "Fan constant Ca2",
     "Fan constant Ca1",
     "Fan constant Ca0",
     "Fan constant Cb2",
     "Fan constant Cb1",
     "Fan constant Cb0",
     "Fan constant Cc2",
     "Fan constant Cc1",
     "Fan constant Cc0",
     "Min. Ventilation (%)",
     "CO2 concentration absent (ppm)",
     "CO2 concentration present (ppm)",
     "Min. fan setpoint at valve low (%)",
     "Min. fan setpoint at valve high (%)",
     "Min. fan setpoint at damper high and PIR (%)",
     "CO2 concentration at 100% valve low (ppm)",
     "CO2 concentration at 100% valve high (ppm)",
     "CO2 concentration at 100% valve high and PIR (ppm)",
     "Max. speed change per minute (%)",
     "Expiration time PIR presence (Min)",
     "Cycle time (min)",
     "Measuring time (min)",
     "CO2 concentration present (ppm)"
};

const struct ithoLabels ithoCVE14StatusLabels[] {
	{  "Ventilation level (%)",	 	"ventilation-level_perc" },
	{  "Fan setpoint (rpm)",		"fan-setpoint_rpm" },
	{  "Fan speed (rpm)", 		 	"fan-speed_rpm" },
	{  "Error", 					"error" },
	{  "Selection", 				"selection" },
	{  "Startup counter", 		 	"startup-counter" },
	{  "Total operation (hours)",  	"total-operation_hours" },
	{  "CO2 value (ppm)", 		 	"co2-value_ppm" },
	{  "CO2 ventilation (%)", 	 	"co2ventilation_perc" },
	{  "Valve", 					"valve" },
	{  "Presence timer (sec)",		"presence-timer_sec" },
	{  "Condition", 				"condition" },
	{  "Cycle timer", 			 	"cycle-timer" },
	{  "Sample timer", 			 	"sample-timer" }
};


const uint8_t itho_CVE1Bstatus6[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,255};
const uint8_t itho_CVE1Bstatus7[] 		{ 0,1,2,3,4,5,6,7,8,9,10,11,12,255};
const uint8_t itho_CVE1Bstatus8[] 		{ 0,1,2,3,4,5,6,14,15,7,8,9,10,11,12,255};
const uint8_t itho_CVE1Bstatus9_11[] 	{ 0,1,2,3,4,5,6,14,15,7,8,9,10,11,12,16,17,18,19,20,21,255};
const uint8_t itho_CVE1Bstatus17[] 		{ 0,1,2,3,4,5,6,14,15,7,17,8,9,10,11,12,255};
const uint8_t itho_CVE1Bstatus18[] 		{ 0,1,2,3,4,5,6,14,15,7,17,8,9,11,12,255};
const uint8_t itho_CVE1Bstatus20_21[] 	{ 0,1,2,3,4,5,6,14,7,17,8,9,11,12,255};
const uint8_t itho_CVE1Bstatus22[] 		{ 0,1,2,3,4,5,6,14,7,17,8,9,11,12,22,23,24,25,26,27,28,29,30,31,32,255};
const uint8_t itho_CVE1Bstatus24_27[] 	{ 0,1,2,3,4,5,6,14,7,17,33,34,255};

const uint16_t itho_CVE1Bsetting6[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,999};
const uint16_t itho_CVE1Bsetting7[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,32,33,34,35,36,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting8[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,41,42,43,44,17,18,19,20,21,22,23,24,25,26,27,32,33,34,35,36,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting9[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,41,42,43,44,45,46,47,48,49,17,18,19,20,21,22,23,24,25,26,27,32,33,34,35,36,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting10[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,41,42,43,44,45,46,47,48,49,50,18,19,20,21,22,23,24,25,26,27,32,33,34,35,36,37,38,39,40,51,30,31,999};
const uint16_t itho_CVE1Bsetting11[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,41,42,43,44,45,46,47,48,49,50,51,18,19,20,21,22,23,24,25,26,27,32,33,34,35,36,52,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting17_20[] { 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,56,57,58,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting21[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,69,57,58,70,71,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,30,31,999};
const uint16_t itho_CVE1Bsetting22[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,69,57,58,70,71,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,30,31,999};
const uint16_t itho_CVE1Bsetting24[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,69,57,58,70,71,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,30,31,88,89,90,91,92,75,81,93,94,95,96,97,98,999};
const uint16_t itho_CVE1Bsetting25[] 	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,69,57,58,70,71,99,100,101,102,103,104,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,30,31,88,89,90,91,92,75,81,93,94,95,96,97,98,999};
const uint16_t itho_CVE1Bsetting26_27[]	{ 0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,42,43,44,45,46,47,48,49,50,53,54,55,69,57,58,70,71,105,106,107,108,109,110,111,112,113,114,115,116,117,118,59,60,61,62,63,64,65,32,33,34,66,67,68,37,38,39,40,30,31,88,89,90,91,92,75,81,93,94,95,96,97,98,999};


const uint16_t *itho1BSettingsMap[] =	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, itho_CVE1Bsetting6, itho_CVE1Bsetting7, itho_CVE1Bsetting8, itho_CVE1Bsetting9,   itho_CVE1Bsetting10,  itho_CVE1Bsetting11,  nullptr, nullptr, nullptr, nullptr, nullptr, itho_CVE1Bsetting17_20, itho_CVE1Bsetting17_20, nullptr, itho_CVE1Bsetting17_20, itho_CVE1Bsetting21, itho_CVE1Bsetting22, nullptr, itho_CVE1Bsetting24,   itho_CVE1Bsetting25,   itho_CVE1Bsetting26_27, itho_CVE1Bsetting26_27 };
const uint8_t  *itho1BStatusMap[]   =	{ nullptr, nullptr, nullptr, nullptr, nullptr, nullptr, itho_CVE1Bstatus6,  itho_CVE1Bstatus7,  itho_CVE1Bstatus8,  itho_CVE1Bstatus9_11, itho_CVE1Bstatus9_11, itho_CVE1Bstatus9_11, nullptr, nullptr, nullptr, nullptr, nullptr, itho_CVE1Bstatus17,     itho_CVE1Bstatus18,     nullptr, itho_CVE1Bstatus20_21, itho_CVE1Bstatus20_21, itho_CVE1Bstatus22, nullptr, itho_CVE1Bstatus24_27, itho_CVE1Bstatus24_27, itho_CVE1Bstatus24_27,  itho_CVE1Bstatus24_27  };

const char* ithoCVE1BSettingsLabels[] =  {
      "OEM number", 
      "Print version (1 is high performance)", 
      "Min setting of potentiometer low (rpm)", 
      "Max setting of potentiometer low (rpm)", 
      "Min setting of potentiometer high (rpm)", 
      "Max setting of potentiometer high (rpm)", 
      "RF enable", 
      "I2C mode", 
      "Fan constant Ca2", 
      "Fan constant Ca1", 
      "Fan constant Ca0", 
      "Fan constant Cb2", 
      "Fan constant Cb1", 
      "Fan constant Cb0", 
      "Fan constant Cc2", 
      "Fan constant Cc1", 
      "Fan constant Cc0", 
      "Minimum ventilation (%)", 
      "CO2 concentration absent (ppm)", 
      "CO2 concentration at present (ppm)", 
      "Min. Fan setpoint at valve low (%)", 
      "Min. Fan setpoint at valve high (%)", 
      "Min. Fan setpoint at valve high and PIR (%)", 
      "CO2 concentration at 100% valve low (ppm)", 
      "CO2 concentration at 100% valve high (ppm)", 
      "CO2 concentration at 100% valve high and PIR (ppm)", 
      "Max speed change per minute (%)", 
      "Expiration time PIR present (Min)", 
      "Period time (min)", 
      "Stabilization period (min)", 
      "Manual operation", 
      "Speed at manual operation (rpm)", 
      "CO2 period time (min)", 
      "CO2 change rising CO2 (ppm)", 
      "CO2 change decreasing CO2 (ppm)", 
      "CO2 detection limit normal situation (ppm)", 
      "Time duration of boost blocking (min)", 
      "Stop blocking time at low desire (%)", 
      "Max fast change. CO2 reg. Blocking time (%_min)", 
      "CO2 increase start of blocking time (ppm)", 
      "Max speed up during blocking time (%)", 
      "Speed at absence (rpm)", 
      "Block time auto. reset absence (min)", 
      "Max. time deferred absence (min)", 
      "Min. desire for deferred absence (%)", 
      "PIR level 1 (%)", 
      "PIR level 2 (%)", 
      "PIR level 1 wait time (sec)", 
      "PIR level 2 wait time (sec)", 
      "PIR level 1 running time (sec)", 
      "PIR level 2 running time (sec)", 
      "Min fan setpoint BaseFlow present", 
      "Maximum period rev. block (min)", 
      "Minimum ventilation level in Auto (%)", 
      "Max time in high (min)", 
      "Max time in low or middle (min)", 
      "Heatrae", 
      "CO2 value air quality moderate (ppm)", 
      "CO2 value air quality good (ppm)", 
      "CO2 value absent (ppm)", 
      "CO2 value present (ppm)", 
      "Min speed valve low AreaFlow (%)", 
      "Min speed valve high AreaFlow (%)", 
      "CO2 value vent. 100% valve low (ppm)", 
      "CO2 value vent. 100% valve high (ppm)", 
      "Max speed change CO2 control (%_min)", 
      "CO2 change constant CO2 (ppm)", 
      "Blocking time fan ramp up (min)", 
      "Blocking time maximum duration (min)", 
      "Position after maximum time high", 
      "Type of dwelling", 
      "Number of occupants", 
      "UseWiredRh", 
      "RhSensorInterval (0.1s)", 
      "StoreInterval (s)", 
      "Ventilation level it goes to when the RH control turns on (%)", 
      "RHmin (%)", 
      "RHmax (%)", 
      "RHstart (%)", 
      "RHstop (%)", 
      "RHIncrease (%)", 
      "Minimum time for fan to go to high in RH control (min)", 
      "RHdiff (%)", 
      "RHconstantDiff (%)", 
      "RHconstantTime (min)", 
      "RHlow (%)", 
      "RHselectOperationMode", 
      "RHsensitivityIncrease", 
      "Dew point at which the RV-IC heater should start (K)", 
      "Dew point at which the RV-IC heater should stop (K)", 
      "Minimum time the RV-IC heater is on (min)", 
      "Maximum time the RV-IC heater should be on (min)", 
      "RH sensor found-available", 
      "External RH sensor overrides internal yes-no", 
      "Sampling time of the RH and temperature measurement (sec)", 
      "Hysteresis RH for return to normal operation (%)", 
      "Rise in absolute humidity (X) at which it goes to high (mg_kg)", 
      "Hysteresis X before falling back to normal operation (mg_kg)", 
      "Maximum time it is allowed by the RH control to remain in high (min)", 
      "NightOnePersonOneFloor (%)", 
      "NightTwoPersonsOneFloor (%)", 
      "NightMultiPersonsOneFloor (%)", 
      "NightOnePersonMultiFloor (%)", 
      "NightTwoPersonsMultiFloor (%)", 
      "NightMultiPersonsMultiFloor (%)", 
      "NightOnePersonOneFloorOptima1 (%)", 
      "NightTwoPersonsOneFloorOptima1 (%)", 
      "NightMultiPersonsOneFloorOptima1 (%)", 
      "NightOnePersonMultiFloorOptima1 (%)", 
      "NightTwoPersonsMultiFloorOptima1 (%)", 
      "NightMultiPersonsMultiFloorOptima1 (%)", 
      "NightOnePersonOneFloorOptima2 (%)", 
      "NightTwoPersonsOneFloorOptima2 (%)", 
      "NightMultiPersonsOneFloorOptima2 (%)", 
      "NightOnePersonMultiFloorOptima2 (%)", 
      "NightTwoPersonsMultiFloorOptima2 (%)", 
      "NightMultiPersonsMultiFloorOptima2 (%)", 
      "AutoVentilationOneFloor (%)", 
      "AutoVentilationMultiFloor (%)"
};

const struct ithoLabels ithoCVE1BStatusLabels[] =  {
	{   "Ventilation setpoint (%)",  		  "ventilation-setpoint_perc" },
	{   "Fan setpoint (rpm)",  				  "fan-setpoint_rpm" },
	{   "Fan speed (rpm)",  				  "fan-speed_rpm" },
	{   "Error",  							  "error" },
	{   "Selection",  						  "selection" },
	{   "Startup counter",  				  "startup-counter" },
	{   "Total operation (hours)",  		  "total-operation_hours" },
	{   "Highest CO2 concentration (ppm)",    "highest-co2-concentration_ppm" },
	{   "Co2 velocity", 					  "co2-velocity" },
	{   "Valve",  							  "valve" },
	{   "Presence timer (sec)", 			  "presence-timer_sec" },
	{   "Current period",  					  "current-period" },
	{   "Period timer",  					  "period-timer" },
	{   "Sample timer",  					  "sample-timer" },
	{   "Absence (min)",  					  "absence_min" },
	{   "Deferred absence (min)",  			  "deferred-absence_min" },
	{   "Rfspeed absolute",  				  "rfspeed-absolute" },
	{   "Highest RH concentration (%)",  	  "highest-rh-concentration_perc" },
	{   "Next Rfspeed absolute",  			  "next-rfspeed-absolute" },
	{   "Next Rfspeed level",  				  "next-rfspeed-level" },
	{   "Speed timer",  					  "speed-timer" },
	{   "New level time",  					  "new-level-time" },
	{   "Sensor fault",  					  "sensor-fault" },
	{   "Internal humidity (%)",  			  "internal-humidity_perc" },
	{   "Internal temp (°C)",  				  "internal-temp_c" },
	{   "Selected mode",  					  "selected-mode" },
	{   "Enhanced bathroom", 				  "enhanced-bathroom" },
	{   "Counter stop (s)",  				  "counter-stop_s" },
	{   "Counter min-time (s)",  			  "counter-min-time_s" },
	{   "Ventilation request (%)",  		  "ventilation-request_perc" },
	{   "Measurement actual (%)",  			  "measurement-actual_perc" },
	{   "Measurement previous first (%)",  	  "measurement-previous-first_perc" },
	{   "Measurement previous second (%)",    "measurement-previous-second_perc" },
	{   "RelativeHumidity",  				  "relativehumidity" },
	{   "Temperature", 			 			  "temperature" }
};


//                                        { IthoUnknown,  IthoJoin,                           IthoLeave,                            IthoAway,                    IthoLow,                            IthoMedium,                           IthoHigh,                           IthoFull, IthoTimer1,                           IthoTimer2,                           IthoTimer3,                           IthoAuto,                           IthoAutoNight,                            IthoCook30,                       IthoCook60 }
const uint8_t *RFTCVE_Remote_Map[]      = { nullptr,      ithoMessageCVERFTJoinCommandBytes,  ithoMessageLeaveCommandBytes,         ithoMessageAwayCommandBytes, ithoMessageLowCommandBytes,         ithoMessageMediumCommandBytes,        ithoMessageHighCommandBytes,        nullptr,  ithoMessageTimer1CommandBytes,        ithoMessageTimer2CommandBytes,        ithoMessageTimer3CommandBytes,        nullptr,                            nullptr,                                  nullptr,                          nullptr };
const uint8_t *RFTAUTO_Remote_Map[]     = { nullptr,      ithoMessageAUTORFTJoinCommandBytes, ithoMessageAUTORFTLeaveCommandBytes,  nullptr,                     ithoMessageAUTORFTLowCommandBytes,  nullptr,                              ithoMessageAUTORFTHighCommandBytes, nullptr,  ithoMessageAUTORFTTimer1CommandBytes, ithoMessageAUTORFTTimer2CommandBytes, ithoMessageAUTORFTTimer3CommandBytes, ithoMessageAUTORFTAutoCommandBytes, ithoMessageAUTORFTAutoNightCommandBytes,  nullptr,                          nullptr };
const uint8_t *DEMANDFLOW_Remote_Map[]  = { nullptr,      ithoMessageDFJoinCommandBytes,      ithoMessageLeaveCommandBytes,         nullptr,                     ithoMessageDFLowCommandBytes,       nullptr,                              ithoMessageDFHighCommandBytes,      nullptr,  ithoMessageDFTimer1CommandBytes,      ithoMessageDFTimer2CommandBytes,      ithoMessageDFTimer3CommandBytes,      nullptr,                            nullptr,                                  ithoMessageDFCook30CommandBytes,  ithoMessageDFCook60CommandBytes };
const uint8_t *RFTRV_Remote_Map[]       = { nullptr,      ithoMessageRVJoinCommandBytes,      ithoMessageLeaveCommandBytes,         nullptr,                     ithoMessageLowCommandBytes,         ithoMessageRV_CO2MediumCommandBytes,  ithoMessageHighCommandBytes,        nullptr,  ithoMessageRV_CO2Timer1CommandBytes,  ithoMessageRV_CO2Timer2CommandBytes,  ithoMessageRV_CO2Timer3CommandBytes,  ithoMessageRV_CO2AutoCommandBytes,  ithoMessageRV_CO2AutoNightCommandBytes,   nullptr,                          nullptr };
const uint8_t *RFTCO2_Remote_Map[]      = { nullptr,      ithoMessageCO2JoinCommandBytes,     ithoMessageLeaveCommandBytes,         nullptr,                     ithoMessageLowCommandBytes,         ithoMessageRV_CO2MediumCommandBytes,  ithoMessageHighCommandBytes,        nullptr,  ithoMessageRV_CO2Timer1CommandBytes,  ithoMessageRV_CO2Timer2CommandBytes,  ithoMessageRV_CO2Timer3CommandBytes,  ithoMessageRV_CO2AutoCommandBytes,  ithoMessageRV_CO2AutoNightCommandBytes,   nullptr,                          nullptr };

struct ihtoRemoteCmdMap {
  RemoteTypes type;
  const uint8_t **cammandMapping;
};

const struct ihtoRemoteCmdMap ihtoRemoteCmdMapping[] {
  { RFTCVE,     RFTCVE_Remote_Map },
  { RFTAUTO,    RFTAUTO_Remote_Map },
  { DEMANDFLOW, DEMANDFLOW_Remote_Map },
  { RFTRV,      RFTRV_Remote_Map },
  { RFTCO2,     RFTCO2_Remote_Map }
};

const uint8_t* getRemoteCmd(const RemoteTypes type, const IthoCommand command) {

  const struct ihtoRemoteCmdMap* ihtoRemoteCmdMapPtr = ihtoRemoteCmdMapping;
  const struct ihtoRemoteCmdMap* ihtoRemoteCmdMapEndPtr = ihtoRemoteCmdMapping + sizeof(ihtoRemoteCmdMapping) / sizeof(ihtoRemoteCmdMapping[0]);
  while ( ihtoRemoteCmdMapPtr < ihtoRemoteCmdMapEndPtr ) {
    if (ihtoRemoteCmdMapPtr->type == type) {
      return *(ihtoRemoteCmdMapPtr->cammandMapping + command);
    }
    ihtoRemoteCmdMapPtr++;
  }
  return nullptr;
}


struct ithoDeviceType {
  uint8_t ID;
  const char *name;
  const uint16_t **settingsMapping;
  uint16_t versionsMapLen;
  const char **settingsDescriptions;
  const uint8_t **statusLabelMapping;
  uint8_t statusMapLen;
  const struct ithoLabels *settingsStatusLabels;
};


const struct ithoDeviceType ithoDevices[] {
  { 0x01, "Air curtain",        nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x03, "HRU ECO-fan",        ithoHRUecoFanSettingsMap, sizeof(ithoHRUecoFanSettingsMap) / sizeof(ithoHRUecoFanSettingsMap[0]), ithoHRUecoSettingsLabels, ithoHRUecoFanStatusMap, sizeof(ithoHRUecoFanStatusMap) / sizeof(ithoHRUecoFanStatusMap[0]), ithoHRUecoStatusLabels },
  { 0x08, "LoadBoiler",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x0A, "GGBB",               nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x0B, "DemandFlow",         ithoDemandFlowSettingsMap, sizeof(ithoDemandFlowSettingsMap) / sizeof(ithoDemandFlowSettingsMap[0]), ithoDemandFlowSettingsLabels, ithoDemandFlowStatusMap, sizeof(ithoDemandFlowStatusMap) / sizeof(ithoDemandFlowStatusMap[0]), ithoDemandFlowStatusLabels  },
  { 0x0C, "CO2 relay",          nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x0D, "Heatpump",           ithoWPUSettingsMap, sizeof(ithoWPUSettingsMap) / sizeof(ithoWPUSettingsMap[0]), ithoWPUSettingsLabels, ithoWPUStatusMap, sizeof(ithoWPUStatusMap) / sizeof(ithoWPUStatusMap[0]), ithoWPUStatusLabels },
  { 0x0E, "OLB Single",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x0F, "AutoTemp",           ithoAutoTempSettingsMap, sizeof(ithoAutoTempSettingsMap) / sizeof(ithoAutoTempSettingsMap[0]), ithoAutoTempSettingsLabels, ithoAutoTempStatusMap, sizeof(ithoAutoTempStatusMap) / sizeof(ithoAutoTempStatusMap[0]), ithoAutoTempStatusLabels },
  { 0x10, "OLB Double",         nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x11, "RF+",                nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x14, "CVE",                itho14SettingsMap, sizeof(itho14SettingsMap) / sizeof(itho14SettingsMap[0]), ithoCVE14SettingsLabels, itho14StatusMap, sizeof(itho14StatusMap) / sizeof(itho14StatusMap[0]), ithoCVE14StatusLabels  },
  { 0x15, "Extended",           nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x16, "Extended Plus",      nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x1A, "AreaFlow",           nullptr, 0, nullptr, nullptr, 0, nullptr },
  { 0x1B, "CVE-Silent",         itho1BSettingsMap, sizeof(itho1BSettingsMap) / sizeof(itho1BSettingsMap[0]), ithoCVE1BSettingsLabels, itho1BStatusMap, sizeof(itho1BStatusMap) / sizeof(itho1BStatusMap[0]), ithoCVE1BStatusLabels  },
  { 0x1C, "CVE-SilentExt",      nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x1D, "CVE-SilentExtPlus",  ithoHRU200SettingsMap, sizeof(ithoHRU200SettingsMap) / sizeof(ithoHRU200SettingsMap[0]), ithoHRU200SettingsLabels, ithoHRU200StatusMap, sizeof(ithoHRU200StatusMap) / sizeof(ithoHRU200StatusMap[0]), ithoHRU200StatusLabels },
  { 0x20, "RF_CO2",             nullptr, 0, nullptr, nullptr, 0, nullptr },
  //{ 0x2B, "HRU 350",            ithoHRU350SettingsMap, sizeof(ithoHRU350SettingsMap) / sizeof(ithoHRU350SettingsMap[0]), ithoHRU350SettingsLabels, ithoHRU350StatusMap, sizeof(ithoHRU350StatusMap) / sizeof(ithoHRU350StatusMap[0]), ithoHRU350StatusLabels  }
};

const char* getIthoType(const uint8_t deviceID) {
  static char ithoDeviceType[32] = "Unkown device type";

  const struct ithoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ithoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      strcpy(ithoDeviceType, ithoDevicesptr->name);
    }
    ithoDevicesptr++;
  }
  return ithoDeviceType;
}

int getSettingsLength(const uint8_t deviceID, const uint8_t version) {

  const struct ithoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ithoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      if (ithoDevicesptr->settingsMapping == nullptr) {
        return -2; //Settings not available for this device
      }
      if (version > (ithoDevicesptr->versionsMapLen - 1)) {
        return -3; //Settings not available for this version
      }

      for (int i = 0; i < 999; i++) {
        if (static_cast<int>( * (*(ithoDevicesptr->settingsMapping + version) + i) == 999 )) {
          //end of array
          if (ithoSettingsArray == nullptr) ithoSettingsArray = new ithoSettings[i];
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop) {
  getSetting(i, updateState, updateweb, loop, ithoDeviceptr, ithoDeviceID, itho_fwversion);
}

void getSetting(const uint8_t i, const bool updateState, const bool updateweb, const bool loop, const struct ithoDeviceType* settingsPtr, const uint8_t deviceID, const uint8_t version) {

  int settingsLen = getSettingsLength(deviceID, version);
  if (settingsLen < 0) {
    //logMessagejson("Settings not available for this device or its firmware version", WEBINTERFACE);
    return;
  }

  StaticJsonDocument<512> doc;
  JsonObject root = doc.to<JsonObject>();

  root["Index"] = i;
  root["Description"] = settingsPtr->settingsDescriptions[static_cast<int>( * (*(settingsPtr->settingsMapping + version) + i ))];

  if (!updateState && !updateweb) {
    root["update"] = false;
    root["loop"] = loop;
    root["Current"] = nullptr;
    root["Minimum"] = nullptr;
    root["Maximum"] = nullptr;
    //logMessagejson(root, ITHOSETTINGS);
  }
  else if (updateState && !updateweb) {
    index2410 = i;
    i2c_result_updateweb = false;
    resultPtr2410 = nullptr;
    get2410 = true;

    auto timeoutmillis = millis() + 1000;
    while (resultPtr2410 == nullptr && millis() < timeoutmillis) {
      //wait for result
    }
    root["update"] = true;
    root["loop"] = loop;
    if (resultPtr2410 != nullptr && ithoSettingsArray != nullptr) {
      if (ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
        root["Current"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<int8_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
        root["Current"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<int16_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_uint32) {
        root["Current"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 0));
        root["Minimum"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 1));
        root["Maximum"] = *(reinterpret_cast<uint32_t*>(resultPtr2410 + 2));
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float2) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 2.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 2.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 2.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float10) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 10.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 10.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 10.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float100) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 100.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 100.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 100.0;
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
        root["Current"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 0)) / 1000.0;
        root["Minimum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 1)) / 1000.0;
        root["Maximum"] = *(reinterpret_cast<int32_t*>(resultPtr2410 + 2)) / 1000.0;
      }
      else {
        root["Current"] = *(resultPtr2410 + 0);
        root["Minimum"] = *(resultPtr2410 + 1);
        root["Maximum"] = *(resultPtr2410 + 2);
      }
    }
    else {
      root["Current"] = nullptr;
      root["Minimum"] = nullptr;
      root["Maximum"] = nullptr;
    }
    //logMessagejson(root, ITHOSETTINGS);
  }
  else {
    index2410 = i;
    i2c_result_updateweb = updateweb;
    resultPtr2410 = nullptr;
    get2410 = true;
  }

}

int getStatusLabelLength(const uint8_t deviceID, const uint8_t version) {

  const struct ithoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ithoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      if (ithoDevicesptr->statusLabelMapping == nullptr) {
        return -2; //Labels not available for this device
      }
      if (version > (ithoDevicesptr->statusMapLen - 1)) {
        return -3; //Labels not available for this version
      }

      for (int i = 0; i < 255; i++) {
        if (static_cast<int>( * (*(ithoDevicesptr->statusLabelMapping + version) + i) == 255 )) {
          //end of array
          return i;
        }
      }
    }
    ithoDevicesptr++;
  }
  return -1;
}

const char* getStatusLabel(const uint8_t i, const struct ithoDeviceType* statusPtr, const uint8_t version) {

  if (statusPtr == nullptr) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[0].labelFull;
    }
    else {
      return ithoLabelErrors[0].labelNormalized;
    }
  }
  else if (ithoStatusLabelLength == -2) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[1].labelFull;
    }
    else {
      return ithoLabelErrors[1].labelNormalized;
    }
  }
  else if (ithoStatusLabelLength == -3) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[2].labelFull;
    }
    else {
      return ithoLabelErrors[2].labelNormalized;
    }
  }
  else if (!(i < ithoStatusLabelLength)) {
    if (systemConfig.api_normalize == 0) {
      return ithoLabelErrors[3].labelFull;
    }
    else {
      return ithoLabelErrors[3].labelNormalized;
    }
  }
  else {
    if (systemConfig.api_normalize == 0) {
      return (statusPtr->settingsStatusLabels[static_cast<int>( * (*(statusPtr->statusLabelMapping + version) + i) )].labelFull);
    }
    else {
      return (statusPtr->settingsStatusLabels[static_cast<int>( * (*(statusPtr->statusLabelMapping + version) + i) )].labelNormalized);
    }
  }

}

void updateSetting(const uint8_t i, const int32_t value, bool webupdate) {

  if (xSemaphoreTake(mutexI2Ctask, (TickType_t) 500 / portTICK_PERIOD_MS) == pdTRUE) {

    i2c_result_updateweb = webupdate;
    index2410 = i;
    value2410 = value;
    set2410 = true;
  }
}

const struct ithoDeviceType* getDevicePtr(const uint8_t deviceID) {

  const struct ithoDeviceType* ithoDevicesptr = ithoDevices;
  const struct ithoDeviceType* ithoDevicesendPtr = ithoDevices + sizeof(ithoDevices) / sizeof(ithoDevices[0]);
  while ( ithoDevicesptr < ithoDevicesendPtr ) {
    if (ithoDevicesptr->ID == deviceID) {
      return ithoDevicesptr;
    }
    ithoDevicesptr++;
  }
  return nullptr;
}



uint8_t checksum(const uint8_t* buf, size_t buflen) {
  uint8_t sum = 0;
  while (buflen--) {
    sum += *buf++;
  }
  return -sum;
}


void sendI2CPWMinit() {

  uint8_t command[] = { 0x82, 0xEF, 0xC0, 0x00, 0x01, 0x06, 0x00, 0x00, 0x09, 0x00, 0x09, 0x00, 0xB6 };

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

}

uint8_t cmdCounter = 0;

void sendRemoteCmd(const uint8_t remoteIndex, const IthoCommand command, IthoRemote &remotes) {
  
  //command structure: 
  // [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]<  opcode  ><len2><   len2 length command      >[chk2][cntr][chk ]
  // 0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x05, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
  if (remoteIndex > remotes.getMaxRemotes()) return;

  const RemoteTypes remoteType = remotes.getRemoteType(remoteIndex);
  if (remoteType == RemoteTypes::UNSETTYPE) return;
  
  //Get the corresponding command / remote type combination
  const uint8_t *remote_command = getRemoteCmd(remoteType, command);
  if (remote_command == nullptr) return;

  uint8_t i2c_command[64] = {};
  uint8_t i2c_command_len = 0;

  /*
     First build the i2c header / wrapper for the remote command

  */
  //                       [I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr]
  uint8_t i2c_header[] = { 0x82, 0x60, 0xC1, 0x01, 0x01, 0x09, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF };

  unsigned long curtime = millis();
  i2c_header[6] = (curtime >> 24) & 0xFF;
  i2c_header[7] = (curtime >> 16) & 0xFF;
  i2c_header[8] = (curtime >> 8) & 0xFF;
  i2c_header[9] = curtime & 0xFF;

  const int * id = remotes.getRemoteIDbyIndex(remoteIndex);
  i2c_header[11] = *id;
  i2c_header[12] = *(id + 1);
  i2c_header[13] = *(id + 2);

  i2c_header[14] = cmdCounter;
  cmdCounter++;

  for (; i2c_command_len < sizeof(i2c_header) / sizeof(i2c_header[0]); i2c_command_len++) {
    i2c_command[i2c_command_len] = i2c_header[i2c_command_len];
  }

  //determine command length
  const int command_len = remote_command[2];

  //copy to i2c_command
  for (int i = 0; i < 2 + command_len + 1; i++) {
    i2c_command[i2c_command_len] = remote_command[i];
    i2c_command_len++;
  }

  //if join or leave, add remote ID fields
  if (command == IthoJoin || command == IthoLeave) {
    //set command ID's
    if (command_len > 0x05) {
      //add 1st ID
      i2c_command[21] = *id;
      i2c_command[22] = *(id + 1);
      i2c_command[23] = *(id + 2);
    }
    if (command_len > 0x0B) {
      //add 2nd ID
      i2c_command[27] = *id;
      i2c_command[28] = *(id + 1);
      i2c_command[29] = *(id + 2);
    }
    if (command_len > 0x12) {
      //add 3rd ID
      i2c_command[33] = *id;
      i2c_command[34] = *(id + 1);
      i2c_command[35] = *(id + 2);
    }
    if (command_len > 0x17) {
      //add 4th ID
      i2c_command[39] = *id;
      i2c_command[40] = *(id + 1);
      i2c_command[41] = *(id + 2);
    }
    if (command_len > 0x1D) {
      //add 5th ID
      i2c_command[45] = *id;
      i2c_command[46] = *(id + 1);
      i2c_command[47] = *(id + 2);
    }
  }

  //if timer1-3 command use timer config of systemConfig.itho_timer1-3
  //example timer command: {0x22, 0xF3, 0x06, 0x00, 0x00, 0xFF, 0x00, 0x00, 0x00} 0xFF is timer value on pos 20 of i2c_command[]
  if (command == IthoTimer1) {
    i2c_command[20] = systemConfig.itho_timer1;
  }
  else if (command == IthoTimer2) {
    i2c_command[20] = systemConfig.itho_timer2;
  }
  else if (command == IthoTimer3) {
    i2c_command[20] = systemConfig.itho_timer3;
  }
    
  /*
     built the footer of the i2c wrapper
     chk2 = checksum of [fmt]+[remote ID]+[cntr]+[remote command]
     chk = checksum of the whole command

  */
  //                        [chk2][cntr][chk ]
  //uint8_t i2c_footer[] = { 0x00, 0x00, 0xFF };

  //calculate chk2 val
  uint8_t i2c_command_tmp[64] = {};
  for (int i = 10; i < i2c_command_len; i++) {
    i2c_command_tmp[(i - 10)] = i2c_command[i];
  }

  i2c_command[i2c_command_len] = checksum(i2c_command_tmp, i2c_command_len - 11);
  i2c_command_len++;

  //[cntr]
  i2c_command[i2c_command_len] = 0x00;
  i2c_command_len++;

  //set i2c_command length value
  i2c_command[5] = i2c_command_len - 6;
  
  //calculate chk val
  i2c_command[i2c_command_len] = checksum(i2c_command, i2c_command_len - 1);
  i2c_command_len++;

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(i2c_command, i2c_command_len);


}

void sendQueryDevicetype(bool updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x90, 0xE0, 0x04, 0x00, 0x8A };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};

  size_t len = i2c_slave_receive(i2cbuf);

  //if (len > 2) {
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;

      //jsonSysmessage("ithotype", i2cbuf2string(i2cbuf, len).c_str());
    }

    ithoDeviceID = i2cbuf[9];
    itho_fwversion = i2cbuf[11];
    ithoDeviceptr = getDevicePtr(ithoDeviceID);
    ithoSettingsLength = getSettingsLength(ithoDeviceID, itho_fwversion);
  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("ithotype", "failed");
    }
  }

}

void sendQueryStatusFormat(bool updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x00, 0x04, 0x00, 0xD6 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  uint8_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("ithostatusformat", i2cbuf2string(i2cbuf, len).c_str());
    }

    if (!ithoStatus.empty()) {
      ithoStatus.clear();
    }
    if (!(itho_fwversion > 0)) return;
    ithoStatusLabelLength = getStatusLabelLength(ithoDeviceID, itho_fwversion);
    const uint8_t endPos = i2cbuf[5];
    for (uint8_t i = 0; i < endPos; i++) {
      ithoStatus.push_back(ithoDeviceStatus());

      //      char fStringBuf[32];
      //      getSatusLabel(i, ithoDeviceptr, itho_fwversion, fStringBuf);



      ithoStatus.back().divider = 0;
      if ((i2cbuf[6 + i] & 0x07) == 0) { //integer value
        if ((i2cbuf[6 + i] & 0x80) == 0) { //unsigned value
          ithoStatus.back().type = ithoDeviceStatus::is_uint;
        }
        else {
          ithoStatus.back().type = ithoDeviceStatus::is_int;
        }
      }
      else {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().divider = quick_pow10((i2cbuf[6 + i] & 0x07));
      }
      if (((i2cbuf[6 + i] >> 3) & 0x07) == 0) {
        ithoStatus.back().length = 1;
      }
      else {
        ithoStatus.back().length = (i2cbuf[6 + i] >> 3) & 0x07;
      }

      //special cases
      if (i2cbuf[6 + i] == 0x0C || i2cbuf[6 + i] == 0x6C) {
        ithoStatus.back().type = ithoDeviceStatus::is_byte;
        ithoStatus.back().length = 1;
      }
      if (i2cbuf[6 + i] == 0x0F) {
        ithoStatus.back().type = ithoDeviceStatus::is_float;
        ithoStatus.back().length = 1;
        ithoStatus.back().divider = 2;
      }
      if (i2cbuf[6 + i] == 0x5B) {
        ithoStatus.back().type = ithoDeviceStatus::is_uint;
        ithoStatus.back().length = 2;
      }

    }

  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("ithostatusformat", "failed");
    }
  }
}

void sendQueryStatus(bool updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x01, 0x04, 0x00, 0xD5 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("ithostatus", i2cbuf2string(i2cbuf, len).c_str());
    }

    int statusPos = 6; //first byte with status info
    int labelPos = 0;
    if (!ithoStatus.empty()) {
      for (auto& ithoStat : ithoStatus) {
        ithoStat.name = getStatusLabel(labelPos, ithoDeviceptr, itho_fwversion);
        auto tempVal = 0;
        for (int i = ithoStat.length; i > 0; i--) {
          tempVal |= i2cbuf[statusPos + (ithoStat.length - i)] << ((i - 1) * 8);
        }

        if (ithoStat.type == ithoDeviceStatus::is_byte) {
          ithoStat.value.byteval = (byte)tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_uint) {
          ithoStat.value.uintval = tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_int) {
          if (ithoStat.length == 4) {
            tempVal = (int32_t)tempVal;
          }
          if (ithoStat.length == 2) {
            tempVal = (int16_t)tempVal;
          }
          if (ithoStat.length == 1) {
            tempVal = (int8_t)tempVal;
          }
          ithoStat.value.intval = tempVal;
        }
        if (ithoStat.type == ithoDeviceStatus::is_float) {
          ithoStat.value.floatval = static_cast<double>(tempVal) / ithoStat.divider;
        }

        statusPos += ithoStat.length;

        if (strcmp("Highest CO2 concentration (ppm)", ithoStat.name) == 0 || strcmp("highest-co2-concentration_ppm", ithoStat.name) == 0) {
          if (ithoStat.value.intval == 0x8200) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp("Highest RH concentration (%)", ithoStat.name) == 0 || strcmp("highest-rh-concentration_perc", ithoStat.name) == 0) {
          if (ithoStat.value.intval == 0xEF) {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp("RelativeHumidity", ithoStat.name) == 0 || strcmp("relativehumidity", ithoStat.name) == 0) {
          if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0) {
            if (systemConfig.syssht30 == 0) {
              ithoHum = ithoStat.value.floatval;
              itho_internal_hum_temp = true;
            }
          }
          else {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        if (strcmp("Temperature", ithoStat.name) == 0 || strcmp("temperature", ithoStat.name) == 0) {
          if (ithoStat.value.floatval > 1.0 && ithoStat.value.floatval < 100.0) {
            if (systemConfig.syssht30 == 0) {
              ithoTemp = ithoStat.value.floatval;
              itho_internal_hum_temp = true;
            }
          }
          else {
            ithoStat.type = ithoDeviceStatus::is_string;
            ithoStat.value.stringval = fanSensorErrors.begin()->second;
          }
        }
        labelPos++;
      }
    }
  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("ithostatus", "failed");
    }
  }

}

void sendQuery31DA(bool updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xDA, 0x04, 0x00, 0xEF };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);

  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho31DA", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataLength = i2cbuf[5];

    auto dataStart = 6;
    if (!ithoMeasurements.empty()) {
      ithoMeasurements.clear();
    }
    const int labelLen = 19;
    static const char* labels31DA[labelLen] {};
    for (int i = 0; i < labelLen; i++) {
      if (systemConfig.api_normalize == 0) {
        labels31DA[i] = itho31DALabels[i].labelFull;
      }
      else {
        labels31DA[i] = itho31DALabels[i].labelNormalized;
      }
    }
    if (dataLength > 0) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[0];
      if (i2cbuf[0 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[0 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = i2cbuf[0 + dataStart];
      }

      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[1];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = i2cbuf[1 + dataStart];
    }
    if (dataLength > 1) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[2];
      if (i2cbuf[2 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[2 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[2 + dataStart] << 8;
        tempVal |= i2cbuf[3 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = tempVal;
      }
    }
    if (dataLength > 3) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[3];
      if (i2cbuf[4 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[4 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
        ithoMeasurements.back().value.intval = i2cbuf[4 + dataStart];
      }
    }
    if (dataLength > 4) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[4];
      if (i2cbuf[5 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[5 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[5 + dataStart] / 2;
      }
    }
    if (dataLength > 5) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[5];
      if (i2cbuf[6 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[6 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[6 + dataStart] << 8;
        tempVal |= i2cbuf[7 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 7) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[6];
      if (i2cbuf[8 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[8 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[8 + dataStart] << 8;
        tempVal |= i2cbuf[9 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 9) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[7];
      if (i2cbuf[10 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[10 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[10 + dataStart] << 8;
        tempVal |= i2cbuf[11 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 11) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[8];
      if (i2cbuf[12 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[12 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[12 + dataStart] << 8;
        tempVal |= i2cbuf[13 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 13) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[9];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      int32_t tempVal = i2cbuf[14 + dataStart] << 8;
      tempVal |= i2cbuf[15 + dataStart];
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 15) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[10];
      if (i2cbuf[16 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[16 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[16 + dataStart] / 2;
      }
    }
    if (dataLength > 16) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[11];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
      auto it = fanInfo.find(i2cbuf[17 + dataStart] & 0x1F);
      if (it != fanInfo.end()) ithoMeasurements.back().value.stringval = it->second;
      else ithoMeasurements.back().value.stringval = fanInfo.rbegin()->second;
    }
    if (dataLength > 17) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[12];
      if (i2cbuf[18 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[18 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;

      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[18 + dataStart] / 2;
      }
    }
    if (dataLength > 18) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[13];
      if (i2cbuf[19 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors.find(i2cbuf[19 + dataStart]);
        if (it != fanSensorErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[19 + dataStart] / 2;
      }
    }
    if (dataLength > 19) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[14];
      int32_t tempVal = i2cbuf[20 + dataStart] << 8;
      tempVal |= i2cbuf[21 + dataStart];
      ithoMeasurements.back().type = ithoDeviceMeasurements::is_int;
      ithoMeasurements.back().value.intval = tempVal;
    }
    if (dataLength > 21) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[15];
      if (i2cbuf[22 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanHeatErrors.find(i2cbuf[22 + dataStart]);
        if (it != fanHeatErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[22 + dataStart] / 2;
      }
    }
    if (dataLength > 22) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[16];
      if (i2cbuf[23 + dataStart] > 200) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanHeatErrors.find(i2cbuf[23 + dataStart]);
        if (it != fanHeatErrors.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanHeatErrors.rbegin()->second;
      }
      else {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = i2cbuf[23 + dataStart] / 2;
      }
    }
    if (dataLength > 23) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[17];
      if (i2cbuf[24 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[24 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[24 + dataStart] << 8;
        tempVal |= i2cbuf[25 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
    if (dataLength > 25) {
      ithoMeasurements.push_back(ithoDeviceMeasurements());
      ithoMeasurements.back().name = labels31DA[18];

      if (i2cbuf[26 + dataStart] >= 0x7F) {
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_string;
        auto it = fanSensorErrors2.find(i2cbuf[26 + dataStart]);
        if (it != fanSensorErrors2.end()) ithoMeasurements.back().value.stringval = it->second;
        else ithoMeasurements.back().value.stringval = fanSensorErrors2.rbegin()->second;
      }
      else {
        int32_t tempVal = i2cbuf[26 + dataStart] << 8;
        tempVal |= i2cbuf[27 + dataStart];
        ithoMeasurements.back().type = ithoDeviceMeasurements::is_float;
        ithoMeasurements.back().value.floatval = tempVal / 100.0;
      }
    }
  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho31DA", "failed");
    }
  }

}

void sendQuery31D9(bool updateweb) {

  uint8_t command[] = { 0x82, 0x80, 0x31, 0xD9, 0x04, 0x00, 0xF0 };

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho31D9", i2cbuf2string(i2cbuf, len).c_str());
    }

    auto dataStart = 6;

    if (!ithoInternalMeasurements.empty()) {
      ithoInternalMeasurements.clear();
    }
    const int labelLen = 4;
    static const char* labels31D9[labelLen] {};
    for (int i = 0; i < labelLen; i++) {
      if (systemConfig.api_normalize == 0) {
        labels31D9[i] = itho31D9Labels[i].labelFull;
      }
      else {
        labels31D9[i] = itho31D9Labels[i].labelNormalized;
      }
    }

    double tempVal = i2cbuf[1 + dataStart] / 2.0;
    ithoDeviceMeasurements sTemp = {labels31D9[0], ithoDeviceMeasurements::is_float, {.floatval = tempVal} } ;
    ithoInternalMeasurements.push_back(sTemp);

    int status = 0;
    if (i2cbuf[0 + dataStart] == 0x80) {
      status = 1; //fault
    }
    else {
      status = 0; //no fault
    }
    ithoInternalMeasurements.push_back({labels31D9[1], ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x40) {
      status = 1; //frost cycle active
    }
    else {
      status = 0; //frost cycle not active
    }
    ithoInternalMeasurements.push_back({labels31D9[2], ithoDeviceMeasurements::is_int, {.intval = status}});
    if (i2cbuf[0 + dataStart] == 0x20) {
      status = 1; //filter dirty
    }
    else {
      status = 0; //filter clean
    }
    ithoInternalMeasurements.push_back({labels31D9[3], ithoDeviceMeasurements::is_int, {.intval = status}});
    //    if (i2cbuf[0 + dataStart] == 0x10) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x08) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x04) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x02) {
    //      //unknown
    //    }
    //    if (i2cbuf[0 + dataStart] == 0x01) {
    //      //unknown
    //    }

  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho31D9", "failed");
    }
  }
}

int32_t * sendQuery2410(bool & updateweb) {

  static int32_t values[3];
  values[0] = 0;
  values[1] = 0;
  values[2] = 0;

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x10, 0x04, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

  command[23] = index2410;
  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {

    uint8_t tempBuf[] = { i2cbuf[9], i2cbuf[8], i2cbuf[7], i2cbuf[6] };
    std::memcpy(&values[0], tempBuf, 4);

    uint8_t tempBuf2[] = { i2cbuf[13], i2cbuf[12], i2cbuf[11], i2cbuf[10] };
    std::memcpy(&values[1], tempBuf2, 4);

    uint8_t tempBuf3[] = { i2cbuf[17], i2cbuf[16], i2cbuf[15], i2cbuf[14] };
    std::memcpy(&values[2], tempBuf3, 4);

    if(ithoSettingsArray == nullptr) return nullptr;

    ithoSettingsArray[index2410].value = values[0];

    if (((i2cbuf[22] >> 3) & 0x07) == 0) {
      ithoSettingsArray[index2410].length = 1;
    }
    else {
      ithoSettingsArray[index2410].length = (i2cbuf[22] >> 3) & 0x07;
    }

    if ((i2cbuf[22] & 0x07) == 0) { //integer value
      if ((i2cbuf[22] & 0x80) == 0) { //unsigned value
        if (ithoSettingsArray[index2410].length == 1) {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
        }
        else if (ithoSettingsArray[index2410].length == 2) {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint16;
        }
        else {
          ithoSettingsArray[index2410].type = ithoSettings::is_uint32;
        }
      }
      else {
        if (ithoSettingsArray[index2410].length == 1) {
          ithoSettingsArray[index2410].type = ithoSettings::is_int8;
        }
        else if (ithoSettingsArray[index2410].length == 2) {
          ithoSettingsArray[index2410].type = ithoSettings::is_int16;
        }
        else {
          ithoSettingsArray[index2410].type = ithoSettings::is_int32;
        }
      }
    }
    else { //float

      if ((i2cbuf[22] & 0x04) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float1000;
      }
      else if ((i2cbuf[22] & 0x02) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float100;
      }
      else if ((i2cbuf[22] & 0x01) != 0) {
        ithoSettingsArray[index2410].type = ithoSettings::is_float10;
      }
      else {
        ithoSettingsArray[index2410].type = ithoSettings::is_unknown;
      }

    }
    //special cases
    if (i2cbuf[22] == 0x01) {
      ithoSettingsArray[index2410].type = ithoSettings::is_uint8;
    }

    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho2410", i2cbuf2string(i2cbuf, len).c_str());

      char tempbuffer0[256];
      char tempbuffer1[256];
      char tempbuffer2[256];

      if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_uint32) { //unsigned value
        uint32_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%u", val[0]);
        sprintf(tempbuffer1, "%u", val[1]);
        sprintf(tempbuffer2, "%u", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
        int8_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
        int16_t val[3];
        std::memcpy(&val[0], &values[0], sizeof(val[0]));
        std::memcpy(&val[1], &values[1], sizeof(val[0]));
        std::memcpy(&val[2], &values[2], sizeof(val[0]));
        sprintf(tempbuffer0, "%d", val[0]);
        sprintf(tempbuffer1, "%d", val[1]);
        sprintf(tempbuffer2, "%d", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_int32) {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float10) {
        double val[3];
        val[0] = values[0] / 10.0f;
        val[1] = values[1] / 10.0f;
        val[2] = values[2] / 10.0f;
        sprintf(tempbuffer0, "%.1f", val[0]);
        sprintf(tempbuffer1, "%.1f", val[1]);
        sprintf(tempbuffer2, "%.1f", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float100) {
        double val[3];
        val[0] = values[0] / 100.0f;
        val[1] = values[1] / 100.0f;
        val[2] = values[2] / 100.0f;
        sprintf(tempbuffer0, "%.2f", val[0]);
        sprintf(tempbuffer1, "%.2f", val[1]);
        sprintf(tempbuffer2, "%.2f", val[2]);
      }
      else if (ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
        double val[3];
        val[0] = values[0] / 1000.0f;
        val[1] = values[1] / 1000.0f;
        val[2] = values[2] / 1000.0f;
        sprintf(tempbuffer0, "%.3f", val[0]);
        sprintf(tempbuffer1, "%.3f", val[1]);
        sprintf(tempbuffer2, "%.3f", val[2]);
      }
      else {
        sprintf(tempbuffer0, "%d", values[0]);
        sprintf(tempbuffer1, "%d", values[1]);
        sprintf(tempbuffer2, "%d", values[2]);
      }
      //jsonSysmessage("itho2410cur", tempbuffer0);
      //jsonSysmessage("itho2410min", tempbuffer1);
      //jsonSysmessage("itho2410max", tempbuffer2);

    }


  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho2410", "failed");
    }

  }

  return values;

}

void setSetting2410(bool & updateweb) {

  if (index2410 == 7 && value2410 == 1 && (ithoDeviceID == 0x14 || ithoDeviceID == 0x1B || ithoDeviceID == 0x1D)) {
    //logMessagejson("<br>!!Warning!! Command ignored!<br>Setting index 7 to value 1 will switch off I2C!", WEBINTERFACE);
    return;
  }
  
  if(ithoSettingsArray == nullptr) return;

  uint8_t command[] = { 0x82, 0x80, 0x24, 0x10, 0x06, 0x13, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF, 0x00, 0xFF };

  command[23] = index2410;

  if (ithoSettingsArray[index2410].type == ithoSettings::is_uint8 || ithoSettingsArray[index2410].type == ithoSettings::is_int8) {
    command[9] = value2410 & 0xFF;
  }
  else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint16 || ithoSettingsArray[index2410].type == ithoSettings::is_int16) {
    command[9] = value2410 & 0xFF;
    command[8] = (value2410 >> 8) & 0xFF;
  }
  else if (ithoSettingsArray[index2410].type == ithoSettings::is_uint32 || ithoSettingsArray[index2410].type == ithoSettings::is_int32 || ithoSettingsArray[index2410].type == ithoSettings::is_float2 || ithoSettingsArray[index2410].type == ithoSettings::is_float10 || ithoSettingsArray[index2410].type == ithoSettings::is_float100 || ithoSettingsArray[index2410].type == ithoSettings::is_float1000) {
    command[9] = value2410 & 0xFF;
    command[8] = (value2410 >> 8) & 0xFF;
    command[7] = (value2410 >> 16) & 0xFF;
    command[6] = (value2410 >> 24) & 0xFF;
  }
  else {
    if (updateweb) {
      updateweb = false;
      //jsonSysmessage("itho2410setres", "format error, first use query 2410");
    }
    return; //unsupported value format
  }

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  //jsonSysmessage("itho2410set", i2cbuf2string(command, sizeof(command)).c_str());


  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));

  uint8_t i2cbuf[512] {};
  size_t len = i2c_slave_receive(i2cbuf);
  if (len > 1 && i2cbuf[len - 1] == checksum(i2cbuf, len - 1)) {
    if (updateweb) {
      updateweb = false;
      if (len > 2) {
        if (command[6] == i2cbuf[6] && command[7] == i2cbuf[7] && command[8] == i2cbuf[8] && command[9] == i2cbuf[9] && command[23] == i2cbuf[23]) {
          //jsonSysmessage("itho2410setres", "confirmed");
        }
        else {
          //jsonSysmessage("itho2410setres", "confirmation failed");
        }
      }
      else {
        //jsonSysmessage("itho2410setres", "failed");
      }
    }
  }

}


void filterReset() {

  //[I2C addr ][  I2C command   ][len ][    timestamp         ][fmt ][    remote ID   ][cntr][cmd opcode][len ][  command ][  counter ][chk]
  uint8_t command[] = { 0x82, 0x62, 0xC1, 0x01, 0x01, 0x10, 0xFF, 0xFF, 0xFF, 0xFF, 0x16, 0xFF, 0xFF, 0xFF, 0xFF, 0x10, 0xD0, 0x02, 0x63, 0xFF, 0x00, 0x00, 0xFF };

  unsigned long curtime = millis();

  command[6] = (curtime >> 24) & 0xFF;
  command[7] = (curtime >> 16) & 0xFF;
  command[8] = (curtime >> 8) & 0xFF;
  command[9] = curtime & 0xFF;

  command[11] = id0;
  command[12] = id1;
  command[13] = id2;

  command[14] = cmdCounter;
  cmdCounter++;

  command[sizeof(command) - 1] = checksum(command, sizeof(command) - 1);

  while (digitalRead(SCLPIN) == LOW ) {
    yield();
    delay(1);
  }

  i2c_sendBytes(command, sizeof(command));


}

int quick_pow10(int n) {
  static int pow10[10] = {
    1, 10, 100, 1000, 10000,
    100000, 1000000, 10000000, 100000000, 1000000000
  };
  if(n>(sizeof(pow10)/sizeof(pow10[0])-1)) return 1;
  return pow10[n];
}

std::string i2cbuf2string(const uint8_t* data, size_t len) {
  std::string s;
  s.reserve(len * 3 + 2);
  for (size_t i = 0; i < len; ++i) {
    if (i)
      s += ' ';
    s += toHex(data[i] >> 4);
    s += toHex(data[i] & 0xF);
  }
  return s;
}

}
}