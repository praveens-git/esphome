#include "bl0939.h"
#include "esphome/core/log.h"
#include <cinttypes>

namespace esphome {
namespace bl0939 {

static const char *const TAG = "bl0939";

// https://www.belling.com.cn/media/file_object/bel_product/BL0939/datasheet/BL0939_V1.2_cn.pdf
// (unfortunately chinese, but the protocol can be understood with some translation tool)
static const uint8_t BL0939_READ_COMMAND = 0x50;  // 0x5{A4,A3,A2,A1}
static const uint8_t BL0939_FULL_PACKET = 0xAA;
static const uint8_t BL0939_PACKET_HEADER = 0x55;

static const uint8_t BL0939_WRITE_COMMAND = 0xA0;  // 0xA{A4,A3,A2,A1}
static const uint8_t BL0939_REG_IA_FAST_RMS_CTRL = 0x10;
static const uint8_t BL0939_REG_IB_FAST_RMS_CTRL = 0x1E;
static const uint8_t BL0939_REG_MODE = 0x18;
static const uint8_t BL0939_REG_SOFT_RESET = 0x19;
static const uint8_t BL0939_REG_USR_WRPROT = 0x1A;
static const uint8_t BL0939_REG_TPS_CTRL = 0x1B;

static const uint8_t BL0939_INIT_DEFAULT[6][6] = {
    // Reset to default
    {BL0939_WRITE_COMMAND, BL0939_REG_SOFT_RESET, 0x5A, 0x5A, 0x5A, 0x33},
    // Enable User Operation Write
    {BL0939_WRITE_COMMAND, BL0939_REG_USR_WRPROT, 0x55, 0x00, 0x00, 0xEB},
    // 0x0100 = CF_UNABLE energy pulse, AC_FREQ_SEL 50Hz, RMS_UPDATE_SEL 800mS
    {BL0939_WRITE_COMMAND, BL0939_REG_MODE, 0x00, 0x10, 0x00, 0x32},
    // 0x47FF = Over-current and leakage alarm on, Automatic temperature measurement, Interval 100mS
    {BL0939_WRITE_COMMAND, BL0939_REG_TPS_CTRL, 0xFF, 0x47, 0x00, 0xF9},
    // 0x181C = Half cycle, Fast RMS threshold 6172
    {BL0939_WRITE_COMMAND, BL0939_REG_IA_FAST_RMS_CTRL, 0x1C, 0x18, 0x00, 0x16},
    // 0x181C = Half cycle, Fast RMS threshold 6172
    {BL0939_WRITE_COMMAND, BL0939_REG_IB_FAST_RMS_CTRL, 0x1C, 0x18, 0x00, 0x08}};


static const uint32_t BL0939_REG_MODE_RMS_400MS = 0;
static const uint32_t BL0939_REG_MODE_RMS_800MS = 0x100;
static const uint32_t BL0939_REG_MODE_50HZ_FREQ = 0;
static const uint32_t BL0939_REG_MODE_60HZ_FREQ = 0x200;
static const uint32_t BL0939_REG_MODE_CF_OUT_A = 0;
static const uint32_t BL0939_REG_MODE_CF_OUT_B = 0x800;
static const uint32_t BL0939_REG_MODE_CFUNABLE_ENERGY_PULSE = 0;
static const uint32_t BL0939_REG_MODE_CFUNABLE_ALARM = 0x1000;

static const uint32_t BL0939_REG_MODE_DEFAULT = BL0939_REG_MODE_RMS_800MS | BL0939_REG_MODE_50HZ_FREQ | BL0939_REG_MODE_CF_OUT_A | BL0939_REG_MODE_CFUNABLE_ENERGY_PULSE;
static const uint32_t BL0939_REG_TPS_DEFAULT = 0x07FF;

static const uint32_t BL0939_REG_SOFT_RESET_MAGIC = 0x5A5A5A;
static const uint32_t BL0939_REG_USR_WRPROT_MAGIC = 0x55;

void BL0939::loop() {
  DataPacket buffer;
  if (!this->available()) {
    return;
  }
  if (read_array((uint8_t *) &buffer, sizeof(buffer))) {
    if (validate_checksum_(&buffer)) {
      received_package_(&buffer);
    }
  } else {
    ESP_LOGW(TAG, "Junk on wire. Throwing away partial message");
    while (read() >= 0)
      ;
  }
}

bool BL0939::validate_checksum_(const DataPacket *data) const {
  uint8_t checksum = BL0939_READ_COMMAND | (this->address_ & 0xF);
  ESP_LOGI(TAG, "Address of the Device: %d %d %d",checksum, BL0939_READ_COMMAND , (this->address_));

  // Whole package but checksum
  for (uint32_t i = 0; i < sizeof(data->raw) - 1; i++) {
    checksum += data->raw[i];
    ESP_LOGI(TAG, "Values[%d]: %d %d",i, data->raw[i], checksum);
  }
  checksum ^= 0xFF;
  if (checksum != data->checksum) {
    ESP_LOGW(TAG, "BL0939 invalid checksum! 0x%02X != 0x%02X", checksum, data->checksum);
  }
  return checksum == data->checksum;
}

void BL0939::update() {
  this->flush();
  this->write_byte(BL0939_READ_COMMAND | (this->address_ & 0xF));
  this->write_byte(BL0939_FULL_PACKET);
}

void BL0939::write_reg_(uint8_t reg, uint32_t val) {
  uint8_t pkt[6];

  this->flush();
  pkt[0] = BL0939_WRITE_COMMAND | this->address_;
  pkt[1] = reg;
  pkt[2] = (val & 0xff);
  pkt[3] = (val >> 8) & 0xff;
  pkt[4] = (val >> 16) & 0xff;
  pkt[5] = (pkt[0] + pkt[1] + pkt[2] + pkt[3] + pkt[4]) ^ 0xff;
  this->write_array(pkt, 6);
  delay(1);
}

int BL0939::read_reg_(uint8_t reg) {
  union {
    uint8_t b[4];
    uint32_t le32;
  } resp;

  this->write_byte(BL0939_READ_COMMAND | this->address_);
  this->write_byte(reg);
  this->flush();
  if (this->read_array(resp.b, 4) &&
      resp.b[3] ==
          (uint8_t) ((BL0939_READ_COMMAND + this->address_ + reg + resp.b[0] + resp.b[1] + resp.b[2]) ^ 0xff)) {
    resp.b[3] = 0;
    return resp.le32;
  }
  return -1;
}

void BL0939::setup() {

  this->write_reg_(BL0939_REG_SOFT_RESET, BL0939_REG_SOFT_RESET_MAGIC);
  this->write_reg_(BL0939_REG_USR_WRPROT, BL0939_REG_USR_WRPROT_MAGIC);

  uint32_t mode = BL0939_REG_MODE_DEFAULT;
  
  if (this->line_frequency_ == LINE_FREQUENCY_60HZ)
    mode |= BL0939_REG_MODE_60HZ_FREQ;

  this->write_reg_(BL0939_REG_MODE, mode);

  this->write_reg_(BL0939_REG_USR_WRPROT, 0);

  if (this->read_reg_(BL0939_REG_MODE) != mode)
    this->status_set_warning("BL0939 setup failed!");

  this->flush();
}

void BL0939::received_package_(const DataPacket *data) const {
  // Bad header
  if (data->frame_header != BL0939_PACKET_HEADER) {
    ESP_LOGI(TAG, "Invalid data. Header mismatch: %d", data->frame_header);
    return;
  }

  float v_rms = (float) to_uint32_t(data->v_rms) / voltage_reference_;
  float ia_rms = (float) to_uint32_t(data->ia_rms) / current_reference_;
  float ib_rms = (float) to_uint32_t(data->ib_rms) / current_reference_;
  float a_watt = (float) to_int32_t(data->a_watt) / power_reference_;
  float b_watt = (float) to_int32_t(data->b_watt) / power_reference_;
  int32_t cfa_cnt = to_int32_t(data->cfa_cnt);
  int32_t cfb_cnt = to_int32_t(data->cfb_cnt);
  float a_energy_consumption = (float) cfa_cnt / energy_reference_;
  float b_energy_consumption = (float) cfb_cnt / energy_reference_;
  float total_energy_consumption = a_energy_consumption + b_energy_consumption;

  if (voltage_sensor_ != nullptr) {
    voltage_sensor_->publish_state(v_rms);
  }
  if (current_sensor_1_ != nullptr) {
    current_sensor_1_->publish_state(ia_rms);
  }
  if (current_sensor_2_ != nullptr) {
    current_sensor_2_->publish_state(ib_rms);
  }
  if (power_sensor_1_ != nullptr) {
    power_sensor_1_->publish_state(a_watt);
  }
  if (power_sensor_2_ != nullptr) {
    power_sensor_2_->publish_state(b_watt);
  }
  if (energy_sensor_1_ != nullptr) {
    energy_sensor_1_->publish_state(a_energy_consumption);
  }
  if (energy_sensor_2_ != nullptr) {
    energy_sensor_2_->publish_state(b_energy_consumption);
  }
  if (energy_sensor_sum_ != nullptr) {
    energy_sensor_sum_->publish_state(total_energy_consumption);
  }

  ESP_LOGV(TAG,
           "BL0939: U %fV, I1 %fA, I2 %fA, P1 %fW, P2 %fW, CntA %" PRId32 ", CntB %" PRId32 ", ∫P1 %fkWh, ∫P2 %fkWh",
           v_rms, ia_rms, ib_rms, a_watt, b_watt, cfa_cnt, cfb_cnt, a_energy_consumption, b_energy_consumption);
}

void BL0939::dump_config() {  // NOLINT(readability-function-cognitive-complexity)
  ESP_LOGCONFIG(TAG, "BL0939:");
  LOG_SENSOR("", "Voltage", this->voltage_sensor_);
  LOG_SENSOR("", "Current 1", this->current_sensor_1_);
  LOG_SENSOR("", "Current 2", this->current_sensor_2_);
  LOG_SENSOR("", "Power 1", this->power_sensor_1_);
  LOG_SENSOR("", "Power 2", this->power_sensor_2_);
  LOG_SENSOR("", "Energy 1", this->energy_sensor_1_);
  LOG_SENSOR("", "Energy 2", this->energy_sensor_2_);
  LOG_SENSOR("", "Energy sum", this->energy_sensor_sum_);
  ESP_LOGCONFIG(TAG, "Device Address: %d", this->address_);
}

uint32_t BL0939::to_uint32_t(ube24_t input) { return input.h << 16 | input.m << 8 | input.l; }

int32_t BL0939::to_int32_t(sbe24_t input) { return input.h << 16 | input.m << 8 | input.l; }

}  // namespace bl0939
}  // namespace esphome
