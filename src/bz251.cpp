#include <bz251.h>
#include <bz251_defs.h>
#include <esp_log.h>
#include <driver/uart.h>
#include <sstream>

using namespace std;

static const char *TAG = "BZ251";

uint8_t Bz251::read(void)
{
    uint8_t* data = (uint8_t*) malloc(1024+1);

    const int rxBytes = uart_read_bytes(UART_NUM_2, data, 1024,pdMS_TO_TICKS(500));
    if (rxBytes > 0) {
        data[rxBytes] = 0;
    }

    string GXRMC[13];
    string GXGGA[13];
    uint8_t pos = 0;
    string data_GXRMC;
    string data_GXGGA;
    string gnss_string = string((char *)data);
    free(data);

    stringstream gnss_string_to_stream(gnss_string);
    
    string data_GNSS;

    while (getline(gnss_string_to_stream, data_GNSS, '\n'))
    {
        if (data_GNSS.find("RMC")<10)
        {
            data_GXRMC = data_GNSS;
        }
        else if (data_GNSS.find("GGA")<10)
        {
            data_GXGGA = data_GNSS;
        }
    }
    
    stringstream data_GXRMC_to_stream(data_GXRMC);
    stringstream data_GXGGA_to_stream(data_GXGGA);

    while (getline(data_GXRMC_to_stream, data_GNSS, ','))
    {
        GXRMC[pos] = data_GNSS;
        pos++;
    }
    pos = 0;

    while (getline(data_GXGGA_to_stream, data_GNSS, ','))
    {
        GXGGA[pos] = data_GNSS;
        pos++;
    }
    pos = 0;

    /**
     * V = 86 A = 65
     * N = 78 S = 83
     * E = 69 W = 87
     */
    if( GXRMC[2][0] == 86)
    {
        ESP_LOGW(TAG,"RMC No signal");
    }
    else if( GXRMC[2][0] == 65)
    {
        this->rmc.utc = stoi(GXRMC[1].substr(0,6));
        this->rmc.pos_status = GXRMC[2][0];
        this->rmc.latitude = stof(GXRMC[3])/100;
        this->rmc.lat_dir = GXRMC[4][0];
        this->rmc.longitude = stof(GXRMC[5])/1000;
        this->rmc.lon_dir = GXRMC[6][0];
        this->rmc.speed_Kn = stof(GXRMC[7]);
        if (GXRMC[8]!=""){this->rmc.track_true = stof(GXRMC[8]);}
        this->rmc.date = stoi(GXRMC[9].substr(0,6));
        if (GXRMC[10]!=""){this->rmc.mag_var = stof(GXRMC[10]);}
        this->rmc.var_dir = GXRMC[11][0];
        this->rmc.mode_ind = GXRMC[12][0];  
    }

    if( GXGGA[6][0] == 0)
    {
        ESP_LOGW(TAG,"GGA No signal");
    }
    else if( GXGGA[6][0] <= 9)
    {
        this->gga.utc = stoi(GXGGA[1].substr(0,6));
        this->gga.latitude = stof(GXGGA[2])/100;
        this->gga.lat_dir = GXGGA[3][0];
        this->gga.longitude = stof(GXGGA[4])/1000;
        this->gga.lon_dir = GXGGA[5][0];
        this->gga.quality = GXGGA[6][0];
        this->gga.satellites = stoi(GXGGA[7].substr(0,2));
        this->gga.hdop = stof(GXGGA[8]);
        this->gga.altitude = stof(GXGGA[9]);
        this->gga.a_units = GXGGA[10][0];
        this->gga.undulation = stof(GXGGA[11]);
        this->gga.u_units = GXGGA[12][0];
        this->gga.age = stoi(GXGGA[13].substr(0,2));
        this->gga.stn_ID = stoi(GXGGA[14].substr(0,3));
    }

    return 0;
}

uint8_t Bz251::getPosition(float &latitude, float &longitude)
{
    read();
    if( this->rmc.pos_status == 65)
    {
        latitude = this->rmc.latitude;
        longitude = this->rmc.longitude;
        if (this->rmc.lat_dir == 83){ latitude = latitude * -1; };
        if (this->rmc.lon_dir == 87){ longitude = longitude * -1; };
    }

    return 0;
}

uint8_t Bz251::getAltitude(float &altitude)
{
    read();
    if( this->rmc.pos_status == 65)
    {
        altitude = this->gga.altitude;
    }

    return 0;
}

