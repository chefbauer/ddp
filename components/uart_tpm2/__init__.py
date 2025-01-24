import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UUID
from esphome.core import coroutine

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UARTTPM2 = uart_tpm2_ns.class_('UARTTPM2', cg.Component, uart.UARTDevice)

# Verwende CONF_UUID statt eines neuen CONF_*-Werts
CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UARTTPM2),
    cv.Required(uart.CONF_UART_ID): cv.use_id(uart.UARTComponent),
    cv.Required(CONF_UUID): cv.use_id(cg.float_),  # Hier verwenden wir cg.float_ als Platzhalter
})

@coroutine
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID], id(config[CONF_UUID]))
    await cg.register_component(var, config)
    await uart.register_uart_device(var, config)