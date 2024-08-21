import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import sensor, uart
from esphome.const import (
    CONF_ID,
    CONF_VOLTAGE,
    DEVICE_CLASS_CURRENT,
    DEVICE_CLASS_ENERGY,
    DEVICE_CLASS_POWER,
    DEVICE_CLASS_VOLTAGE,
    STATE_CLASS_MEASUREMENT,
    STATE_CLASS_TOTAL_INCREASING,
    UNIT_AMPERE,
    UNIT_KILOWATT_HOURS,
    UNIT_VOLT,
    UNIT_WATT,
    CONF_ADDRESS,
)

DEPENDENCIES = ["uart"]

CONF_CURRENT_1 = "current_1"
CONF_CURRENT_2 = "current_2"
CONF_ACTIVE_POWER_1 = "active_power_1"
CONF_ACTIVE_POWER_2 = "active_power_2"
CONF_ENERGY_1 = "energy_1"
CONF_ENERGY_2 = "energy_2"
CONF_ENERGY_TOTAL = "energy_total"
CONF_CURRENT_REFERENCE = "current_reference"
CONF_ENERGY_REFERENCE = "energy_reference"
CONF_POWER_REFERENCE = "power_reference"
CONF_VOLTAGE_REFERENCE = "voltage_reference"
CONF_LINE_FREQUENCY = "line_frequency"

bl0939_ns = cg.esphome_ns.namespace("bl0939")
BL0939 = bl0939_ns.class_("BL0939", cg.PollingComponent, uart.UARTDevice)

LineFrequency = bl0939_ns.enum("LineFrequency")
LINE_FREQS = {
    50: LineFrequency.LINE_FREQUENCY_50HZ,
    60: LineFrequency.LINE_FREQUENCY_60HZ,
}

CONFIG_SCHEMA = (
    cv.Schema(
        {
            cv.GenerateID(): cv.declare_id(BL0939),
            cv.Optional(CONF_VOLTAGE): sensor.sensor_schema(
                unit_of_measurement=UNIT_VOLT,
                accuracy_decimals=1,
                device_class=DEVICE_CLASS_VOLTAGE,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_CURRENT_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_AMPERE,
                accuracy_decimals=2,
                device_class=DEVICE_CLASS_CURRENT,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACTIVE_POWER_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ACTIVE_POWER_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_WATT,
                accuracy_decimals=0,
                device_class=DEVICE_CLASS_POWER,
                state_class=STATE_CLASS_MEASUREMENT,
            ),
            cv.Optional(CONF_ENERGY_1): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_ENERGY_2): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_ENERGY_TOTAL): sensor.sensor_schema(
                unit_of_measurement=UNIT_KILOWATT_HOURS,
                accuracy_decimals=3,
                device_class=DEVICE_CLASS_ENERGY,
                state_class=STATE_CLASS_TOTAL_INCREASING,
            ),
            cv.Optional(CONF_LINE_FREQUENCY, default="50HZ"): cv.All(
                cv.frequency,
                cv.enum(
                    LINE_FREQS,
                    int=True,
                ),
            ),
            cv.Optional(CONF_ADDRESS, default=5): cv.int_range(min=0, max=15),
            cv.Optional(CONF_CURRENT_REFERENCE): cv.float_,
            cv.Optional(CONF_ENERGY_REFERENCE): cv.float_,
            cv.Optional(CONF_POWER_REFERENCE): cv.float_,
            cv.Optional(CONF_VOLTAGE_REFERENCE): cv.float_,
        }
    )
    .extend(cv.polling_component_schema("60s"))
    .extend(uart.UART_DEVICE_SCHEMA)
)


async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)

    if voltage_config := config.get(CONF_VOLTAGE):
        sens = await sensor.new_sensor(voltage_config)
        cg.add(var.set_voltage_sensor(sens))
    if current_1_config := config.get(CONF_CURRENT_1):
        sens = await sensor.new_sensor(current_1_config)
        cg.add(var.set_current_sensor_1(sens))
    if current_2_config := config.get(CONF_CURRENT_2):
        sens = await sensor.new_sensor(current_2_config)
        cg.add(var.set_current_sensor_2(sens))
    if active_power_1_config := config.get(CONF_ACTIVE_POWER_1):
        sens = await sensor.new_sensor(active_power_1_config)
        cg.add(var.set_power_sensor_1(sens))
    if active_power_2_config := config.get(CONF_ACTIVE_POWER_2):
        sens = await sensor.new_sensor(active_power_2_config)
        cg.add(var.set_power_sensor_2(sens))
    if energy_1_config := config.get(CONF_ENERGY_1):
        sens = await sensor.new_sensor(energy_1_config)
        cg.add(var.set_energy_sensor_1(sens))
    if energy_2_config := config.get(CONF_ENERGY_2):
        sens = await sensor.new_sensor(energy_2_config)
        cg.add(var.set_energy_sensor_2(sens))
    if energy_total_config := config.get(CONF_ENERGY_TOTAL):
        sens = await sensor.new_sensor(energy_total_config)
        cg.add(var.set_energy_sensor_sum(sens))
    cg.add(var.set_line_freq(config[CONF_LINE_FREQUENCY]))
    cg.add(var.set_address(config.get(CONF_ADDRESS)))
    if config.get(CONF_CURRENT_REFERENCE):
        cg.add(var.set_current_reference(config[CONF_CURRENT_REFERENCE]))
    if config.get(CONF_ENERGY_REFERENCE):
        cg.add(var.set_energy_reference(config[CONF_ENERGY_REFERENCE]))
    if config.get(CONF_POWER_REFERENCE):
        cg.add(var.set_power_reference(config[CONF_POWER_REFERENCE]))
    if config.get(CONF_VOLTAGE_REFERENCE):
        cg.add(var.set_voltage_reference(config[CONF_VOLTAGE_REFERENCE]))
