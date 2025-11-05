#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SECONDS2HOURS 1/3600

//note: DST in US: 2nd Sunday March - 1st Sunday November
// DST in Europe: 30 March - 26 Oct
//so EU & US is in DST most of the time, however during the main part of the sports season they arent. what to do?

int strToInt(char *str, int lengthOfStr) {
    int result = 0;
    int sign = 1;
    int i = 0;

    // Handle sign
    if (str[i] == '+' || str[i] == '-') {
        if (str[i] == '-') {
            sign = -1;
        }
        i++;
    }

    // Convert digits
    while (i < lengthOfStr) {
        result = result * 10 + (str[i] - '0');
        i++;
    }

    return result * sign;
}
void zeroString(char * string, int length){
  for(int i = 0; i < length; i++){
    string[i] = '0';
  }
}

typedef struct DateTime{
  char * datetimestring;
  int years;
  int months;
  int days;
  int hours;
  int minutes;
  int seconds;

  int timezoneOffsetHours; //if this number is neg then so is timezoneOffsetMinutes
  int timezoneOffsetMinutes;
} DateTime;

void PrintDateTime(DateTime * d){
  printf("years: %d\n", d->years);
  printf("months: %d\n", d->months);
  printf("days: %d\n", d->days);
  printf("hours: %d\n", d->hours);
  printf("minutes: %d\n", d->minutes);
  printf("seconds: %d\n", d->seconds);

  printf("timezoneOffsetHours: %d\n", d->timezoneOffsetHours);
  printf("timezoneOffsetMinutes: %d\n", d->timezoneOffsetMinutes);
}

//TIME STUFF
time_t now; //time_t is either a long or long long. represents seconds since Jan 1 1970

DateTime * ConvertDateTimeToDTStruct(char * DateT){
  /*Format: YYYY-MM-DDTHH:MM:SS+HHO:MMO
  YYYY = Year
  MM = Month
  DD = Day
  T = Seperator between Date and time
  HH = Hour of day (0-23)
  MM = Minute of hour (0-59)
  SS = Second of minute (0-59)
  + = start of sequence representing offset from UTC
  HHO = Hours offset from UTC
  MMO = Minutes offset from UTC */
  // int timeElements[8]; //all the elements of the DateT in order
  // int * timeElements = (int *)malloc(8 * sizeof(int));
  int timeElements[8];
  int charIndex = 0;
  char strBuffer[4];
  char signForTAfterUCT;

  for(int i = 0; i < 4; i++){
    strBuffer[i] = DateT[charIndex];
    charIndex++;
  }
  timeElements[0] = strToInt(strBuffer, 4);
  zeroString(strBuffer, 4);
  charIndex++; //skip the - (dash) symbol

  //YYYY is the only 4 digit number so from here we can use a loop to fill the rest in
  for(int i = 1; i < 8; i++){
    int stopAt = charIndex+2;
    for(int j = charIndex; j < stopAt; j++){
      strBuffer[j - (stopAt - 2)] = DateT[charIndex];
      charIndex++;
    }
    int a = strToInt(strBuffer, 2);
    timeElements[i] = a;
    zeroString(strBuffer, 4);
    if(i == 5){signForTAfterUCT = DateT[charIndex];}
    charIndex++;//skip the symbol
  }

  DateTime * d = (DateTime *)malloc(sizeof(DateTime));
  int rawDTStrLen = strlen(DateT) + 1;
  d->datetimestring = (char*)malloc(sizeof(char) * rawDTStrLen);
  strcpy(d->datetimestring, DateT);
  d->datetimestring[rawDTStrLen-1] = '\0';

   d->years = timeElements[0];
   d->months = timeElements[1];
   d->days = timeElements[2];
   d->hours = timeElements[3];
   d->minutes = timeElements[4];
   d->seconds = timeElements[5];

   d->timezoneOffsetHours = timeElements[6]; //if this number is neg then so is timezoneOffsetMinutes
   d->timezoneOffsetMinutes = timeElements[7];

  return d;
}

// Cumulative days at start of each month (non-leap year)
int month_days[] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

int is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int hoursAndMinsToMins(int h, int m){
  int sign = h < 0 ? 1 : -1;
  return (h*60) + (sign *m);
}

time_t timeDifference(time_t a, time_t b){
  //returns absolute value of time difference between two times
  time_t c = a - b;
  if(c < 0){
    c *= -1;
  }
  return c;
}

