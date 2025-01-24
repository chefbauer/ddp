import esphome.codegen as cg
import esphome.config_validation as cv
from esphome.components import uart
from esphome.const import CONF_ID, CONF_NAME, CONF_UART_ID
from esphome.core import coroutine

uart_tpm2_ns = cg.esphome_ns.namespace('uart_tpm2')
UARTTPM2 = uart_tpm2_ns.class_('UARTTPM2', cg.Component, uart.UARTDevice)

# Hier wird angenommen, dass es eine Art von Dekorator gibt, der ähnlich wie register_addressable_effect funktioniert.
# Da es keine direkte Entsprechung für eine allgemeine Komponente gibt, verwenden wir hier eine vereinfachte Version.
def register_uart_tpm2_effect(name, effect_class, friendly_name):
    return lambda config: {
        cv.GenerateID(): cv.declare_id(effect_class),
        cv.GenerateID(CONF_UART_ID): cv.use_id(uart.UARTComponent),
    }

@register_uart_tpm2_effect(
    "uart_tpm2",
    UARTTPM2,
    "UART TPM2",
)
def CONFIG_SCHEMA(config):
    return cv.Schema({
        cv.GenerateID(): cv.declare_id(UARTTPM2),
    })

@coroutine
async def to_code(config):
    var = cg.new_Pvariable(config[CONF_ID])
    await cg.register_component(var, config)
    if CONF_UART_ID in config:
        uart_id = await cg.get_variable(config[CONF_UART_ID])
        await uart.register_uart_device(var, uart_id)
    else:
        # Falls keine spezifische UART-ID angegeben ist, wird die Standard-UART-Komponente verwendet.
        await uart.register_uart_device(var, config)