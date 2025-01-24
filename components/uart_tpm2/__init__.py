from esphome import automation
import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID

DEPENDENCIES = ["uart"]

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UartTpm2 = uart_tpm2_ns.class_('UartTpm2', cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UartTpm2),
    cv.GenerateID(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    cv.Optional('variable_id'): cv.use_id(cg.global_ns.namespace('std').template('array').template('array').template('unsigned char').template('512').template('3')),
}).extend(cv.COMPONENT_SCHEMA)

async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    uart_parent = await cg.get_variable(config[CONF_UART_ID])
    await uart.register_uart_device(var, uart_parent)

    if 'variable_id' in config:
        cg.add(var.set_it_bg(config['variable_id']))
    else:
        cg.add(var.set_it_bg(cg.RawExpression('new unsigned char[512][3]()')))