import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
# from esphome.const import CONF_ID
# from esphome.core import coroutine

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UARTTPM2 = uart_tpm2_ns.class_('UARTTPM2', cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UARTTPM2),
    cv.Required(uart.CONF_UART_ID): cv.use_id(uart.UARTComponent),
})





# @coroutine
# def to_code(config):
#     var = cg.new_Pvariable(config[CONF_ID])
#     yield cg.register_component(var, config)
#     yield uart.register_uart_device(var, config)