time_t datetime_to_epoch(DateTime * dat) {

  int year = dat->years;
  int month = dat->months;
  int day = dat->days;
  int hour = dat->hours;
  int minute = dat->minutes;
  int second = dat->seconds;
  int tz_offset_minutes = hoursAndMinsToMins(dat->timezoneOffsetHours, dat->timezoneOffsetMinutes);
    time_t total_days = 0;
    time_t total_seconds = 0;
    int leap_days = 0;
    int i;

    // Calculate leap days from 1970 to year-1
    for (i = 1972; i < year; i += 4) {
        if (is_leap_year(i)) leap_days++;
    }

    // Days from complete years
    total_days = (year - 1970) * 365 + leap_days;

    // Days from months in current year
    total_days += month_days[month - 1];

    // Add extra day if current year is leap and we're past February
    if (month > 2 && is_leap_year(year)) {
        total_days += 1;
    }

    // Add days in current month
    total_days += day - 1;

    // Convert to total seconds
    total_seconds = total_days * 86400LL;  // Days to seconds
    total_seconds += hour * 3600LL;        // Hours to seconds
    total_seconds += minute * 60LL;        // Minutes to seconds
    total_seconds += second;               // Seconds

    // Apply timezone offset (subtract because we want UTC)
    // Positive offset means ahead of UTC, so we subtract to get UTC
    total_seconds -= tz_offset_minutes * 60LL;

    return total_seconds;
}

