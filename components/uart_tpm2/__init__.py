import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_UART_ID
from esphome.core import coroutine

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UARTTPM2 = uart_tpm2_ns.class_('UARTTPM2', cg.Component)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UARTTPM2),
    cv.Optional(uart.CONF_UART_ID): cv.use_id(uart.UARTComponent),
})

@coroutine
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_UART_ID in config:
        uart_id = await cg.get_variable(config[CONF_UART_ID])
        cg.add(var.set_uart(uart_id))