#include "my_BMP280.h"

/**Các biến cần sử dụng*/
calib_dataa dig;
ctrl_measure_Reg MeaReg;
config_Reg ConfReg;
Adafruit_I2CDevice *i2c_device = NULL; // khởi tạo một con trỏ I2C để thực hiện các thao tác đọc ghi dữ liệu vào cảm biến


void write8bit_i2c(byte reg, byte value){ // hàm ghi dữ liệu - page29
  byte buffer[2];
  buffer[1] = value;
  buffer[0] = reg;
  i2c_device->write(buffer,2);// dữ liệu, độ dài byte
}
uint16_t read16bit_i2c(byte reg) { // hàm đọc dữ liệu KHÔNG DẤU - pa29+30
  uint8_t buffer[3];
  int i=0;
    buffer[0] = uint8_t(reg);
    i2c_device->write_then_read(buffer, 1, buffer, 2);

  return uint16_t(buffer[0]) << 8 | uint16_t(buffer[1]);
}
uint16_t read16bit_LE_i2c(byte reg) {// hàm đọc dữ liệu KHÔNG DẤU với dạng lưu trễ Little Endian
  uint16_t temp = read16bit_i2c(reg);
  return (temp >> 8) | (temp << 8);
}
int16_t read_Sign16bit_i2c(byte reg) {// hàm đọc dữ liệu CÓ DẤU
  return (int16_t)read16bit_i2c(reg);
}
int16_t read_Sign16bit_LE_i2c(byte reg) {// hàm đọc dữ liệu CÓ DẤU với dạng lưu trễ Little Endian
  return (int16_t)read16bit_LE_i2c(reg);
}
uint32_t read24bit_i2c(byte reg) {// hàm đọc dữ liệu KHÔNG DẤU(24bit)
  uint8_t buffer[3];
    buffer[0] = uint8_t(reg);
    i2c_device->write_then_read(buffer, 1, buffer, 3);

  return uint32_t(buffer[0]) << 16 | uint32_t(buffer[1]) << 8 |
         uint32_t(buffer[2]);
}

/*Hàm đọc các giá trị được định nghĩa sẵn trong cảm biến thông qua I2C(bằng các hàm vừa xây dựng trên) để lưu trữ vào các thanh ghi dùng cho calib */
void read_Compensation_parameter_storage() {
  dig.dig_T1 = read16bit_LE_i2c(Addr_REGISTER_DIG_T1);
  dig.dig_T2 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_T2);
  dig.dig_T3 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_T3);
    
  dig.dig_P1 = read16bit_LE_i2c(Addr_REGISTER_DIG_P1);
  dig.dig_P2 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P2);
  dig.dig_P3 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P3);
  dig.dig_P4 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P4);
  dig.dig_P5 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P5);
  dig.dig_P6 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P6);
  dig.dig_P7 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P7);
  dig.dig_P8 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P8);
  dig.dig_P9 = read_Sign16bit_LE_i2c(Addr_REGISTER_DIG_P9);
}
/**Hàm khởi tạo*/
bool init_BMP280(uint8_t addr){// tham số là địa chỉ của cảm biến để tiến hành kết nối i2c
  if (i2c_device)
      delete i2c_device;
  i2c_device = new Adafruit_I2CDevice(addr);/*Tạo ra một đối tượng con trỏ để thực hiện thao tác trên I2C*/
  if (!i2c_device->begin()) 
    return false;
  else
    return true;
}

/** Hàm để setup cho cảm biến trước khi tiến hành đo, hàm sẽ tiến hành cấu hình lại 2 thanh ghi
                Control_Measure và Config 
  để phù hợp với yêu cầu của người dùng, sau đó tiến hành ghi vào cảm biến
*/
void setup_BMP280( Mode mode = normal, Sampling tempSampling = SAMPLING_X16, Sampling pressSampling = SAMPLING_X16,
                   Filter filter = FILTER_OFF, Standby_duration duration = STANDBY_MS_1){
        MeaReg.mode = mode;
        MeaReg.temp = tempSampling;
        MeaReg.press = pressSampling;

        ConfReg.filter = filter;
        ConfReg.t_sb = duration;
        //khi vào 2 thanh ghi trên giá trị vừa được cấu hình
        write8bit_i2c(Addr_REGISTER_CONTROL_MEASURE,MeaReg.get());
        write8bit_i2c(Addr_REGISTER_CONFIG,ConfReg.get());
}
float readTemperaturee() {
  int32_t var1, var2;
  
  int32_t adc_T = read24bit_i2c(Addr_REGISTER_TEMPDATA); // đọc ra giá trị của 3 thanh ghi chứa dữ liệu của Temperature
  /**Các công thức calib(Compensation formula) sau được tham khảo từ DATASHEET của cảm biến - page21*/
  adc_T >>= 4;

  var1 = ((((adc_T >> 3) - ((int32_t)dig.dig_T1 << 1))) * ((int32_t)dig.dig_T2)) >> 11;

  var2 = (((((adc_T >> 4) - ((int32_t)dig.dig_T1)) * ((adc_T >> 4) - ((int32_t)dig.dig_T1))) >> 12) * ((int32_t)dig.dig_T3)) >> 14;

  int32_t t_fine = var1 + var2;

  float T = (t_fine * 5 + 128) >> 8;
  return T / 100;
}