struct TimeZoneReferenceTable{
  char * TimeZoneName[116];
  char * TimeZoneCode[116];
  int UTCOffset[116];
  int DaylightSavingsUTCOffset[116];
  int length;
};
// Initialize the main timezone table at declaration
struct TimeZoneReferenceTable timezones = {
  .TimeZoneName = {
    "America/New_York", "America/New_York", "America/Chicago", "America/Chicago",
    "America/Denver", "America/Denver", "America/Los_Angeles", "America/Los_Angeles",
    "America/Anchorage", "America/Anchorage", "America/Honolulu", "America/Toronto",
    "America/Toronto", "America/Vancouver", "America/Vancouver", "America/Mexico_City",
    "America/Mexico_City", "America/Sao_Paulo", "America/Sao_Paulo", "America/Argentina/Buenos_Aires",
    "America/Lima", "America/Bogota", "America/Caracas", "America/Santiago",
    "America/Santiago", "Europe/London", "Europe/London", "Europe/Dublin",
    "Europe/Dublin", "Europe/Paris", "Europe/Paris", "Europe/Berlin",
    "Europe/Berlin", "Europe/Rome", "Europe/Rome", "Europe/Madrid",
    "Europe/Madrid", "Europe/Amsterdam", "Europe/Amsterdam", "Europe/Brussels",
    "Europe/Brussels", "Europe/Zurich", "Europe/Zurich", "Europe/Vienna",
    "Europe/Vienna", "Europe/Prague", "Europe/Prague", "Europe/Warsaw",
    "Europe/Warsaw", "Europe/Stockholm", "Europe/Stockholm", "Europe/Oslo",
    "Europe/Oslo", "Europe/Copenhagen", "Europe/Copenhagen", "Europe/Helsinki",
    "Europe/Helsinki", "Europe/Athens", "Europe/Athens", "Europe/Istanbul",
    "Europe/Moscow", "Europe/Kiev", "Europe/Kiev", "Asia/Tokyo",
    "Asia/Seoul", "Asia/Shanghai", "Asia/Hong_Kong", "Asia/Singapore",
    "Asia/Bangkok", "Asia/Jakarta", "Asia/Manila", "Asia/Kuala_Lumpur",
    "Asia/Taipei", "Asia/Kolkata", "Asia/Mumbai", "Asia/Karachi",
    "Asia/Dhaka", "Asia/Dubai", "Asia/Tehran", "Asia/Tehran",
    "Asia/Jerusalem", "Asia/Jerusalem", "Asia/Riyadh", "Asia/Yekaterinburg",
    "Asia/Novosibirsk", "Asia/Krasnoyarsk", "Asia/Irkutsk", "Asia/Vladivostok",
    "Africa/Cairo", "Africa/Cairo", "Africa/Lagos", "Africa/Johannesburg",
    "Africa/Nairobi", "Africa/Casablanca", "Africa/Casablanca", "Africa/Algiers",
    "Africa/Tunis", "Australia/Sydney", "Australia/Sydney", "Australia/Melbourne",
    "Australia/Melbourne", "Australia/Brisbane", "Australia/Perth", "Australia/Adelaide",
    "Australia/Adelaide", "Australia/Darwin", "Pacific/Auckland", "Pacific/Auckland",
    "Pacific/Fiji", "Pacific/Fiji", "Pacific/Honolulu", "Pacific/Guam",
    "UTC", "GMT", "Etc/UTC", "Etc/GMT"
  },
  .TimeZoneCode = {
    "EST", "EDT", "CST", "CDT", "MST", "MDT", "PST", "PDT",
    "AKST", "AKDT", "HST", "EST", "EDT", "PST", "PDT", "CST",
    "CDT", "BRT", "BRST", "ART", "PET", "COT", "VET", "CLT",
    "CLST", "GMT", "BST", "GMT", "IST", "CET", "CEST", "CET",
    "CEST", "CET", "CEST", "CET", "CEST", "CET", "CEST", "CET",
    "CEST", "CET", "CEST", "CET", "CEST", "CET", "CEST", "CET",
    "CEST", "CET", "CEST", "CET", "CEST", "CET", "CEST", "EET",
    "EEST", "EET", "EEST", "TRT", "MSK", "EET", "EEST", "JST",
    "KST", "CST", "HKT", "SGT", "ICT", "WIB", "PHT", "MYT",
    "CST", "IST", "IST", "PKT", "BST", "GST", "IRST", "IRDT",
    "IST", "IDT", "AST", "YEKT", "NOVT", "KRAT", "IRKT", "VLAT",
    "EET", "EEST", "WAT", "SAST", "EAT", "WET", "WEST", "CET",
    "CET", "AEST", "AEDT", "AEST", "AEDT", "AEST", "AWST", "ACST",
    "ACDT", "ACST", "NZST", "NZDT", "FJT", "FJST", "HST", "ChST",
    "UTC", "GMT", "UTC", "GMT"
  },
  .UTCOffset = {
    -18000,-18000,-21600,-21600,-25200,-25200,-28800,-28800,-32400,-32400,
    -36000,-18000,-18000,-28800,-28800,-21600,-21600,-10800,-10800,-10800,
    -18000,-18000,-14400,-14400,-14400,0,0,0,0,3600,3600,3600,3600,3600,
    3600,3600,3600,3600,3600,3600,3600,3600,3600,3600,3600,3600,3600,3600,
    3600,3600,3600,3600,3600,3600,3600,7200,7200,7200,7200,10800,10800,
    7200,7200,32400,32400,28800,28800,28800,25200,25200,28800,28800,28800,
    19800,19800,18000,21600,14400,12600,12600,7200,7200,10800,18000,25200,
    25200,28800,36000,7200,7200,3600,7200,10800,0,0,3600,3600,36000,36000,
    36000,36000,36000,28800,34200,34200,34200,43200,43200,43200,43200,
    -36000,36000,0,0,0,0
  },
  .DaylightSavingsUTCOffset = {
    -14400,-14400,-18000,-18000,-21600,-21600,-25200,-25200,-28800,-28800,
    -36000,-14400,-14400,-25200,-25200,-18000,-18000,-7200,-7200,-10800,
    -18000,-18000,-14400,-10800,-10800,3600,3600,3600,3600,7200,7200,7200,
    7200,7200,7200,7200,7200,7200,7200,7200,7200,7200,7200,7200,7200,7200,
    7200,7200,7200,7200,7200,7200,7200,7200,7200,10800,10800,10800,10800,
    10800,10800,10800,10800,32400,32400,28800,28800,28800,25200,25200,
    28800,28800,28800,19800,19800,18000,21600,14400,16200,16200,10800,
    10800,10800,18000,25200,25200,28800,36000,10800,10800,3600,7200,10800,
    3600,3600,3600,3600,39600,39600,39600,39600,36000,28800,37800,37800,
    34200,46800,46800,46800,46800,-36000,36000,0,0,0,0
  },
  .length = 116
};

struct TimeZoneReferenceTable_LegacyPOSIX{ //translates
  char * POSIX_TimeZoneName[99];
  char * TimeZoneCode[99];
  int  UTCOffset[99];
  int  DaylightSavingsUTCOffset[99];
  int length;
};