uint8_t Bz251::getWeekday(uint8_t day, uint8_t month, uint8_t year)
{
    static int t[] = {0, 3, 2, 5, 0, 3, 5, 1, 4, 6, 2, 4};
        if( month < 3 )
        {
          year -= 1;
        }
        return (year + year/4 - year/100 + year/400 + t[month-1] + day) % 7;
}

uint8_t Bz251::getFirstDay(uint8_t year)
{
    /* ultimo domingo Marzo*/
    for (uint8_t day=31; day>0; day--)
        {
            if (getWeekday(day, 3, year) == 0)
            {
                return day;
            }
        }
    return 0;
}

uint8_t Bz251::getLastDay(uint8_t year)
{
    /* ultimo domingo Octubre*/
    for (uint8_t day=31; day>0; day--)
        {
            if (getWeekday(day, 10, year) == 0)
            {
                return day;
            }
        }
    return 0;
}

uint8_t Bz251::addTimeZone(uint8_t &hour, uint8_t &day, uint8_t &month, uint8_t &year)
{
    uint8_t daysMonth[12] = {31,28,31,30,31,30,31,31,30,31,30,31};
    if (((year % 4 == 0) && (year % 100 != 0)) || (year % 400 == 0))
    {
        daysMonth[1]=29;
    }

    hour++;
    if (hour > 23)
    {
        hour = 0;
        day++;
        if (day > daysMonth[month-1])
        {
            day = 1;
            month++;
            if (month > 12)
            {
                month = 1;
                year++;
            }
        }
    }
    return 0;
}

uint8_t Bz251::sync(uint32_t &rawTime, uint32_t &rawDate)
{
    string strRawTime = to_string(rawTime);

    uint8_t hour = stoi(strRawTime.substr(0,2));
    uint8_t minute = stoi(strRawTime.substr(2,2));
    uint8_t second = stoi(strRawTime.substr(4,2));

    string strRawDate = to_string(rawDate);

    uint8_t day = stoi(strRawDate.substr(0,2));
    uint8_t month = stoi(strRawDate.substr(2,2));
    uint8_t year = stoi(strRawDate.substr(4,2));

    for (uint8_t i=0; i<timeZone; i++)
    {
        addTimeZone(hour, day, month, year);
    }

    if (((month > 3) && (month < 10)) || 
        ((month == 3) && (day >= getFirstDay(year))) || 
        ((month == 10) && (day <= getLastDay(year))))
    {
        addTimeZone(hour, day, month, year);
    }
    // printf("%u:%u - %u-%u-%u",hour, minute, day, month, year);

    this->rmc.utc = (hour * 10000) + (minute * 100) + second;
    this->rmc.date = (day * 10000) + (month * 100) + year;

    return 0;
}

uint8_t Bz251::getTime(uint8_t &hour, uint8_t &minute)
{
    read();
    if( this->rmc.pos_status == 65)
    {
        sync(this->rmc.utc, this->rmc.date);
        string strRawTime = to_string(this->rmc.utc);
        if (this->rmc.utc >= 100000)
        {
            hour = stoi(strRawTime.substr(0,2));
            minute = stoi(strRawTime.substr(2,2));
        }
        else
        {
            hour = strRawTime[0];
            minute = stoi(strRawTime.substr(1,2));
        }
    }
    return 0;
}

uint8_t Bz251::getDate(uint8_t &day, uint8_t &month, uint8_t &year)
{
    read();
    if( this->rmc.pos_status == 65)
    {
        sync(this->rmc.utc, this->rmc.date);
        string strRawDate = to_string(this->rmc.date);
        if (this->rmc.date >= 100000)
        {
            day = stoi(strRawDate.substr(0,2));
            month = stoi(strRawDate.substr(2,2));
            year = stoi(strRawDate.substr(4,2));
        }
        else
        {
            day = strRawDate[0];
            month = stoi(strRawDate.substr(1,2));
            year = stoi(strRawDate.substr(3,2));
        }
    }
    return 0;
}


uint8_t Bz251::setTimeZone(uint8_t tZone)
{
    this->timeZone = tZone;

    return 0;
}

/****************************
 *                          *
 *                          *
 *         CONFIG           *
 *                          *
 *                          *
 ***************************/

 void Bz251::calculateChecksum(uint8_t *message, uint8_t length) {
    uint8_t ck_a = 0, ck_b = 0;
    for (int i = 2; i < length - 2; i++) {
        ck_a += message[i];
        ck_b += ck_a;
    }
    message[length - 2] = ck_a;
    message[length - 1] = ck_b;
}

