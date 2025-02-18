#ifndef BZ251_H__
#define BZ251_H__

#include <stdint.h>
#include <driver/gpio.h>
#include <driver/uart.h>


typedef struct {
    uart_port_t uartNum;
    gpio_num_t tx;
    gpio_num_t rx;
} bz251Config;

class Bz251
{
    public:

        struct Bz251Data {
            float latitude;
            float longitude;
            float altitude;
            uint8_t year;
            uint8_t month;
            uint8_t day;
            uint8_t hour;
            uint8_t minute;
        };
        Bz251Data data;
        

        void init(bz251Config config);

        uint8_t getPosition(float &latitude, float &longitude);
        uint8_t getAltitude(float &altitude);
        uint8_t getTime(uint8_t &hour, uint8_t &minute);
        uint8_t getDate(uint8_t &day, uint8_t &month, uint8_t &year);
        uint8_t setTimeZone(uint8_t tZone);
    
        void configSetValue(uint64_t key, uint8_t value);
        void configGetValue(uint64_t key);
        void reset(void);
        void factoryReset(void);
        void setValueFlash(uint64_t key, uint8_t value);

    private:

        struct rmcData{
            uint32_t utc;           /* hhmmss.sss */
            uint8_t pos_status;     /* A=data valid or V=data not valid */
            float latitude;         /* ddmm.mmmm */
            uint8_t lat_dir;        /* N=north or S=south */
            float longitude;        /* dddmm.mmmm */
            uint8_t lon_dir;        /* E=east or W=west */
            float speed_Kn;         /* Speed over ground, knots */
            float track_true;       /* Track made good, degrees True */
            uint32_t date;          /* ddmmyy */
            float mag_var;          /* Magnetic variation, degrees */
            uint8_t var_dir;        /* degrees E=east or W=west */    
            uint8_t mode_ind;       /* A=Autonomous, D=Diferential, E=Estimated M=Manual, N=Data no valid */
        };
        rmcData rmc;

        struct ggaData{
            uint32_t utc;           /* hhmmss.sss */
            float latitude;         /* ddmm.mmmm */
            uint8_t lat_dir;        /* N=north or S=south */
            float longitude;        /* dddmm.mmmm */
            uint8_t lon_dir;        /* E=east or W=west */
            uint8_t quality;        /*  0 - Fix not available or invalid
                                        1 - Single point
                                            Converging PPP (TerraStar-L)
                                        2 - Pseudorange differential
                                            Converged PPP (TerraStar-L)
                                            Converging PPP (TerraStar-C PRO, TerraStar-X)
                                        4 - RTK fixed ambiguity solution
                                        5 - RTK floating ambiguity solution 
                                            Converged PPP (TerraStar-C PRO, TerraStar-X)
                                        6 - Dead reckoning mode
                                        7 - Manual input mode (fixed position)
                                        8 - Simulator mode
                                        9 - WAAS (SBAS)1 */
            uint8_t satellites;    /* Number of satellites in use */
            float hdop;             /* Horizontal dilution of precision */
            float altitude;         /* Antenna altitude above/below mean sea level */
            uint8_t a_units;        /* Units of antenna altitude (M = metres) */       
            float undulation;       /* Undulation - the relationship between the geoid and the WGS84 ellipsoid */
            uint8_t u_units;        /* Units of undulation (M = metres) */
            uint8_t age;            /* Age of correction data (in seconds) */
            uint16_t stn_ID;        /* Differential base station ID */
        };
        ggaData gga;
        
        uint8_t read(void);
        uint8_t getWeekday(uint8_t day, uint8_t month, uint8_t year);
        uint8_t getFirstDay(uint8_t year);
        uint8_t getLastDay(uint8_t year);
        uint8_t addTimeZone(uint8_t &hour, uint8_t &day, uint8_t &month, uint8_t &year);
        uint8_t sync(uint32_t &rawTime, uint32_t &rawDate); 

        uint8_t timeZone = 1;

        void calculateChecksum(uint8_t *message, uint8_t length);
};


#endif