// Initialize the legacy POSIX timezone table at declaration
struct TimeZoneReferenceTable_LegacyPOSIX LegacyPOSIXTimezoneReferenceTable = {
  .POSIX_TimeZoneName = {
    "US/Alaska", "US/Alaska", "US/Aleutian", "US/Aleutian", "US/Arizona",
    "US/Central", "US/Central", "US/East-Indiana", "US/East-Indiana", "US/Eastern",
    "US/Eastern", "US/Hawaii", "US/Indiana-Starke", "US/Indiana-Starke", "US/Michigan",
    "US/Michigan", "US/Mountain", "US/Mountain", "US/Pacific", "US/Pacific",
    "US/Pacific-New", "US/Pacific-New", "US/Samoa", "Canada/Atlantic", "Canada/Atlantic",
    "Canada/Central", "Canada/Central", "Canada/Eastern", "Canada/Eastern", "Canada/Mountain",
    "Canada/Mountain", "Canada/Newfoundland", "Canada/Newfoundland", "Canada/Pacific", "Canada/Pacific",
    "Canada/Saskatchewan", "Canada/Yukon", "Canada/Yukon", "Mexico/BajaNorte", "Mexico/BajaNorte",
    "Mexico/BajaSur", "Mexico/BajaSur", "Mexico/General", "Mexico/General", "Brazil/Acre",
    "Brazil/DeNoronha", "Brazil/East", "Brazil/East", "Brazil/West", "Chile/Continental",
    "Chile/Continental", "Chile/EasterIsland", "Chile/EasterIsland", "Jamaica", "Cuba",
    "Cuba", "Europe/Belfast", "Europe/Belfast", "Europe/Tiraspol", "Europe/Tiraspol",
    "Asia/Ashkhabad", "Asia/Calcutta", "Asia/Chungking", "Asia/Dacca", "Asia/Istanbul",
    "Asia/Katmandu", "Asia/Macao", "Asia/Rangoon", "Asia/Saigon", "Asia/Tel_Aviv",
    "Asia/Tel_Aviv", "Asia/Thimbu", "Asia/Ujung_Pandang", "Asia/Ulan_Bator", "Africa/Asmera",
    "Africa/Timbuktu", "Australia/ACT", "Australia/ACT", "Australia/Canberra", "Australia/Canberra",
    "Australia/LHI", "Australia/LHI", "Australia/NSW", "Australia/NSW", "Australia/North",
    "Australia/Queensland", "Australia/South", "Australia/South", "Australia/Tasmania", "Australia/Tasmania",
    "Australia/Victoria", "Australia/Victoria", "Australia/West", "Australia/Yancowinna", "Australia/Yancowinna",
    "Pacific/Ponape", "Pacific/Samoa", "Pacific/Truk", "Pacific/Yap"
  },
  .TimeZoneCode = {
    "AKST", "AKDT", "HST", "HDT", "MST", "CST", "CDT", "EST", "EDT", "EST",
    "EDT", "HST", "CST", "CDT", "EST", "EDT", "MST", "MDT", "PST", "PDT",
    "PST", "PDT", "SST", "AST", "ADT", "CST", "CDT", "EST", "EDT", "MST",
    "MDT", "NST", "NDT", "PST", "PDT", "CST", "PST", "PDT", "PST", "PDT",
    "MST", "MDT", "CST", "CDT", "ACT", "FNT", "BRT", "BRST", "AMT", "CLT",
    "CLST", "EAST", "EASST", "EST", "CST", "CDT", "GMT", "BST", "EET", "EEST",
    "TMT", "IST", "CST", "BST", "TRT", "NPT", "CST", "MMT", "ICT", "IST",
    "IDT", "BTT", "WITA", "ULAT", "EAT", "GMT", "AEST", "AEDT", "AEST", "AEDT",
    "LHST", "LHDT", "AEST", "AEDT", "ACST", "AEST", "ACST", "ACDT", "AEST", "AEDT",
    "AEST", "AEDT", "AWST", "ACST", "ACDT", "PONT", "SST", "CHUT", "CHUT"
  },
  .UTCOffset = {
    -32400,-32400,-36000,-36000,-25200,-21600,-21600,-18000,-18000,-18000,
    -18000,-36000,-21600,-21600,-18000,-18000,-25200,-25200,-28800,-28800,
    -28800,-28800,-39600,-14400,-14400,-21600,-21600,-18000,-18000,-25200,
    -25200,-12600,-12600,-28800,-28800,-21600,-28800,-28800,-28800,-28800,
    -25200,-25200,-21600,-21600,-18000,-7200,-10800,-10800,-14400,-14400,
    -14400,-21600,-21600,-18000,-18000,-18000,0,0,7200,7200,18000,19800,
    28800,21600,10800,20700,28800,23400,25200,7200,7200,21600,28800,28800,
    10800,0,36000,36000,36000,36000,37800,37800,36000,36000,34200,36000,
    34200,34200,36000,36000,36000,36000,28800,34200,34200,39600,-39600,
    36000,36000
  },
  .DaylightSavingsUTCOffset = {
    -28800,-28800,-32400,-32400,-25200,-18000,-18000,-14400,-14400,-14400,
    -14400,-36000,-18000,-18000,-14400,-14400,-21600,-21600,-25200,-25200,
    -25200,-25200,-39600,-10800,-10800,-18000,-18000,-14400,-14400,-21600,
    -21600,-9000,-9000,-25200,-25200,-21600,-25200,-25200,-25200,-25200,
    -21600,-21600,-18000,-18000,-18000,-7200,-7200,-7200,-14400,-10800,
    -10800,-18000,-18000,-18000,-14400,-14400,3600,3600,10800,10800,18000,
    19800,28800,21600,10800,20700,28800,23400,25200,10800,10800,21600,
    28800,28800,10800,0,39600,39600,39600,39600,39600,39600,39600,39600,
    34200,36000,37800,37800,39600,39600,39600,39600,28800,37800,37800,
    39600,-39600,36000,36000
  },
  .length = 99
};