void Bz251::reset() {
    uint8_t ubx_reset_cmd[] = {
        0xB5, 0x62,  // Sync chars
        0x06, 0x04,  // Clase e ID
        0x04, 0x00,  // Longitud del payload
        0xFF, 0xFF, 0x00, 0x00,  // Payload: Cold start
        0x00, 0x00  // Checksum (placeholder)
    };

    calculateChecksum(ubx_reset_cmd, sizeof(ubx_reset_cmd));
    uart_write_bytes(UART_NUM_2, (const char*)ubx_reset_cmd, sizeof(ubx_reset_cmd));

    ESP_LOGE(TAG, "Restart");
}

void Bz251::factoryReset() {
    uint8_t ubx_cfg_cfg_factory_reset[] = {
        0xB5, 0x62,  // Encabezado UBX
        0x06, 0x09,  // Clase e ID (CFG-CFG)
        0x0D, 0x00,  // Longitud del payload (13 bytes)
        0xFF, 0xFF, 0xFF, 0xFF,  // clearMask: Borrar todo (0xFFFF)
        0x00, 0x00, 0x00, 0x00,  // saveMask: No guardar nada (0x0000)
        0xFF, 0xFF, 0xFF, 0xFF,  // loadMask: Cargar todo desde la memoria no volátil (0xFFFF)
        0x07,
        0x00, 0x00   // Checksum (placeholder)
    };

    calculateChecksum(ubx_cfg_cfg_factory_reset, sizeof(ubx_cfg_cfg_factory_reset));
    uart_write_bytes(UART_NUM_2, (const char*)ubx_cfg_cfg_factory_reset, sizeof(ubx_cfg_cfg_factory_reset));
}

void Bz251::configSetValue(uint64_t key, uint8_t value)
{
    uint8_t len = 0x09;
    uint8_t ubx_cfg[] = {
        0xB5, 0x62, // Sync chars (UBX header)
        0x06, 0x8A, // Clase e ID del mensaje
        len, 0x00, // Longitud del payload
        0x00, 0x01, 0x00, 0x00, // Message version(0), 1-ram 2-bbr 4-flash, reserved
        (uint8_t)(key), (uint8_t)(key >> 8), (uint8_t)(key >> 16), (uint8_t)(key >> 24),
        value,
        0x00, 0x00 // Checksum (placeholder)
    };

    calculateChecksum(ubx_cfg, sizeof(ubx_cfg));
    uart_write_bytes(UART_NUM_2, (const char*)ubx_cfg, sizeof(ubx_cfg));
}

void Bz251::configGetValue(uint64_t key)
{
    uint8_t ubx_cfg[] = {
        0xB5, 0x62, // Sync chars (UBX header)
        0x06, 0x8B, // Clase e ID del mensaje
        0x08, 0x00, // Longitud del payload
        0x00, 0x00, 0x00, 0x00, // Message version(0), 0-ram 1-bbr 2-flash 7-default, reserved
        (uint8_t)(key), (uint8_t)(key >> 8), (uint8_t)(key >> 16), (uint8_t)(key >> 24),
        0x00, 0x00 // Checksum (placeholder)
    };

    calculateChecksum(ubx_cfg, sizeof(ubx_cfg));
    uart_write_bytes(UART_NUM_2, (const char*)ubx_cfg, sizeof(ubx_cfg));
}

void Bz251::setValueFlash(uint64_t key, uint8_t value)
{
    uint8_t len = 0x09;
    uint8_t ubx_cfg[] = {
        0xB5, 0x62, // Sync chars (UBX header)
        0x06, 0x8A, // Clase e ID del mensaje
        len, 0x00, // Longitud del payload
        0x00, 0x07, 0x00, 0x00, // Message version(0), 1-ram 2-bbr 4-flash, reserved
        (uint8_t)(key), (uint8_t)(key >> 8), (uint8_t)(key >> 16), (uint8_t)(key >> 24),
        value,
        0x00, 0x00 // Checksum (placeholder)
    };

    calculateChecksum(ubx_cfg, sizeof(ubx_cfg));
    uart_write_bytes(UART_NUM_2, (const char*)ubx_cfg, sizeof(ubx_cfg));
}