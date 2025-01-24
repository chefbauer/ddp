import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID
from esphome.core import coroutine

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UARTTPM2 = uart_tpm2_ns.class_('UARTTPM2', cg.Component, uart.UARTDevice)

CONFIG_SCHEMA = cv.Schema({
    cv.GenerateID(): cv.declare_id(UARTTPM2),
})

@coroutine
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    # Die UART-Komponente wird automatisch auf die Standard-UART gesetzt, wie bei Adalight