//helper function to check if a tz is UTC to prevent unnessecary searching
int TzIsUTC(char * f){
  if(f[0] == 'U' && f[1] == 'T' && f[2] == 'C'){
    return 1;
  }
  return 0;
}

int GetIndexOfTimeZoneCode(char * tzcode){
  //validate tzcode string: max 4 letters, min 3 letters
  int tzcodeLength = strlen(tzcode);
  if(tzcodeLength != 4 && tzcodeLength != 3){return -1;}
  int found = 1;
  //use an extremely inefficient double loop linear search for now
  for(int i = 0; i < timezones.length; i++){
    for(int j = 0; j < tzcodeLength; j++){
      if(timezones.TimeZoneCode[i][j] != tzcode[j]){
        found = 0;
        break;
      }
    }
    if(found == 1){
      return i;
    }else{found = 1;}
  }

  return -1;

}
//get seconds difference between utc and a given time zone
void GetOffsetFromUTC(char * tzcode, int * utcOffsetInHere, int * utcDSTOffsetInHere){
  int i = GetIndexOfTimeZoneCode(tzcode);
  *utcOffsetInHere = timezones.UTCOffset[i];
  *utcDSTOffsetInHere = timezones.DaylightSavingsUTCOffset[i];
}

int GetTimeDifferenceBetween2TimeZones(char * tzcode1, char * tzcode2){
  //if either one is 'UTC', just avoid unnessecary searching
  if(TzIsUTC(tzcode1) == 1){
    return timezones.UTCOffset[GetIndexOfTimeZoneCode(tzcode2)];
  }
  if(TzIsUTC(tzcode2) == 1){
    return timezones.UTCOffset[GetIndexOfTimeZoneCode(tzcode1)];
  }
  int offset1 = GetIndexOfTimeZoneCode(tzcode1);
  int offset2 = GetIndexOfTimeZoneCode(tzcode2);
  if(offset1 == -1 || offset2 == -1){
    return -1;
  }
  return timezones.UTCOffset[offset1] + timezones.UTCOffset[offset2];
}

// int main(int argc, char ** argv){
//
//   printf("%f\n", ((float)GetTimeDifferenceBetween2TimeZones(argv[1], argv[2])) / 3600);
// }
/*int main(){
  // char inputString1[] = "2025-06-17T04:19:00-00:00";
  // char inputString2[] = "2025-06-16T04:19:00-00:00";
  // // PrintDateTime(ConvertDateTimeToDTStruct(inputString1));
  // time_t aEpoch = datetime_to_epoch(ConvertDateTimeToDTStruct(inputString1));
  // time_t bEpoch = datetime_to_epoch(ConvertDateTimeToDTStruct(inputString2));
  // printf("%lld", timeDifference(aEpoch, bEpoch));
  printf("%lld", datetime_to_epoch(ConvertDateTimeToDTStruct("2025-06-29T20:00:00+00:00")));

}*/
