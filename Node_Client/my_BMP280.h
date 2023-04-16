#include <Adafruit_I2CDevice.h>
#include <Wire.h>

/* Số lần lấy Sampling - page13*/
enum Sampling {
  SAMPLING_NONE = 0x00,
  SAMPLING_X1 = 0x01,
  SAMPLING_X2 = 0x02,
  SAMPLING_X4 = 0x03,
  SAMPLING_X8 = 0x04, 
  SAMPLING_X16 = 0x05
};
/*MODE hoạt động của cảm biến - page 15*/
enum Mode {
  Sleep = 0x00, 
  forced = 0x01,
  normal = 0x03,
  soft_reset_code = 0xB6
};
/*Mức độ lọc Filter*/
enum Filter {
  FILTER_OFF = 0x00, 
  FILTER_X2 = 0x01,
  FILTER_X4 = 0x02, 
  FILTER_X8 = 0x03,
  FILTER_X16 = 0x04
};
/** Standby duration - thời gian chờ giữa các lần đo(ms) - page 17*/
enum Standby_duration {
  STANDBY_MS_1 = 0x00,
  STANDBY_MS_63 = 0x01,
  STANDBY_MS_125 = 0x02,
  STANDBY_MS_250 = 0x03,
  STANDBY_MS_500 = 0x04,
  STANDBY_MS_1000 = 0x05,
  STANDBY_MS_2000 = 0x06,
  STANDBY_MS_4000 = 0x07
};
//Địa chỉ của thanh ghi sử dụng cho calib, config, controlMeasure  - page 20+23 - page 25+26
enum Addr_REGISTER{
  //Địa chỉ thanh ghi sử dụng để calib Nhiệt độ Temperature(các thanh ghi này chứa các giá trị được định nghĩa sẵn)
  Addr_REGISTER_DIG_T1 = 0x88,
  Addr_REGISTER_DIG_T2 = 0x8A,
  Addr_REGISTER_DIG_T3 = 0x8C,

  //Địa chỉ thanh ghi sử dụng để calib Áp suất Pressure(các thanh ghi này chứa các giá trị được định nghĩa sẵn)
  Addr_REGISTER_DIG_P1 = 0x8E,
  Addr_REGISTER_DIG_P2 = 0x90,
  Addr_REGISTER_DIG_P3 = 0x92,
  Addr_REGISTER_DIG_P4 = 0x94,
  Addr_REGISTER_DIG_P5 = 0x96,
  Addr_REGISTER_DIG_P6 = 0x98,
  Addr_REGISTER_DIG_P7 = 0x9A,
  Addr_REGISTER_DIG_P8 = 0x9C,
  Addr_REGISTER_DIG_P9 = 0x9E,

  // Địa thanh ghi CONTROL_MEASURE - page25
  Addr_REGISTER_CONTROL_MEASURE = 0xF4,

  // Địa thanh ghi CONFIG - page26
  Addr_REGISTER_CONFIG = 0xF5,
  // Địa chỉ của thanh ghi đàu tiên chưa dữ liệu Temperature - page24+27
  Addr_REGISTER_TEMPDATA = 0xFA,
};
//Cấu hìnhThanh ghi 0xF5 “config”
struct config_Reg {
  /* Khởi tạo mặc định */
  config_Reg() : t_sb(STANDBY_MS_1), filter(FILTER_OFF), none(0), spi3w_en(0) {}
  //Setting standby time trong normal mode - 3bit - page 17
  unsigned int t_sb : 3;
  //Filter settings - 3bit
  unsigned int filter : 3;
  //Bit này không sử dụng - 1bit
  unsigned int none : 1;
  //Cho phép sử dụng SPI - 1bit
  unsigned int spi3w_en : 1;
  // Truy xuất giá trị của thanh ghi sau khi tổng hợp
  unsigned int get() { return (t_sb << 5) | (filter << 2) | spi3w_en; }
};
//Cấu hìnhThanh ghi r 0xF4 “ctrl_meas” - page25*/
struct ctrl_measure_Reg {
   /* Khởi tạo mặc định */
  ctrl_measure_Reg(): temp(SAMPLING_NONE), press(SAMPLING_NONE), mode(Sleep) {}
  // Setting số lần Sampling của Temperature
  unsigned int temp : 3;
  // Setting số lần Sampling của  Pressure 
  unsigned int press : 3;
  // MODE Setting
  unsigned int mode : 2;
  // Truy xuất giá trị của thanh ghi sau khi tổng hợp
  unsigned int get() { return (temp << 5) | (press << 2) | mode; }
};
//Định nghĩa một kiểu dữ liệu để lưu trữ giá trị của các được lưu trong các thanh ghi sử dụng để calib
struct calib_dataa{
  uint16_t dig_T1; 
  int16_t dig_T2; 
  int16_t dig_T3; 

  uint16_t dig_P1; 
  int16_t dig_P2;  
  int16_t dig_P3;  
  int16_t dig_P4;  
  int16_t dig_P5;  
  int16_t dig_P6;  
  int16_t dig_P7;  
  int16_t dig_P8;  
  int16_t dig_P9;  
};

/**Các hàm thao tác đọc và ghi dữ liệu qua sensor bằng i2c*/
void write8bit_i2c(byte reg, byte value); // hàm ghi dữ liệu - page29
uint16_t read16bit_i2c(byte reg);// hàm đọc dữ liệu KHÔNG DẤU - pa29+30
uint16_t read16bit_LE_i2c(byte reg);// hàm đọc dữ liệu KHÔNG DẤU với dạng lưu trễ Little Endian
int16_t read_Sign16bit_i2c(byte reg);// hàm đọc dữ liệu CÓ DẤU
int16_t read_Sign16bit_LE_i2c(byte reg);// hàm đọc dữ liệu CÓ DẤU với dạng lưu trễ Little Endian
uint32_t read24bit_i2c(byte reg);// hàm đọc dữ liệu KHÔNG DẤU(24bit)

/*Hàm đọc các giá trị được định nghĩa sẵn trong cảm biến thông qua I2C(bằng các hàm vừa xây dựng trên) để lưu trữ vào các thanh ghi dùng cho calib */
void read_Compensation_parameter_storage();
/**Hàm khởi tạo*/
bool init_BMP280(uint8_t addr);

/** Hàm để setup cho cảm biến trước khi tiến hành đo, hàm sẽ tiến hành cấu hình lại 2 thanh ghi
                Control_Measure và Config 
  để phù hợp với yêu cầu của người dùng, sau đó tiến hành ghi vào cảm biến
*/
void setup_BMP280( Mode mode, Sampling tempSampling, Sampling pressSampling,
                   Filter filter, Standby_duration duration );
/**Hàm dùng để đọc giá trị Nhiệt độ từ thanh ghi REGISTER_TEMPDATA, có cả đoạn chương trình calib dữ liệu sau khi thu được*/
float readTemperaturee() ;